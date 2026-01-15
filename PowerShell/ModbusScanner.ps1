function Invoke-Modbus {
    param(
        [string]$IP,
        [int]$Function,
        [int]$Address,
        [int]$Count = 1
    )

    $Port = 502
    [int]$Transaction = Get-Random -Minimum 1 -Maximum 65535
    [int]$UnitID = 1

    $adu = @(
        [byte](($Transaction -shr 8) -band 0xFF),
        [byte]($Transaction -band 0xFF),
        0x00,0x00,
        0x00,0x06,
        [byte]$UnitID,
        [byte]$Function,
        [byte](($Address -shr 8) -band 0xFF),
        [byte]($Address -band 0xFF),
        [byte](($Count -shr 8) -band 0xFF),
        [byte]($Count -band 0xFF)
    )

    try {
        $client = New-Object System.Net.Sockets.TcpClient
        $client.ReceiveTimeout = 400
        $client.SendTimeout = 400
        $client.Connect($IP, $Port)

        $stream = $client.GetStream()
        $stream.Write($adu, 0, $adu.Length)

        $buffer = New-Object byte[] 256
        $read = $stream.Read($buffer, 0, 256)

        $client.Close()

        if ($read -lt 1) { return "timeout" }

        $fc = $buffer[7]

        if ($fc -eq ($Function + 0x80)) {
            return "exception_$($buffer[8])"
        }

        return "ok"
    }
    catch {
        return "timeout"
    }
}

function Scan-ModbusRegisters {
    param(
        [string]$IP,
        [int]$Function,
        [int]$Start = 0,
        [int]$End = 9999
    )

    $results = @()

    foreach ($addr in $Start..$End) {
        $resp = Invoke-Modbus -IP $IP -Function $Function -Address $addr

        switch ($resp) {
            "ok" {
                Write-Host "Geldig register: $addr" -ForegroundColor Green
                $results += $addr
            }
            "exception_2" {
                # Illegal Data Address → bestaat niet
            }
            "timeout" {
                # geen reactie → functie niet ondersteund of device busy
            }
        }
    }

    return $results
}

$validHR = Scan-ModbusRegisters -IP "192.168.12.117" -Function 3 -Start 0 -End 9999

$validIR = Scan-ModbusRegisters -IP "192.168.12.117" -Function 4 -Start 0 -End 9999