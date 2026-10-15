@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /utf-8 /I outputs\EmployeeManagementMySQL outputs\EmployeeManagementMySQL\tests\database_tests.cpp /Fe:work\database_tests.exe /Fo:work\database_tests.obj
