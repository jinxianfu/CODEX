# MySQL 职工管理系统实施计划

目标：将现有控制台项目的职工增删改查从文本文件改为 MySQL，交付可编译的 VS2022 x64 项目及 ZIP。

结构：保留 worker/employee/manager/boss 的展示；Database 管理连接、配置及参数化 SQL；WorkerRepository 提供数据操作；workermanager 处理菜单输入。独立复制到 outputs，不修改 D 盘原项目。

- [ ] 写真实 MySQL 集成测试，覆盖中文、引号、重复编号、批量事务回滚、改号、查找、排序、删除、清空、重新连接持久性、非法输入及错误配置；确认缺少数据库实现导致失败。
- [ ] 实现 database.h/.cpp；连接字符集 utf8mb4；INSERT/UPDATE/DELETE/SELECT 使用预处理语句；批量写入事务；编号为正整数、岗位 1/2/3、姓名 1–100 个 UTF-8 字符。
- [ ] 改造 workermanager.h/.cpp 和入口；菜单逐行读取、EOF 正常退出；每次查询数据库；删除和清空需用户确认；保留多态展示并为基类增加虚析构。
- [ ] 创建 sql/setup.sql、database.ini、README、VS solution/project、构建及启动脚本；内附本机官方 MySQL x64 客户端 SDK 与许可证；生成 Debug/Release 配置。
- [ ] 用 work/mysql-test 下独立 MySQL 实例运行真实集成测试和控制台输入测试，编译 Debug/Release，验证客户端 DLL 依赖；记录验证结果。
- [ ] 打包源码、SQL、配置示例、说明、测试及 Release 可执行文件；检查压缩包；关闭测试实例。
