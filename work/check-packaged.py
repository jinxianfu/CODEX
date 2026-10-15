import os
import subprocess
from pathlib import Path

workspace = Path.cwd()
project = workspace / 'work/打包验证 项目/EmployeeManagementMySQL'
environment = {key.upper(): value for key, value in os.environ.items()}
environment.pop('EMPLOYEE_DB_PASSWORD', None)
program = project / 'bin/x64/Release/EmployeeManagement.exe'
assert subprocess.run([str(program), '--help'], env=environment, capture_output=True, timeout=10).returncode == 0
msbuild = r'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe'
result = subprocess.run([msbuild, 'EmployeeManagement.sln', '/t:Rebuild', '/m:1', '/nr:false',
                         '/p:Configuration=Release', '/p:Platform=x64', '/verbosity:minimal'],
                        cwd=project, env=environment, timeout=60)
assert result.returncode == 0, 'Relocated solution rebuild failed'
test_config = workspace / 'work/mysql-test/test-database.ini'
result = subprocess.run([str(project / 'bin/x64/Release/DatabaseTests.exe'), str(test_config)],
                        env=environment, timeout=15)
assert result.returncode == 0
production_config = workspace / 'outputs/EmployeeManagementMySQL/database.ini'
result = subprocess.run([str(program), '--config', str(production_config)], input='0\n',
                        env=environment, capture_output=True, encoding='utf-8', errors='strict', timeout=15)
assert result.returncode == 0 and '已连接 MySQL：127.0.0.1:3306/hrms' in result.stdout
print('PASS: extracted ZIP rebuilt in a new Unicode/spaced path; database tests passed; packaged app connected to real hrms.')
