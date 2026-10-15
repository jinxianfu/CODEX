# 职工管理系统（MySQL 版）

基于你提供的 C++ 控制台项目改造。保留员工、经理、老板及原有 0–7 菜单；数据直接保存在 MySQL，程序不再读写 empfile.txt。

连接目标已设置为 **127.0.0.1:3306 / root / hrms**。项目使用 **hrms.workers** 表，原有 **hrms.employee** 表保留不动。

## 首次运行

1. 解压整个文件夹到任意目录，不要单独取出 EXE。需要 Windows x64 和运行中的 MySQL **8.0.16 或更高版本**。
2. 在 MySQL Workbench 中用 root 登录，打开 `sql/setup.sql` 并执行。脚本只创建缺少的数据库/表，不清空数据。若由本次改造直接在你的机器上交付，hrms.workers 已创建并验证。
3. 用记事本编辑根目录 `database.ini`：在 `password=` 后填写你的 MySQL 密码并保存。发布的 ZIP **不包含你的密码**。密码值不要加引号，允许包含 `#`、`;`、`=`、`%`；密码前后的空格会原样保留。
4. 双击 `run.bat`。该脚本使用根目录配置，启动 `bin/x64/Release/EmployeeManagement.exe`。

双击 EXE 直接运行时，它读取 **EXE 同目录**的 database.ini；推荐使用 run.bat，避免编辑错配置文件。

连接配置示例：

```ini
[mysql]
host=127.0.0.1
port=3306
user=root
password=
database=hrms
```

也可用 `EMPLOYEE_DB_PASSWORD` 环境变量提供密码，它会优先于配置文件。连接失败时程序提示错误，不会转为文件保存。

## Visual Studio 中打开和编译

安装 Visual Studio **2022** 的“使用 C++ 的桌面开发”工作负载和 Windows 10/11 SDK，打开 `EmployeeManagement.sln`，选择 **Release / x64**。第一个项目 EmployeeManagement 为主程序，DatabaseTests 为集成测试程序。

MySQL C API 头文件、x64 导入库及客户端 DLL 已放在 `third_party/mysql`，项目使用相对路径，不需要重新设置 MySQL include/lib 路径。没有 Win32 配置，避免和 x64 MySQL 库混用。

也可以双击 `build.bat`，或在命令提示符执行：

```bat
build.bat Release
build.bat Debug
```

输出在 `bin/x64/Release` 或 `bin/x64/Debug`。编译会把根目录 database.ini 复制到输出目录；使用 VS 的 F5 时应确保根目录配置已填写，启动项目选择 EmployeeManagement。

EXE 使用静态 C++ 运行库，但官方 `libmysql.dll` 还依赖 **Microsoft Visual C++ 2015–2022 Redistributable（x64）**。本机已安装；其他电脑如提示缺少 MSVCP140.dll 或 VCRUNTIME140.dll，请安装微软官方 x64 运行库：https://aka.ms/vs/17/release/vc_redist.x64.exe 。MySQL 客户端的三个 DLL 会随项目复制到程序目录。

## 功能和数据库对应关系

| 菜单 | 行为 |
|---|---|
| 1 增加 | 参数化 INSERT；整批事务提交，重复编号或失败时整批回滚 |
| 2 显示 | SELECT 全部职工，每次读取数据库最新数据 |
| 3 删除 | 按编号 DELETE，显示记录并确认后删除 |
| 4 修改 | 参数化 UPDATE，可同时修改编号、姓名、岗位 |
| 5 查找 | 按编号或完整姓名查询，同名返回多条；姓名区分大小写 |
| 6 排序 | SQL ORDER BY id ASC / DESC；排序控制显示顺序 |
| 7 清空 | 确认后 DELETE workers 全部记录 |

编号必须为 1–2147483647；姓名为 1–100 个 UTF-8 字符，可含中文、空格、引号和 emoji；岗位为 1 员工 / 2 经理 / 3 老板。每行输入一项，输入错误时会提示，输入流结束时未提交的批次取消。

在 Workbench 中查看程序操作结果：

```sql
SELECT id, name, dept_id FROM hrms.workers ORDER BY id;
```

你也可以在 MySQL 中直接增删改查 workers，程序下次查询会看到结果：

```sql
INSERT INTO hrms.workers (id, name, dept_id) VALUES (101, '张三', 1);
UPDATE hrms.workers SET name = '张三修改', dept_id = 2 WHERE id = 101;
SELECT * FROM hrms.workers WHERE id = 101;
DELETE FROM hrms.workers WHERE id = 101;
```

workers 使用 InnoDB、utf8mb4、id 主键及岗位 CHECK 约束。启动时会验证存储引擎、字段结构和主键，拒绝不兼容旧表，不会自动改表。MySQL 中已有 employee 表包含工资等字段，与原程序不同，所以此版本使用独立 workers 表。旧 empfile.txt 不会自动导入；可按上方 INSERT 格式迁移需要保留的记录。

## 测试

本项目已在真实 MySQL 实例上运行数据库集成测试和菜单端到端测试。结果见 `docs/验证结果.md`。

`DatabaseTests.exe` 和 `tests/console_tests.py` 会清空测试库的 workers 表，**仅对专用测试库运行**。两者均拒绝数据库名不以 `_test` 结尾的配置。可复制 setup.sql，将数据库名改为 hrms_test 后执行，再新建 `test-database.ini`，设置 database=hrms_test。

```bat
bin\x64\Release\DatabaseTests.exe test-database.ini
python tests\console_tests.py --exe bin\x64\Release\EmployeeManagement.exe --config test-database.ini --mysql "C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe"
```

Python 3 只用于可选菜单测试，主程序不需要 Python。

## 主要文件

- `database.h/.cpp`：配置加载、连接生命周期、表结构检查、预处理 SQL、CRUD 及事务。
- `workermanager.h/.cpp`：菜单和职工操作。
- `worker.h`、employee/manager/boss：保留原多态职工信息展示，增加基类虚析构。
- `职工管理系统.cpp`：UTF-8 控制台、启动和错误提示。
- `sql/setup.sql`：正式库建表脚本。
- `database.ini`：本机连接信息；ZIP 中密码为空。
- `third_party/mysql/LICENSE`：随附 MySQL 官方客户端 SDK 许可证。

第三方 MySQL Community 客户端为 GPL 软件，许可证随包提供。自行分发或商用时，请根据实际使用方式遵守随附许可证。
