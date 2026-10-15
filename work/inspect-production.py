import configparser
import os
import subprocess
from pathlib import Path

config_path = Path('outputs/EmployeeManagementMySQL/database.ini').resolve()
cfg = configparser.ConfigParser(interpolation=None)
cfg.read(config_path, encoding='utf-8-sig')
db = cfg['mysql']
environment = {key.upper(): value for key, value in os.environ.items()}
environment['MYSQL_PWD'] = db.get('password', '')
command = [r'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe', '--no-defaults', '--protocol=TCP',
           '--host=' + db['host'], '--port=' + db['port'], '--user=' + db['user'],
           '--default-character-set=utf8mb4', '--batch', '--skip-column-names', '--connect-timeout=5']
sql = "SHOW CREATE TABLE hrms.employee; SELECT COUNT(*) FROM hrms.employee;"
result = subprocess.run(command + ['-e', sql], env=environment, capture_output=True, encoding='utf-8', errors='replace', timeout=15)
print('Production connection status:', result.returncode)
print(result.stdout)
if result.returncode:
    print(result.stderr)
raise SystemExit(result.returncode)
