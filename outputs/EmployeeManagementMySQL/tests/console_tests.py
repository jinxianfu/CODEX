"""End-to-end menu tests against a dedicated *_test database (clears workers)."""
import argparse
import configparser
import os
import subprocess
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--exe', required=True)
parser.add_argument('--config', required=True)
parser.add_argument('--mysql', required=True)
args = parser.parse_args()
exe = str(Path(args.exe).resolve())
config = str(Path(args.config).resolve())
cfg = configparser.ConfigParser(interpolation=None)
cfg.read(config, encoding='utf-8-sig')
db = cfg['mysql']
if not db['database'].endswith('_test'):
    raise SystemExit('Refusing to clear a database that does not end with _test')
environment = {key.upper(): value for key, value in os.environ.items()}
environment.pop('EMPLOYEE_DB_PASSWORD', None)
environment['MYSQL_PWD'] = db.get('password', '')
mysql = [args.mysql, '--no-defaults', '--protocol=TCP', '--host=' + db['host'],
         '--port=' + db['port'], '--user=' + db['user'], '--database=' + db['database'],
         '--default-character-set=utf8mb4', '--batch', '--skip-column-names']
passed = 0

def sql(query):
    process = subprocess.run(mysql + ['-e', query], env=environment, capture_output=True,
                             encoding='utf-8', errors='replace', timeout=15)
    if process.returncode:
        raise RuntimeError(process.stderr)
    return process.stdout.strip()

def run(lines, config_override=None):
    process = subprocess.run([exe, '--config', config_override or config],
                             input='\n'.join(lines) + ('\n' if lines else ''), env=environment,
                             capture_output=True, encoding='utf-8', errors='strict', timeout=15)
    return process.returncode, process.stdout + process.stderr

def check(value, label):
    global passed
    if not value:
        raise AssertionError(label)
    passed += 1

sql('DELETE FROM workers')
status, out = run(['1', '2', '11', "张三 O'Neil 😀", '1', '12', '李四', '2', '2', '0'])
check(status == 0 and "张三 O'Neil 😀" in out and '成功添加 2' in out, 'menu adds and displays Chinese names')
check(sql('SELECT COUNT(*) FROM workers') == '2', 'menu insert visible through mysql client')
status, out = run(['5', '1', '11', '5', '2', "张三 O'Neil 😀", '0'])
check(status == 0 and out.count("职工姓名:张三 O'Neil 😀") == 2, 'id/name search in a second process')
status, out = run(['4', '11', '13', '新姓名 王五', '3', '0'])
check(status == 0 and '修改成功' in out, 'menu update succeeds')
check(sql('SELECT CONCAT(id, ":", name, ":", dept_id) FROM workers WHERE id=13') == '13:新姓名 王五:3', 'update visible through mysql client')
status, out = run(['4', '13', '12', 'collision', '1', '0'])
check(status == 0 and '编号已存在' in out and sql('SELECT name FROM workers WHERE id=13') == '新姓名 王五', 'duplicate update preserves original')
status, out = run(['6', '2', '0'])
check(status == 0 and out.index('职工编号:13') < out.index('职工编号:12'), 'descending menu order')
status, out = run(['3', '13', '2', '0'])
check(status == 0 and sql('SELECT COUNT(*) FROM workers') == '2', 'cancelled deletion preserves rows')
status, out = run(['3', '13', '1', '0'])
check(status == 0 and '删除成功' in out and sql('SELECT COUNT(*) FROM workers WHERE id=13') == '0', 'confirmed deletion persists')
status, out = run(['7', '2', '0'])
check(status == 0 and sql('SELECT COUNT(*) FROM workers') == '1', 'cancelled clear preserves rows')
status, out = run(['7', '1', '0'])
check(status == 0 and '清空成功' in out and sql('SELECT COUNT(*) FROM workers') == '0', 'confirmed clear persists')
status, out = run(['bad', '999', '0'])
check(status == 0 and out.count('输入无效') == 2, 'invalid menu input recovers')
status, out = run(['1', '1', '21', '岗位校验', '7', '2', '0'])
check(status == 0 and '输入无效' in out and sql('SELECT dept_id FROM workers WHERE id=21') == '2', 'invalid department re-prompts')
status, out = run(['1', '2', '31', '未完成批次', '1', '32'])
check(status == 0 and sql('SELECT COUNT(*) FROM workers WHERE id IN (31,32)') == '0', 'EOF cancels whole unsubmitted batch')
status, out = run(['1', '1', '41', '', '1', '0'])
check(status == 0 and '操作失败' in out and sql('SELECT COUNT(*) FROM workers WHERE id=41') == '0', 'empty name rejected without a null worker')
status, out = run(['1', '1', '51', "' OR 1=1 --", '1', '5', '2', "' OR 1=1 --", '0'])
check(status == 0 and '共 1 条记录' in out and sql('SELECT COUNT(*) FROM workers') == '2', 'injection-shaped name is stored/searched as text')
with tempfile.TemporaryDirectory() as temporary:
    unicode_config = Path(temporary) / '数据库配置.ini'
    unicode_config.write_text(Path(config).read_text(encoding='utf-8-sig'), encoding='utf-8-sig')
    status, out = run(['0'], str(unicode_config))
    check(status == 0 and '已连接 MySQL' in out, 'Unicode config path and BOM supported')
    missing = str(Path(temporary) / 'missing.ini')
    status, out = run([], missing)
    check(status == 1 and '启动失败' in out, 'missing config fails clearly')
    bad_config = Path(temporary) / 'bad.ini'
    bad_config.write_text('[mysql]\nhost=127.0.0.1\nport=3306abc\nuser=root\ndatabase=test\n', encoding='utf-8')
    status, out = run([], str(bad_config))
    check(status == 1 and '端口必须' in out, 'malformed port fails before connection')
sql('DELETE FROM workers')
print(f'PASS: {passed} console end-to-end checks')
