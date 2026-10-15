param([string]$Configuration = 'Release', [string]$Target = 'Build')
$ErrorActionPreference = 'Stop'
$taskInfo = [System.Diagnostics.ProcessStartInfo]::new()
$taskInfo.FileName = 'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe'
$taskInfo.UseShellExecute = $false
$taskInfo.Environment.Clear()
foreach ($taskKey in [System.Environment]::GetEnvironmentVariables('Process').Keys) {
    $taskInfo.Environment[$taskKey.ToUpperInvariant()] = [System.Environment]::GetEnvironmentVariable($taskKey)
}
$taskInfo.ArgumentList.Add('outputs\EmployeeManagementMySQL\EmployeeManagement.sln')
$taskInfo.ArgumentList.Add('/m:1')
$taskInfo.ArgumentList.Add('/nr:false')
$taskInfo.ArgumentList.Add('/p:Configuration=' + $Configuration)
$taskInfo.ArgumentList.Add('/p:Platform=x64')
$taskInfo.ArgumentList.Add('/verbosity:minimal')
$taskInfo.ArgumentList.Add('/t:' + $Target)
$taskProcess = [System.Diagnostics.Process]::Start($taskInfo)
$taskProcess.WaitForExit()
exit $taskProcess.ExitCode
