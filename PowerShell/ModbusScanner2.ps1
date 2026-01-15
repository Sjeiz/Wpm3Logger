# Volledig PowerShell‑script: automatische blokdetectie

param(
    [string]$IP = "192.168.12.117",
    [int]$StartAddress = 0,
    [int]$EndAddress = 9999
)

Write-Host "Scanning Modbus/TCP device op $IP, adressen $StartAddress..$EndAddress" -ForegroundColor Cyan

# -----------------------------
# Modbus request-functie
# -----------------------------
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

    # Modbus TCP ADU samenstellen
    $adu = @(
        [byte](($Transaction -shr 8) -band 0xFF),
        [byte]($Transaction -band 0xFF),
        0x00,0x00,          # Protocol ID
        0x00,0x06,          # Length
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

        # Exception als FC = gevraagde FC + 0x80
        if ($fc -eq ([byte]($Function + 0x80))) {
            $ex = $buffer[8]
            return "exception_$ex"
        }

        return "ok"
    }
    catch {
        return "timeout"
    }
}

# -----------------------------
# Scannen van één functiecode
# -----------------------------
function Scan-FunctionCode {
    param(
        [string]$IP,
        [int]$Function,
        [int]$Start,
        [int]$End
    )

    $validAddresses = New-Object System.Collections.Generic.List[int]

    Write-Host "`n--- Scannen FC=$Function ($Start..$End) ---" -ForegroundColor Yellow

    foreach ($addr in $Start..$End) {
        $resp = Invoke-Modbus -IP $IP -Function $Function -Address $addr

        switch ($resp) {
            "ok" {
                Write-Host ("[OK] FC={0} Register={1}" -f $Function,$addr) -ForegroundColor Green
                $validAddresses.Add($addr)
            }
            "timeout" {
                # stil; kan ook betekenen dat FC niet ondersteund wordt
            }
            default {
                # exception_xx -> meestal 02 (illegal address)
            }
        }
    }

    return $validAddresses
}

# -----------------------------
# Blokken vormen uit geldige adressen
# -----------------------------
function Get-AddressBlocks {
    param(
        [System.Collections.Generic.List[int]]$Addresses
    )

    $blocks = @()

    if ($Addresses.Count -eq 0) { return $blocks }

    $sorted = $Addresses | Sort-Object
    $start = $sorted[0]
    $prev = $sorted[0]

    for ($i = 1; $i -lt $sorted.Count; $i++) {
        $current = $sorted[$i]
        if ($current -ne ($prev + 1)) {
            $blocks += [PSCustomObject]@{
                Start = $start
                End   = $prev
                Count = ($prev - $start + 1)
            }
            $start = $current
        }
        $prev = $current
    }

    # laatste blok toevoegen
    $blocks += [PSCustomObject]@{
        Start = $start
        End   = $prev
        Count = ($prev - $start + 1)
    }

    return $blocks
}

# -----------------------------
# Hoofdlogica: FC=3 en FC=4
# -----------------------------
$results = @()

$fcList = 3,4   # 3 = Holding Registers, 4 = Input Registers

foreach ($fc in $fcList) {
    $valid = Scan-FunctionCode -IP $IP -Function $fc -Start $StartAddress -End $EndAddress
    $blocks = Get-AddressBlocks -Addresses $valid

    foreach ($b in $blocks) {
        $results += [PSCustomObject]@{
            FunctionCode = $fc
            Start        = $b.Start
            End          = $b.End
            Count        = $b.Count
        }
    }
}

# -----------------------------
# Resultaat tonen
# -----------------------------
Write-Host "`n=== Overzicht gedetecteerde Modbus-blokken ===" -ForegroundColor Cyan

if ($results.Count -eq 0) {
    Write-Host "Geen geldige registers gevonden in het bereik $StartAddress..$EndAddress" -ForegroundColor Red
} else {
    $results |
        Sort-Object FunctionCode, Start |
        ForEach-Object {
            $fc = $_.FunctionCode
            $s  = $_.Start
            $e  = $_.End
            $c  = $_.Count

            Write-Host ("[FC={0}] {1}–{2}  ({3} registers)" -f $fc,$s,$e,$c) -ForegroundColor White
        }
}

# Optioneel: export naar CSV
# $results | Sort-Object Function