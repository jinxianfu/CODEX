import os
import subprocess
from pathlib import Path

env = {key.upper(): value for key, value in os.environ.items()}
project = Path('outputs/EmployeeManagementMySQL').resolve()
result = subprocess.run(['cmd', '/c', 'build.bat', 'Release'], cwd=project, env=env, timeout=60)
print('Packaged build.bat exit:', result.returncode)
raise SystemExit(result.returncode)
