<#
    Verrijkt manifest.json met modbus-adressen uit WPM-isg-modbus.csv.
    Maakt eerst een timestamp-backup van manifest.json.
    Ondersteunt -WhatIf voor een droge run.
#>

param(
    [switch]$WhatIf
)

$manifestPath = ".\manifest.json"
$csvPath      = ".\WPM-isg-modbus.csv"

Write-Host ""
Write-Host "=== MANIFEST PATCH SCRIPT ==="
Write-Host "Manifest : $manifestPath"
Write-Host "CSV      : $csvPath"
Write-Host "WhatIf   : $WhatIf"
Write-Host ""

# --- 1. Manifest inlezen ---
if (-not (Test-Path $manifestPath)) {
    Write-Host "FOUT: manifest.json niet gevonden!"
    exit 1
}

$manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json

# --- 2. Backup maken ---
$timestamp = (Get-Date).ToString("yyyy-MM-dd_HH-mm-ss")
$backupName = "manifest-$timestamp.ps1"

Write-Host "Backup maken: $backupName"

if (-not $WhatIf) {
    Copy-Item $manifestPath $backupName
}

# --- 3. CSV inlezen ---
$map = Import-Csv $csvPath -Delimiter ';'

Write-Host ""
Write-Host "=== PATCHEN VAN MODBUS VELDEN ==="

$patched = 0
$skipped = 0

foreach ($row in $map) {

    $webid = $row.webid
    $addr  = $row.modbus

    if ($manifest.PSObject.Properties.Name -contains $webid) {

        Write-Host "PATCH: webid $webid → modbus.address = $addr"

        if (-not $WhatIf) {
            $entry = $manifest.$webid

            # modbus object invullen
            $entry.modbus.address = [int]$addr
            $entry.modbus.type    = "input_register"   # eventueel aanpassen
            $entry.modbus.span    = 1                  # eventueel aanpassen
        }

        $patched++
    }
    else {
        Write-Host "SKIP: webid $webid bestaat niet in manifest"
        $skipped++
    }
}

Write-Host ""
Write-Host "Samenvatting:"
Write-Host "  Gepatcht : $patched"
Write-Host "  Overgeslagen : $skipped"

# --- 4. Manifest terugschrijven ---
if (-not $WhatIf) {
    $manifest | ConvertTo-Json -Depth 50 | Set-Content $manifestPath
    Write-Host "`nManifest bijgewerkt."
}
else {
    Write-Host "`n[WHATIF] Geen wijzigingen geschreven."
}

Write-Host ""
