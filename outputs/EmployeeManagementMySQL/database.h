#pragma once
#include <mysql.h>
#include <string>
#include <vector>
#include <variant>
#include <stdexcept>

struct DbConfig {
    std::string host = "127.0.0.1";
    unsigned int port = 3306;
    std::string user;
    std::string password;
    std::string database = "hrms";
    static DbConfig load(const std::string& path);
};

struct WorkerRecord {
    int id;
    std::string name;
    int deptId;
};

class DatabaseError : public std::runtime_error {
public:
    DatabaseError(unsigned int code, const std::string& message);
    unsigned int code() const noexcept { return code_; }
private:
    unsigned int code_;
};

using SqlValue = std::variant<int, std::string>;

class Database {
public:
    explicit Database(const DbConfig& config);
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    unsigned long long execute(const char* sql, const std::vector<SqlValue>& values = {});
    std::vector<WorkerRecord> query(const char* sql, const std::vector<SqlValue>& values = {});
    void begin();
    void commit();
    void rollback() noexcept;
    void validateWorkerSchema();
private:
    MYSQL* connection_ = nullptr;
    void command(const char* sql);
};

class WorkerRepository {
public:
    explicit WorkerRepository(Database& db);
    void addBatch(const std::vector<WorkerRecord>& workers);
    std::vector<WorkerRecord> list(bool descending = false);
    std::vector<WorkerRecord> findById(int id);
    std::vector<WorkerRecord> findByName(const std::string& name);
    bool update(int oldId, const WorkerRecord& replacement);
    bool remove(int id);
    void clear();
    static void validate(const WorkerRecord& worker);
private:
    Database& db_;
};
