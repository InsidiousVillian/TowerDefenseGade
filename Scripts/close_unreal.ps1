$procs = Get-Process | Where-Object { $_.Name -match 'UnrealEditor|LiveCoding' }
foreach ($proc in $procs) {
    Write-Output ("Closing {0} ({1})" -f $proc.Name, $proc.Id)
    $proc.CloseMainWindow() | Out-Null
}
Start-Sleep -Seconds 8
$still = Get-Process | Where-Object { $_.Name -match 'UnrealEditor|LiveCoding' }
if ($still) {
    foreach ($proc in $still) {
        Write-Output ("Force stopping {0} ({1})" -f $proc.Name, $proc.Id)
        Stop-Process -Id $proc.Id -Force
    }
}
Write-Output "Unreal processes closed."
