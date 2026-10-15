"""Initialize only the new workers table, verify CRUD, then remove only our test row."""
import os
import secrets
import subprocess
from pathlib import Path

project = Path('outputs/EmployeeManagementMySQL').resolve()
config_path = project / 'database.ini'
settings = {}
for line in config_path.read_text(encoding='utf-8-sig').splitlines():
    cleaned = line.strip()
    if not cleaned or cleaned[0] in '#;[':
        continue
    key, value = line.split('=', 1)
    settings[key.strip()] = value if key.strip() == 'password' else value.strip()
environment = {key.upper(): value for key, value in os.environ.items()}
environment['MYSQL_PWD'] = environment.get('EMPLOYEE_DB_PASSWORD', settings['password'])
mysql = [r'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe', '--no-defaults', '--protocol=TCP',
         '--host=' + settings['host'], '--port=' + settings['port'], '--user=' + settings['user'],
         '--default-character-set=utf8mb4', '--batch', '--skip-column-names']
exe = project / 'bin/x64/Release/EmployeeManagement.exe'

def sql(query):
    result = subprocess.run(mysql, input=query, env=environment, capture_output=True,
                            encoding='utf-8', errors='strict', timeout=20)
    if result.returncode:
        raise RuntimeError(result.stderr)
    return result.stdout.strip()

def app(lines):
    result = subprocess.run([str(exe), '--config', str(config_path)], input='\n'.join(lines) + '\n',
                            env=environment, capture_output=True, encoding='utf-8', errors='strict', timeout=20)
    if result.returncode:
        raise RuntimeError(result.stderr)
    return result.stdout + result.stderr

sql((project / 'sql/setup.sql').read_text(encoding='utf-8'))
before = sql('SELECT COUNT(*) FROM hrms.workers;')
existing_employee = sql('SELECT COUNT(*) FROM hrms.employee;')
test_id = 1800000000 + secrets.randbelow(100000000)
while sql(f'SELECT COUNT(*) FROM hrms.workers WHERE id={test_id};') != '0':
    test_id = 1800000000 + secrets.randbelow(100000000)
nonce = secrets.token_hex(6)
original = '__codex_test_' + nonce + '_张三'
changed = '__codex_test_' + nonce + '_李四'
external = '__codex_test_' + nonce + '_王五'
passed = 0
try:
    output = app(['1', '1', str(test_id), original, '1', '0'])
    assert '成功添加 1' in output
    passed += 1
    assert sql(f'SELECT name FROM hrms.workers WHERE id={test_id};') == original
    passed += 1
    output = app(['4', str(test_id), str(test_id), changed, '2', '0'])
    assert '修改成功' in output
    passed += 1
    assert sql(f'SELECT CONCAT(name, ":", dept_id) FROM hrms.workers WHERE id={test_id};') == changed + ':2'
    passed += 1
    sql(f"UPDATE hrms.workers SET name='{external}', dept_id=3 WHERE id={test_id} AND name='{changed}';")
    output = app(['5', '1', str(test_id), '0'])
    assert external in output and '岗位:老板' in output
    passed += 1
    output = app(['3', str(test_id), '1', '0'])
    assert '删除成功' in output
    passed += 1
    assert sql(f'SELECT COUNT(*) FROM hrms.workers WHERE id={test_id};') == '0'
    passed += 1
finally:
    # Exact random name ownership protects unrelated rows if someone edits concurrently.
    sql(f"DELETE FROM hrms.workers WHERE id={test_id} AND name IN ('{original}','{changed}','{external}');")
assert sql('SELECT COUNT(*) FROM hrms.workers;') == before
assert sql('SELECT COUNT(*) FROM hrms.employee;') == existing_employee
passed += 2
print(f'PASS: {passed} real hrms CRUD checks; temporary test record removed; original employee table preserved.')
print('Production table: hrms.workers; endpoint: 127.0.0.1:3306; MySQL 8.0.32')
