# pick random port >= 20000
$port = $(Get-Random -Maximum 45535) + 20000

# Start processes and remember their objects
$server = Start-Process -FilePath .\LobbyServer.exe -ArgumentList $Port -PassThru
$clientA = Start-Process -FilePath .\LobbyClient.exe -ArgumentList "A",$Port -PassThru
$clientB = Start-Process -FilePath .\LobbyClient.exe -ArgumentList "B",$Port -PassThru
$clientC = Start-Process -FilePath .\LobbyClient.exe -ArgumentList "C",$Port -PassThru
$clientD = Start-Process -FilePath .\LobbyClient.exe -ArgumentList "D",$Port -PassThru

echo "Started LobbyServer ($($server.Id):$port) and clients (Ids $($clientA.Id),$($clientB.Id),$($clientC.Id),$($clientD.Id))"

Start-Sleep -Seconds 15

$procs = @($clientA, $clientB, $clientC, $clientD, $server)

foreach ($p in $procs) {
    try {
        if (!$p.HasExited) {
            Stop-Process -Id $p.Id -ErrorAction SilentlyContinue
        }
    } catch {}
}

Start-Sleep -Seconds 1
foreach ($p in $procs) {
    try {
        if (!$p.HasExited) {
            Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
        }
    } catch {}
}
