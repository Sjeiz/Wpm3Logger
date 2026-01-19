<#
    Patch KNX CommObjID op basis van knx-mapping_WPM_SYSTEM.csv.
    Fixes:
      - Eerste regel overslaan (# comment)
      - Hardcoded kolomnamen
      - WebID casten naar int → string
      - Alleen CSV-webIDs patchen (geen manifest-loop)
#>

param(
    [switch]$WhatIf
)

$manifestPath = ".\manifest.json"
$csvPath      = ".\knx-mapping_WPM_SYSTEM.csv"

Write-Host ""
Write-Host "=== MANIFEST KNX PATCH SCRIPT (CSV-driven, hardcoded) ==="
Write-Host ""

# Manifest inlezen
$manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json

# CSV inlezen (skip eerste regel, hardcoded kolomnamen)
$csvLines = Get-Content $csvPath | Select-Object -Skip 1

$csv = foreach ($line in $csvLines) {
    if ($line.Trim() -eq "") { continue }

    $parts = $line.Split(";")

    [PSCustomObject]@{
        CommObjID = $parts[0].Trim()
        WebID     = $parts[1].Trim()
    }
}

# Backup
$timestamp = (Get-Date).ToString("yyyy-MM-dd_HH-mm-ss")
$backupName = "manifest-$timestamp.json"
if (-not $WhatIf) {
    Copy-Item $manifestPath $backupName
}

Write-Host "Backup: $backupName"
Write-Host ""

Write-Host "=== PATCHEN VAN KNX COMMOBJID ==="

$patched = 0
$skipped = 0

foreach ($row in $csv) {

    # WebID normaliseren: int → string
    try {
        $webID = ([int]$row.WebID).ToString()
        $comm  = [int]$row.CommObjID
    }
    catch {
        Write-Host "SKIP: ongeldige CSV-regel → $($row | Out-String)"
        continue
    }

    # Alleen patchen als manifest-entry bestaat
    if ($manifest.PSObject.Properties.Name -contains $webID) {

        Write-Host "PATCH: webid $webID → knx.commObjID = $comm"

        if (-not $WhatIf) {
            $manifest.$webID.knx.commObjID = $comm
        }

        $patched++
    }
    else {
        Write-Host "SKIP: webid $webID niet gevonden in manifest"
        $skipped++
    }
}

Write-Host ""
Write-Host "Samenvatting:"
Write-Host "  Gepatcht     : $patched"
Write-Host "  Overgeslagen : $skipped"

# Manifest terugschrijven
if (-not $WhatIf) {
    $manifest | ConvertTo-Json -Depth 50 | Set-Content $manifestPath
    Write-Host "`nManifest bijgewerkt."
}
else {
    Write-Host "`n[WHATIF] Geen wijzigingen geschreven."
}

Write-Host ""
