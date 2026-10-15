from pathlib import Path
import hashlib
import zipfile

workspace = Path.cwd()
project = workspace / 'outputs/EmployeeManagementMySQL'
archive = workspace / 'outputs/职工管理系统_MySQL_VS2022.zip'
example = (project / 'database.example.ini').read_bytes()
with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as package:
    for path in sorted(project.rglob('*')):
        if not path.is_file():
            continue
        relative = path.relative_to(project)
        if any(part in ('build', '.vs', '__pycache__', 'Debug') for part in relative.parts):
            continue
        if path.suffix.lower() in ('.pdb', '.obj', '.log', '.tlog', '.ilk', '.idb', '.user'):
            continue
        target = (Path(project.name) / relative).as_posix()
        if path.name == 'database.ini':
            package.writestr(target, example)
        else:
            package.write(path, target)
with zipfile.ZipFile(archive) as package:
    if package.testzip() is not None:
        raise RuntimeError('ZIP integrity failure')
    for name in package.namelist():
        if name.endswith('/database.ini'):
            contents = package.read(name).decode('utf-8-sig')
            password_lines = [line for line in contents.splitlines() if line.startswith('password=')]
            if password_lines != ['password=']:
                raise RuntimeError('Published config contains password')
    required = ['EmployeeManagement.sln', 'EmployeeManagement.vcxproj', 'database.cpp', 'sql/setup.sql',
                'bin/x64/Release/EmployeeManagement.exe', 'bin/x64/Release/libmysql.dll', 'README.md',
                'third_party/mysql/LICENSE', 'tests/database_tests.cpp']
    for item in required:
        if project.name + '/' + item not in package.namelist():
            raise RuntimeError('Missing deliverable: ' + item)
    extract = workspace / 'work/打包验证 项目'
    extract.mkdir(parents=True, exist_ok=True)
    package.extractall(extract)
    print('Verified entries:', len(package.namelist()))
print('Archive size (MiB):', round(archive.stat().st_size / 1048576, 2))
print('SHA256:', hashlib.sha256(archive.read_bytes()).hexdigest())
print('ZIP password checks passed; original local config retained.')
