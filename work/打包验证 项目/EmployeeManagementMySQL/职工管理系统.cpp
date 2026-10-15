#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "database.h"
#include "workermanager.h"
#include <filesystem>
#include <iostream>

class Utf8Console {
public:
    Utf8Console() : input_(GetConsoleCP()), output_(GetConsoleOutputCP()) {
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    }
    ~Utf8Console() { if (input_) SetConsoleCP(input_); if (output_) SetConsoleOutputCP(output_); }
private:
    UINT input_, output_;
};

int wmain(int argc, wchar_t* argv[]) {
    Utf8Console console;
    try {
        std::filesystem::path configPath;
        if (argc == 2 && std::wstring(argv[1]) == L"--help") {
            std::cout << "用法：EmployeeManagement.exe [--config 数据库配置文件路径]\n"
                      << "默认读取可执行文件所在目录的 database.ini。\n";
            return 0;
        }
        if (argc == 3 && std::wstring(argv[1]) == L"--config") configPath = argv[2];
        else if (argc == 1) {
            wchar_t executable[32768]{};
            auto length = GetModuleFileNameW(nullptr, executable, 32768);
            if (!length || length == 32768) throw std::runtime_error("无法获取程序所在目录，请使用 --config。");
            configPath = std::filesystem::path(executable).parent_path() / L"database.ini";
        } else throw std::runtime_error("参数有误，使用 --help 查看用法。");
        auto config = DbConfig::load(configPath.u8string());
        Database db(config);
        WorkerRepository repository(db);
        // Check table/schema before presenting a menu that cannot operate.
        repository.list();
        std::cout << "已连接 MySQL：" << config.host << ':' << config.port << '/' << config.database << "\n";
        workermanager manager(db);
        manager.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "启动失败：" << error.what() << "\n"
                  << "请检查 database.ini、MySQL 服务及 sql/setup.sql 建表脚本。\n";
        return 1;
    }
}
