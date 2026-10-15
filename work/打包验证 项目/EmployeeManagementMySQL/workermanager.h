#pragma once
#include "database.h"

class workermanager {
public:
    explicit workermanager(Database& database) : repository_(database) {}
    void run();
    void show_menu();
    void add_emp();
    void show_emp();
    void del_emp();
    void mod_emp();
    void find_emp();
    void sort_emp();
    void clean_file();
private:
    WorkerRepository repository_;
    static WorkerRecord read_worker();
    static void show_records(const std::vector<WorkerRecord>& records);
};
