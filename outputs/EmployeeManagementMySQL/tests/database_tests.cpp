#include "database.h"
#include <iostream>
#include <stdexcept>
#include <functional>

static int passed = 0;
static void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
    ++passed;
}
static void rejects(const std::function<void()>& operation, const char* message) {
    bool rejected = false;
    try { operation(); } catch (const std::exception&) { rejected = true; }
    check(rejected, message);
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("Usage: database_tests.exe test-database.ini");
        auto config = DbConfig::load(argv[1]);
        // This suite clears the table: refuse production database names.
        if (config.database.size() < 5 || config.database.substr(config.database.size() - 5) != "_test")
            throw std::runtime_error("Tests require a database ending with _test");
        Database db(config);
        WorkerRepository repo(db);
        repo.clear();
        check(repo.list().empty(), "empty table");
        repo.addBatch({{20, u8"张三 O'Neil 😀", 1}, {10, u8"李四", 2}, {30, u8"李四", 3}});
        auto found = repo.findById(20);
        check(found.size() == 1 && found[0].name == u8"张三 O'Neil 😀", "Unicode and quote round trip");
        check(repo.findByName(u8"李四").size() == 2, "same-name employees returned");
        check(repo.findByName("' OR 1=1 --").empty(), "query values cannot inject SQL");
        rejects([&] { repo.addBatch({{40, "new", 1}, {20, "duplicate", 2}}); }, "duplicate batch rejected");
        check(repo.findById(40).empty() && repo.list().size() == 3, "failed batch rolls back");
        rejects([&] { repo.addBatch({{50, "first", 1}, {50, "second", 2}}); }, "duplicate within batch rejected");
        check(repo.findById(50).empty(), "same-batch duplicate rolls back");
        check(repo.update(20, {21, u8"新姓名 王五", 3}), "update id, name and department");
        check(repo.findById(20).empty() && repo.findById(21)[0].deptId == 3, "old id removed and new id visible");
        rejects([&] { repo.update(21, {10, "collision", 1}); }, "update duplicate rejected");
        check(repo.findById(21)[0].name == u8"新姓名 王五", "failed update preserves original");
        check(repo.update(21, {21, u8"新姓名 王五", 3}), "unchanged update still finds row");
        check(!repo.update(999, {999, "absent", 1}), "update missing returns false");
        check(repo.list()[0].id == 10 && repo.list(true)[0].id == 30, "ascending and descending order");
        {
            Database another(config);
            WorkerRepository fresh(another);
            check(fresh.findById(21)[0].name == u8"新姓名 王五", "new connection sees persisted data");
            fresh.addBatch({{60, "external", 1}});
        }
        check(repo.findById(60).size() == 1, "fresh reads see other connection changes");
        check(repo.remove(21) && repo.findById(21).empty(), "delete persists");
        check(!repo.remove(999), "delete missing returns false");
        rejects([&] { repo.addBatch({{0, "bad", 1}}); }, "zero id rejected");
        rejects([&] { repo.addBatch({{-1, "bad", 1}}); }, "negative id rejected");
        rejects([&] { repo.addBatch({{70, "bad", 4}}); }, "invalid department rejected");
        rejects([&] { repo.addBatch({{70, "", 1}}); }, "empty name rejected");
        rejects([&] { repo.addBatch({{70, std::string(101, 'x'), 1}}); }, "oversize name rejected");
        rejects([&] { repo.addBatch({{70, std::string("\xC0\xAF"), 1}}); }, "malformed UTF-8 rejected");
        repo.addBatch({{70, std::string(100, 'x'), 1}});
        check(repo.findById(70)[0].name.size() == 100, "maximum name accepted");
        repo.clear();
        check(repo.list().empty(), "clear persists");
        db.execute("ALTER TABLE workers ENGINE=MyISAM");
        rejects([&] { WorkerRepository unsupported(db); }, "nontransactional table rejected before writes");
        db.execute("ALTER TABLE workers ENGINE=InnoDB");
        db.execute("ALTER TABLE workers MODIFY id INT UNSIGNED NOT NULL");
        rejects([&] { WorkerRepository unsupported(db); }, "unsigned id schema rejected");
        db.execute("ALTER TABLE workers MODIFY id INT NOT NULL");
        db.execute("ALTER TABLE workers MODIFY name VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL");
        rejects([&] { WorkerRepository unsupported(db); }, "short name schema rejected");
        db.execute("ALTER TABLE workers MODIFY name VARCHAR(100) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL");
        auto wrong = config;
        wrong.port = 1;
        rejects([&] { Database failed(wrong); }, "connection failure reported");
        rejects([] { DbConfig::load("nonexistent-database-config.ini"); }, "missing config reported");
        std::cout << "PASS: " << passed << " database integration checks\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
