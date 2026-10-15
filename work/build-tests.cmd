@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /utf-8 /MT /W4 /I outputs\EmployeeManagementMySQL /I outputs\EmployeeManagementMySQL\third_party\mysql\include outputs\EmployeeManagementMySQL\tests\database_tests.cpp outputs\EmployeeManagementMySQL\database.cpp /Fe:work\database_tests.exe /Fo:work\ /link /LIBPATH:outputs\EmployeeManagementMySQL\third_party\mysql\lib libmysql.lib
