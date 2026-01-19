<#
    Verrijkt manifest.json met sentinel-waardes en webType uit:
      - WPM_3_isg_objects.json
      - isg_core_objects.json

    Beide bestanden hebben dezelfde structuur → samenvoegen → verwerken.
#>

param(
    [switch]$WhatIf
)

$manifestPath = ".\manifest.json"
$wpmPath      = ".\WPM_3_isg_objects.json"
$corePath     = ".\isg_core_objects.json"

Write-Host ""
Write-Host "=== MANIFEST PATCH SCRIPT (simpel concat model) ==="
Write-Host "Manifest : $manifestPath"
Write-Host "WPM      : $wpmPath"
Write-Host "Core     : $corePath"
Write-Host "WhatIf   : $WhatIf"
Write-Host ""

# --- 1. Manifest inlezen ---
if (-not (Test-Path $manifestPath)) {
    Write-Host "FOUT: manifest-bestand niet gevonden: $manifestPath"
    exit 1
}
$manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json

# --- 2. Beide objectbestanden inlezen ---
if (-not (Test-Path $wpmPath)) {
    Write-Host "FOUT: WPM-objectbestand niet gevonden: $wpmPath"
    exit 1
}
if (-not (Test-Path $corePath)) {
    Write-Host "FOUT: CORE-objectbestand niet gevonden: $corePath"
    exit 1
}

$wpm  = Get-Content $wpmPath  -Raw | ConvertFrom-Json
$core = Get-Content $corePath -Raw | ConvertFrom-Json

# --- 3. Samenvoegen tot één lijst ---
$all = @()
$all += $wpm
$all += $core

# --- 4. Lookup tabel bouwen ---
$lookup = @{}
foreach ($obj in $all) {
    $lookup[[string]$obj.webID] = $obj
}

# --- 5. Backup maken ---
$timestamp = (Get-Date).ToString("yyyy-MM-dd_HH-mm-ss")
$backupName = "manifest-$timestamp.json"

Write-Host "Backup maken: $backupName"
if (-not $WhatIf) {
    Copy-Item $manifestPath $backupName
}

Write-Host ""
Write-Host "=== PATCHEN VAN SENTINEL EN WEBTYPE ==="

$patched = 0
$skipped = 0

foreach ($entryName in $manifest.PSObject.Properties.Name) {

    $key = [string]$entryName

    if ($lookup.ContainsKey($key)) {

        $obj      = $lookup[$key]
        $sentinel = $obj.value
        $webType  = $obj.webType

        Write-Host "PATCH: webid $key → sentinel=$sentinel, webType=$webType"

        if (-not $WhatIf) {
            $manifest.$key.sentinel = $sentinel
            $manifest.$key.webType  = $webType
        }

        $patched++
    }
    else {
        Write-Host "SKIP: webid $key niet gevonden in objectbestanden"
        $skipped++
    }
}

Write-Host ""
Write-Host "Samenvatting:"
Write-Host "  Gepatcht     : $patched"
Write-Host "  Overgeslagen : $skipped"

# --- 6. Manifest terugschrijven ---
if (-not $WhatIf) {
    $manifest | ConvertTo-Json -Depth 50 | Set-Content $manifestPath
    Write-Host "`nManifest bijgewerkt."
}
else {
    Write-Host "`n[WHATIF] Geen wijzigingen geschreven."
}

Write-Host ""
