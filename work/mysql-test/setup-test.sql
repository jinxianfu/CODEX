-- MySQL 8.0.16+; run as a user with CREATE privileges.
-- Existing data is preserved; this script does not drop databases or tables.
CREATE DATABASE IF NOT EXISTS employee_management_test
    CHARACTER SET utf8mb4 COLLATE utf8mb4_bin;
USE employee_management_test;
CREATE TABLE IF NOT EXISTS workers (
    id INT NOT NULL COMMENT '职工编号',
    name VARCHAR(100) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL COMMENT '姓名',
    dept_id INT NOT NULL COMMENT '1=员工, 2=经理, 3=老板',
    PRIMARY KEY (id),
    KEY idx_workers_name (name),
    CONSTRAINT chk_worker_id CHECK (id > 0),
    CONSTRAINT chk_worker_dept CHECK (dept_id IN (1, 2, 3)),
    CONSTRAINT chk_worker_name CHECK (CHAR_LENGTH(TRIM(name)) BETWEEN 1 AND 100)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

-- You can use MySQL Workbench to inspect changes:
-- SELECT id, name, dept_id FROM employee_management_test.workers ORDER BY id;
