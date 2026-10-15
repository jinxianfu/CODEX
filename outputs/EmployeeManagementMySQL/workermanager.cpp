#include "workermanager.h"
#include "console_input.h"
#include "employee.h"
#include "manager.h"
#include "boss.h"
#include <memory>
#include <limits>
#include <set>

void workermanager::show_menu() {
    std::cout << "\n********** 职工管理系统 · MySQL **********\n"
              << "0. 退出管理程序\n1. 增加职工信息\n2. 显示职工信息\n"
              << "3. 删除职工信息\n4. 修改职工信息\n5. 查找职工信息\n"
              << "6. 按照编号排序\n7. 清空所有职工\n"
              << "*****************************************\n";
}

void workermanager::run() {
    while (true) {
        try {
            show_menu();
            switch (read_number("请输入您的选择：", 0, 7)) {
            case 0: std::cout << "欢迎下次使用！\n"; return;
            case 1: add_emp(); break;
            case 2: show_emp(); break;
            case 3: del_emp(); break;
            case 4: mod_emp(); break;
            case 5: find_emp(); break;
            case 6: sort_emp(); break;
            case 7: clean_file(); break;
            }
        } catch (const InputClosed&) {
            std::cout << "\n输入结束，未提交的输入已取消。\n";
            return;
        } catch (const std::exception& error) {
            std::cerr << "操作失败：" << error.what() << "\n";
        }
    }
}

WorkerRecord workermanager::read_worker() {
    WorkerRecord value{};
    value.id = read_number("请输入职工编号（正整数）：", 1, (std::numeric_limits<int>::max)());
    value.name = read_text("请输入职工姓名（1–100 字，可含空格）：");
    value.deptId = read_number("请选择岗位：1. 员工  2. 经理  3. 老板：", 1, 3);
    WorkerRepository::validate(value);
    return value;
}

void workermanager::show_records(const std::vector<WorkerRecord>& records) {
    if (records.empty()) { std::cout << "数据库中没有符合条件的职工记录。\n"; return; }
    for (const auto& record : records) {
        std::unique_ptr<worker> person;
        switch (record.deptId) {
        case 1: person = std::make_unique<employee>(record.id, record.name, record.deptId); break;
        case 2: person = std::make_unique<manager>(record.id, record.name, record.deptId); break;
        case 3: person = std::make_unique<boss>(record.id, record.name, record.deptId); break;
        default: throw std::runtime_error("数据库中的岗位编号无效。");
        }
        person->showinfo();
    }
    std::cout << "共 " << records.size() << " 条记录。\n";
}

void workermanager::add_emp() {
    int count = read_number("请输入增加职工数量（1–1000）：", 1, 1000);
    std::vector<WorkerRecord> records;
    std::set<int> ids;
    for (int i = 0; i < count; ++i) {
        std::cout << "第 " << i + 1 << " 位职工：\n";
        auto record = read_worker();
        if (!ids.insert(record.id).second) throw std::invalid_argument("本批次职工编号重复，本批次全部取消。");
        records.push_back(std::move(record));
    }
    repository_.addBatch(records);
    std::cout << "成功添加 " << count << " 名职工，已保存到 MySQL。\n";
}
void workermanager::show_emp() { show_records(repository_.list()); }
void workermanager::del_emp() {
    int id = read_number("请输入要删除的职工编号：", 1, (std::numeric_limits<int>::max)());
    auto found = repository_.findById(id);
    if (found.empty()) { std::cout << "未找到该职工。\n"; return; }
    show_records(found);
    if (read_number("确认删除？1. 确认  2. 返回：", 1, 2) != 1) { std::cout << "已取消。\n"; return; }
    std::cout << (repository_.remove(id) ? "删除成功，MySQL 已更新。\n" : "该职工已被其他连接删除。\n");
}
void workermanager::mod_emp() {
    int id = read_number("请输入要修改的原职工编号：", 1, (std::numeric_limits<int>::max)());
    auto found = repository_.findById(id);
    if (found.empty()) { std::cout << "未找到该职工。\n"; return; }
    show_records(found);
    std::cout << "请输入完整的新信息（编号可以保持不变）：\n";
    auto replacement = read_worker();
    std::cout << (repository_.update(id, replacement) ? "修改成功，MySQL 已更新。\n" : "该职工已被其他连接删除。\n");
}
void workermanager::find_emp() {
    int mode = read_number("查找方式：1. 按职工编号  2. 按姓名：", 1, 2);
    if (mode == 1) show_records(repository_.findById(read_number("请输入职工编号：", 1, (std::numeric_limits<int>::max)())));
    else show_records(repository_.findByName(read_text("请输入完整姓名：")));
}
void workermanager::sort_emp() {
    int mode = read_number("排序方式：1. 编号升序  2. 编号降序：", 1, 2);
    show_records(repository_.list(mode == 2));
}
void workermanager::clean_file() {
    if (read_number("此操作将删除数据库 workers 表中的全部职工。1. 确认  2. 返回：", 1, 2) != 1) {
        std::cout << "已取消。\n";
        return;
    }
    repository_.clear();
    std::cout << "清空成功，MySQL 已更新。\n";
}
