#include "database.h"
#include <array>
#include <fstream>
#include <memory>
#include <map>
#include <cstdlib>
#include <limits>
#include <filesystem>

namespace {
std::string trim(const std::string& text) {
    auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

// Count Unicode scalar values, rejecting invalid encodings rather than losing data.
std::size_t utf8Length(const std::string& text) {
    std::size_t count = 0;
    for (std::size_t i = 0; i < text.size();) {
        auto lead = static_cast<unsigned char>(text[i]);
        unsigned int scalar = 0, minimum = 0;
        std::size_t width = 0;
        if (lead < 0x80) { width = 1; scalar = lead; }
        else if (lead >= 0xC2 && lead <= 0xDF) { width = 2; scalar = lead & 0x1F; minimum = 0x80; }
        else if (lead >= 0xE0 && lead <= 0xEF) { width = 3; scalar = lead & 0x0F; minimum = 0x800; }
        else if (lead >= 0xF0 && lead <= 0xF4) { width = 4; scalar = lead & 0x07; minimum = 0x10000; }
        else throw std::invalid_argument("姓名必须是有效的 UTF-8 文本。");
        if (i + width > text.size()) throw std::invalid_argument("姓名 UTF-8 文本不完整。");
        for (std::size_t j = 1; j < width; ++j) {
            auto ch = static_cast<unsigned char>(text[i + j]);
            if ((ch & 0xC0) != 0x80) throw std::invalid_argument("姓名 UTF-8 文本无效。");
            scalar = (scalar << 6) | (ch & 0x3F);
        }
        if (scalar < minimum || scalar > 0x10FFFF || (scalar >= 0xD800 && scalar <= 0xDFFF)
            || scalar < 0x20 || scalar == 0x7F)
            throw std::invalid_argument("姓名包含无效字符或控制字符。");
        i += width;
        ++count;
    }
    return count;
}

class Statement {
public:
    Statement(MYSQL* connection, const char* sql, const std::vector<SqlValue>& values)
        : handle_(mysql_stmt_init(connection), mysql_stmt_close), bindings_(values.size()), lengths_(values.size()) {
        if (!handle_) throw std::runtime_error("无法创建 MySQL 预处理语句。");
        if (mysql_stmt_prepare(get(), sql, static_cast<unsigned long>(std::char_traits<char>::length(sql)))) fail();
        if (mysql_stmt_param_count(get()) != values.size()) throw std::logic_error("SQL 参数数量不匹配。");
        for (std::size_t i = 0; i < values.size(); ++i) {
            auto& binding = bindings_[i];
            if (std::holds_alternative<int>(values[i])) {
                binding.buffer_type = MYSQL_TYPE_LONG;
                binding.buffer = const_cast<int*>(&std::get<int>(values[i]));
            } else {
                const auto& text = std::get<std::string>(values[i]);
                if (text.size() > (std::numeric_limits<unsigned long>::max)())
                    throw std::invalid_argument("SQL 参数过长。");
                lengths_[i] = static_cast<unsigned long>(text.size());
                binding.buffer_type = MYSQL_TYPE_STRING;
                binding.buffer = const_cast<char*>(text.data());
                binding.buffer_length = lengths_[i];
                binding.length = &lengths_[i];
            }
        }
        if (!bindings_.empty() && mysql_stmt_bind_param(get(), bindings_.data())) fail();
        if (mysql_stmt_execute(get())) fail();
    }
    MYSQL_STMT* get() const { return handle_.get(); }
    [[noreturn]] void fail() const { throw DatabaseError(mysql_stmt_errno(get()), mysql_stmt_error(get())); }
private:
    std::unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> handle_;
    std::vector<MYSQL_BIND> bindings_;
    std::vector<unsigned long> lengths_;
};
}

DbConfig DbConfig::load(const std::string& path) {
    std::ifstream file(std::filesystem::u8path(path), std::ios::binary);
    if (!file) throw std::runtime_error("无法读取数据库配置：" + path + "。请先编辑 database.ini。");
    std::map<std::string, std::string> settings;
    std::string line;
    bool firstLine = true;
    while (std::getline(file, line)) {
        if (firstLine && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        firstLine = false;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto cleaned = trim(line);
        if (cleaned.empty() || cleaned[0] == '#' || cleaned[0] == ';' || cleaned == "[mysql]") continue;
        auto equal = line.find('=');
        if (equal == std::string::npos) throw std::runtime_error("database.ini 格式错误：每项应为 key=value。");
        auto key = trim(line.substr(0, equal));
        if (key != "host" && key != "port" && key != "user" && key != "password" && key != "database")
            throw std::runtime_error("database.ini 中存在未知配置项：" + key);
        if (settings.count(key)) throw std::runtime_error("database.ini 中配置项重复：" + key);
        settings[key] = key == "password" ? line.substr(equal + 1) : trim(line.substr(equal + 1));
    }
    DbConfig config;
    if (settings.count("host")) config.host = settings.at("host");
    if (settings.count("database")) config.database = settings.at("database");
    if (settings.count("user")) config.user = settings.at("user");
    if (settings.count("password")) config.password = settings.at("password");
    if (settings.count("port")) {
        const auto& port = settings.at("port");
        if (port.empty() || port.find_first_not_of("0123456789") != std::string::npos)
            throw std::runtime_error("MySQL 端口必须是 1–65535 的整数。");
        try {
            auto value = std::stoul(port);
            if (value == 0 || value > 65535) throw std::out_of_range("port");
            config.port = static_cast<unsigned int>(value);
        } catch (const std::exception&) { throw std::runtime_error("MySQL 端口必须是 1–65535 的整数。"); }
    }
    char* environmentPassword = nullptr;
    std::size_t environmentLength = 0;
    if (_dupenv_s(&environmentPassword, &environmentLength, "EMPLOYEE_DB_PASSWORD") != 0)
        throw std::runtime_error("无法读取数据库密码环境变量。");
    std::unique_ptr<char, decltype(&std::free)> passwordOwner(environmentPassword, std::free);
    if (environmentPassword) config.password = environmentPassword;
    if (config.host.empty() || config.user.empty() || config.database.empty())
        throw std::runtime_error("database.ini 的 host、user 和 database 不能为空。");
    return config;
}

DatabaseError::DatabaseError(unsigned int code, const std::string& message)
    : std::runtime_error(code == 1062 ? "职工编号已存在，操作取消，原数据保持不变。"
        : "MySQL 错误 " + std::to_string(code) + "：" + message), code_(code) {}

Database::Database(const DbConfig& config) {
    connection_ = mysql_init(nullptr);
    if (!connection_) throw std::runtime_error("MySQL 连接初始化失败。");
    unsigned int timeout = 5;
    try {
        if (mysql_options(connection_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout)
            || mysql_options(connection_, MYSQL_SET_CHARSET_NAME, "utf8mb4"))
            throw DatabaseError(mysql_errno(connection_), mysql_error(connection_));
        if (!mysql_real_connect(connection_, config.host.c_str(), config.user.c_str(), config.password.c_str(),
                config.database.c_str(), config.port, nullptr, CLIENT_FOUND_ROWS))
            throw DatabaseError(mysql_errno(connection_), mysql_error(connection_));
        if (mysql_set_character_set(connection_, "utf8mb4"))
            throw DatabaseError(mysql_errno(connection_), mysql_error(connection_));
    } catch (...) {
        mysql_close(connection_);
        connection_ = nullptr;
        throw;
    }
}
Database::~Database() { if (connection_) mysql_close(connection_); }

unsigned long long Database::execute(const char* sql, const std::vector<SqlValue>& values) {
    Statement statement(connection_, sql, values);
    return mysql_stmt_affected_rows(statement.get());
}
std::vector<WorkerRecord> Database::query(const char* sql, const std::vector<SqlValue>& values) {
    Statement statement(connection_, sql, values);
    if (mysql_stmt_field_count(statement.get()) != 3) throw std::runtime_error("职工表查询字段不匹配。");
    int id = 0, deptId = 0;
    std::array<char, 401> name{};
    unsigned long length = 0;
    std::array<bool, 3> nulls{}, errors{};
    std::array<MYSQL_BIND, 3> results{};
    results[0].buffer_type = MYSQL_TYPE_LONG;
    results[0].buffer = &id;
    results[1].buffer_type = MYSQL_TYPE_STRING;
    results[1].buffer = name.data();
    results[1].buffer_length = static_cast<unsigned long>(name.size());
    results[1].length = &length;
    results[2].buffer_type = MYSQL_TYPE_LONG;
    results[2].buffer = &deptId;
    for (std::size_t i = 0; i < results.size(); ++i) {
        results[i].is_null = &nulls[i];
        results[i].error = &errors[i];
    }
    if (mysql_stmt_bind_result(statement.get(), results.data()) || mysql_stmt_store_result(statement.get())) statement.fail();
    std::vector<WorkerRecord> rows;
    while (true) {
        int status = mysql_stmt_fetch(statement.get());
        if (status == MYSQL_NO_DATA) break;
        if (status == MYSQL_DATA_TRUNCATED || errors[0] || errors[1] || errors[2])
            throw std::runtime_error("数据库记录超出字段限制，请检查 workers 表结构。");
        if (status) statement.fail();
        if (nulls[0] || nulls[1] || nulls[2]) throw std::runtime_error("数据库职工记录存在空字段。");
        WorkerRecord record{id, std::string(name.data(), length), deptId};
        WorkerRepository::validate(record);
        rows.push_back(std::move(record));
    }
    return rows;
}
void Database::command(const char* sql) {
    if (mysql_query(connection_, sql)) throw DatabaseError(mysql_errno(connection_), mysql_error(connection_));
}
void Database::begin() { command("START TRANSACTION"); }
void Database::commit() { command("COMMIT"); }
void Database::rollback() noexcept { mysql_rollback(connection_); }

void Database::validateWorkerSchema() {
    // Never silently use a legacy/nontransactional table: rollback must be real.
    auto readRows = [this](const char* sql) {
        command(sql);
        std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(mysql_store_result(connection_), mysql_free_result);
        if (!result) throw DatabaseError(mysql_errno(connection_), mysql_error(connection_));
        std::vector<std::vector<std::string>> rows;
        const auto fields = mysql_num_fields(result.get());
        while (auto row = mysql_fetch_row(result.get())) {
            std::vector<std::string> values;
            for (unsigned int i = 0; i < fields; ++i) values.emplace_back(row[i] ? row[i] : "");
            rows.push_back(std::move(values));
        }
        if (mysql_errno(connection_)) throw DatabaseError(mysql_errno(connection_), mysql_error(connection_));
        return rows;
    };
    const auto tables = readRows("SELECT ENGINE, TABLE_TYPE FROM information_schema.TABLES "
        "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'workers'");
    if (tables.size() != 1) throw std::runtime_error("workers 表不存在，请先执行 sql/setup.sql。");
    if (tables[0][0] != "InnoDB" || tables[0][1] != "BASE TABLE")
        throw std::runtime_error("workers 必须是 InnoDB 数据表，才能保证批量写入回滚。请备份并人工检查旧表。");
    // Older MySQL 8.0 versions report signed INT as int(11); display width is irrelevant.
    const auto columns = readRows("SELECT COLUMN_NAME, "
        "IF(DATA_TYPE = 'int' AND COLUMN_TYPE NOT LIKE '%unsigned%', 'int', COLUMN_TYPE), "
        "IS_NULLABLE, CHARACTER_SET_NAME, COLLATION_NAME "
        "FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'workers' ORDER BY ORDINAL_POSITION");
    const std::vector<std::vector<std::string>> expected{
        {"id", "int", "NO", "", ""},
        {"name", "varchar(100)", "NO", "utf8mb4", "utf8mb4_bin"},
        {"dept_id", "int", "NO", "", ""}
    };
    if (columns != expected) throw std::runtime_error("workers 字段结构不符合项目要求，请对照 sql/setup.sql；不会自动修改旧表。");
    const auto primary = readRows("SELECT COLUMN_NAME FROM information_schema.STATISTICS "
        "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'workers' AND INDEX_NAME = 'PRIMARY' ORDER BY SEQ_IN_INDEX");
    if (primary != std::vector<std::vector<std::string>>{{"id"}})
        throw std::runtime_error("workers 主键必须仅包含 id，以保证职工编号唯一。");
    const auto otherUnique = readRows("SELECT INDEX_NAME FROM information_schema.STATISTICS "
        "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'workers' AND NON_UNIQUE = 0 AND INDEX_NAME <> 'PRIMARY'");
    if (!otherUnique.empty()) throw std::runtime_error("workers 包含额外唯一索引，会限制重名职工，请人工检查旧表。");
}

WorkerRepository::WorkerRepository(Database& db) : db_(db) { db_.validateWorkerSchema(); }

void WorkerRepository::validate(const WorkerRecord& worker) {
    if (worker.id <= 0) throw std::invalid_argument("职工编号必须是正整数。");
    if (worker.deptId < 1 || worker.deptId > 3) throw std::invalid_argument("岗位只能为 1（员工）、2（经理）、3（老板）。");
    const auto count = utf8Length(worker.name);
    if (count == 0 || count > 100 || trim(worker.name).empty())
        throw std::invalid_argument("职工姓名必须包含 1–100 个字符，不能全为空格。");
}
void WorkerRepository::addBatch(const std::vector<WorkerRecord>& workers) {
    if (workers.empty()) throw std::invalid_argument("新增职工列表不能为空。");
    for (const auto& worker : workers) validate(worker);
    db_.begin();
    try {
        for (const auto& worker : workers)
            db_.execute("INSERT INTO workers (id, name, dept_id) VALUES (?, ?, ?)", {worker.id, worker.name, worker.deptId});
        db_.commit();
    } catch (...) { db_.rollback(); throw; }
}
std::vector<WorkerRecord> WorkerRepository::list(bool descending) {
    return db_.query(descending ? "SELECT id, name, dept_id FROM workers ORDER BY id DESC"
        : "SELECT id, name, dept_id FROM workers ORDER BY id ASC");
}
std::vector<WorkerRecord> WorkerRepository::findById(int id) {
    return db_.query("SELECT id, name, dept_id FROM workers WHERE id = ?", {id});
}
std::vector<WorkerRecord> WorkerRepository::findByName(const std::string& name) {
    return db_.query("SELECT id, name, dept_id FROM workers WHERE name = ? ORDER BY id", {name});
}
bool WorkerRepository::update(int oldId, const WorkerRecord& replacement) {
    validate(replacement);
    return db_.execute("UPDATE workers SET id = ?, name = ?, dept_id = ? WHERE id = ?",
        {replacement.id, replacement.name, replacement.deptId, oldId}) != 0;
}
bool WorkerRepository::remove(int id) { return db_.execute("DELETE FROM workers WHERE id = ?", {id}) != 0; }
void WorkerRepository::clear() { db_.execute("DELETE FROM workers"); }
