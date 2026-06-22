# Elevated: restart the Windows audio engine to clear the broken capture state
# (E_HANDLE / CLASSNOTREG on the mic). Writes a marker when done so the caller knows.
$marker = "$env:TEMP\mic_fix_done.txt"
Remove-Item $marker -ErrorAction SilentlyContinue
try {
    Restart-Service AudioEndpointBuilder -Force -ErrorAction Stop  # restarts Audiosrv too
    Start-Sleep -Seconds 2
    Set-Content -Path $marker -Value "OK"
} catch {
    Set-Content -Path $marker -Value ("FAIL: " + $_.Exception.Message)
}
