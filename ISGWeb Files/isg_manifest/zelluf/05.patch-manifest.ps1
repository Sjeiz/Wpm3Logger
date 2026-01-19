<#
.SYNOPSIS
    Patcht manifest.json met waarden uit loxwiki-extra-webids.json.

.DESCRIPTION
    - Houdt attribute-volgorde in manifest 100% intact.
    - Alleen bij wijzigingen (~) wordt oud → nieuw getoond.
    - Gelijke waarden (=) tonen alleen "blijft gelijk".
    - Nieuwe velden (!) tonen alleen waarschuwing.
    - Negeert 'bits', 'webid', 'type'.
    - Negeert typetext '' en '0 bis 255'.
    - Negeert description '-'.
    - Als description bestaat en name leeg is → name = description.
    - multiplier → scaling (fallback 1).
    - typetext → datatype (lange → korte mapping).
    - modbus.type mapping:
         "R/W Holding" → "holding_register"
         "Read Input"  → "input_register"
    - modbus.type wordt OVERRULED door address:
         4xxxx → holding_register
         3xxxx → input_register
    - modbus-subvelden blijven altijd behouden.
    - modbus-diff wordt getoond als compacte JSON.
    - Kleurcodes: groen (=), oranje (~), rood (!).

.NOTES
    Auteur  : Erik + Copilot
    Versie  : 2026-01-16
#>

param(
    [switch]$WhatIf
)

$manifestPath = ".\manifest.json"
$updatePath   = ".\loxwiki-extra-webids.json"

Write-Host ""
Write-Host "=== JOIN & PATCH SCRIPT ==="
Write-Host "Manifest : $manifestPath"
Write-Host "Updates  : $updatePath"
Write-Host "WhatIf   : $WhatIf"
Write-Host ""

# --- 1. Inlezen ---
$manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
$updates  = Get-Content $updatePath  -Raw | ConvertFrom-Json

# --- 2. Normaliseren naar dictionary ---
if ($updates -isnot [System.Collections.IDictionary]) {
    $tmp = @{}
    foreach ($item in $updates) {
        if ($item.webid) {
            $tmp["$($item.webid)"] = $item
        }
    }
    $updates = $tmp
}

Write-Host "Aantal update-entries: $($updates.Keys.Count)"
Write-Host ""

# --- 3. Backup ---
$timestamp = (Get-Date).ToString("yyyy-MM-dd_HH-mm-ss")
$backupName = "manifest-$timestamp.json"

if (-not $WhatIf) {
    Copy-Item $manifestPath $backupName
    Write-Host "Backup gemaakt: $backupName"
}
else {
    Write-Host "[WHATIF] Backup wordt niet gemaakt"
}

Write-Host ""
Write-Host "=== PATCH VOORBEREIDING ==="

$patched = 0
$skipped = 0

# mapping lange → korte typetext
$typeMap = @{
    "signed16bit"     = "int16"
    "16bitsigned"     = "int16"
    "int16"           = "int16"
    "unsigned16bit"   = "uint16"
    "16bitunsigned"   = "uint16"
    "uint16"          = "uint16"
    "signed32bit"     = "int32"
    "32bitsigned"     = "int32"
    "int32"           = "int32"
    "unsigned32bit"   = "uint32"
    "32bitunsigned"   = "uint32"
    "uint32"          = "uint32"
}

# mapping modbus type
$modbusTypeMap = @{
    "r/wholding" = "holding_register"
    "readinput"  = "input_register"
}

foreach ($webid in $updates.Keys) {

    if (-not ($manifest.PSObject.Properties.Name -contains $webid)) {
        Write-Host "SKIP: webid $webid bestaat niet in manifest" -ForegroundColor Red
        $skipped++
        continue
    }

    $current = $manifest.$webid
    $patch   = $updates.$webid

    Write-Host ""
    Write-Host ">>> WEBID $webid"

    # --- 4. Diff tonen ---
    foreach ($field in $patch.PSObject.Properties.Name) {

        if ($field -in @("bits","webid","type")) { continue }

        # --- MODBUS ---
        if ($field -eq "modbus") {

            $oldModbus = $current.modbus
            $oldJson = if ($oldModbus) { $oldModbus | ConvertTo-Json -Compress } else { "{}" }

            # bepaal address
            $address = $patch.modbus.address
            if (-not $address) { $address = $current.modbus.address }

            $newType = $null

            if ($address) {
                $addrStr = "$address"
                if ($addrStr.StartsWith("4")) { $newType = "holding_register" }
                elseif ($addrStr.StartsWith("3")) { $newType = "input_register" }
            }

            # fallback naar LoxWiki mapping
            if (-not $newType -and $patch.modbus.type) {
                $raw = $patch.modbus.type.ToLower()
                $norm = ($raw -replace "[\s\-_]", "")
                if ($modbusTypeMap.ContainsKey($norm)) {
                    $newType = $modbusTypeMap[$norm]
                }
                else {
                    $newType = $patch.modbus.type
                }
            }

            # fallback naar bestaand type
            if (-not $newType -and $oldModbus.type) {
                $newType = $oldModbus.type
            }

            # nieuwe modbus-object
            $newModbus = @{}
            if ($oldModbus) {
                foreach ($p in $oldModbus.PSObject.Properties) {
                    $newModbus[$p.Name] = $p.Value
                }
            }
            $newModbus["type"] = $newType

            $newJson = ($newModbus | ConvertTo-Json -Compress)

            if ($oldJson -eq $newJson) {
                Write-Host "  =  modbus : blijft gelijk" -ForegroundColor Green
            }
            else {
                Write-Host "  ~  modbus : $oldJson → $newJson" -ForegroundColor DarkYellow
            }

            continue
        }

        # --- multiplier → scaling ---
        if ($field -eq "multiplier") {
            $mappedField = "scaling"
            $new = $patch.multiplier
            if ($new -eq "-" -or $new -eq $null) { $new = 1 }
        }

        # --- typetext → datatype ---
        elseif ($field -eq "typetext") {

            if (-not $patch.typetext -or $patch.typetext.Trim() -eq "") { continue }
            if ($patch.typetext.Trim().ToLower() -eq "0 bis 255") { continue }

            $mappedField = "datatype"

            $raw = $patch.typetext.ToLower()
            $norm = ($raw -replace "[\s\-_]", "")

            if ($typeMap.ContainsKey($norm)) {
                $new = $typeMap[$norm]
            }
            else {
                $new = $raw
                Write-Host "  !  Onbekende typetext '$($patch.typetext)' → patch wordt toch toegepast" -ForegroundColor Red
            }
        }

        # --- description ---
        elseif ($field -eq "description") {

            if ($patch.description -eq "-" -or -not $patch.description) { continue }

            $mappedField = "description"
            $new = $patch.description
        }

        else {
            $mappedField = $field
            $new = $patch.$field
        }

        # oude waarde ophalen
        $prop = $current.PSObject.Properties[$mappedField]
        $old = if ($prop) { $prop.Value } else { $null }

        if ($old -eq $new) {
            Write-Host "  =  $mappedField : blijft gelijk ($old)" -ForegroundColor Green
            continue
        }

        Write-Host "  ~  $mappedField : $old → $new" -ForegroundColor DarkYellow
    }

    # --- 5. PATCH TOEPASSEN ---
    if (-not $WhatIf) {

        $obj = $manifest.$webid

        foreach ($field in $patch.PSObject.Properties.Name) {

            if ($field -in @("bits","webid","type")) { continue }

            # --- MODBUS ---
            if ($field -eq "modbus") {

                if (-not $obj.modbus) {
                    $obj | Add-Member -NotePropertyName "modbus" -NotePropertyValue ([pscustomobject]@{})
                }

                $address = $patch.modbus.address
                if (-not $address) { $address = $obj.modbus.address }

                $newType = $null

                if ($address) {
                    $addrStr = "$address"
                    if ($addrStr.StartsWith("4")) { $newType = "holding_register" }
                    elseif ($addrStr.StartsWith("3")) { $newType = "input_register" }
                }

                if (-not $newType -and $patch.modbus.type) {
                    $raw = $patch.modbus.type.ToLower()
                    $norm = ($raw -replace "[\s\-_]", "")
                    if ($modbusTypeMap.ContainsKey($norm)) {
                        $newType = $modbusTypeMap[$norm]
                    }
                    else {
                        $newType = $patch.modbus.type
                    }
                }

                if (-not $newType -and $obj.modbus.type) {
                    $newType = $obj.modbus.type
                }

                if ($obj.modbus.PSObject.Properties["type"]) {
                    $obj.modbus.PSObject.Properties["type"].Value = $newType
                }
                else {
                    $obj.modbus | Add-Member -NotePropertyName "type" -NotePropertyValue $newType
                }

                continue
            }

            # --- multiplier → scaling ---
            if ($field -eq "multiplier") {
                $mappedField = "scaling"
                $value = $patch.multiplier
                if ($value -eq "-" -or $value -eq $null) { $value = 1 }

                if ($obj.PSObject.Properties[$mappedField]) {
                    $obj.PSObject.Properties[$mappedField].Value = $value
                }
                else {
                    $obj | Add-Member -NotePropertyName $mappedField -NotePropertyValue $value
                }

                continue
            }

            # --- typetext → datatype ---
            if ($field -eq "typetext") {

                if (-not $patch.typetext -or $patch.typetext.Trim() -eq "") { continue }
                if ($patch.typetext.Trim().ToLower() -eq "0 bis 255") { continue }

                $mappedField = "datatype"

                $raw = $patch.typetext.ToLower()
                $norm = ($raw -replace "[\s\-_]", "")

                if ($typeMap.ContainsKey($norm)) {
                    $value = $typeMap[$norm]
                }
                else {
                    $value = $raw
                }

                if ($obj.PSObject.Properties[$mappedField]) {
                    $obj.PSObject.Properties[$mappedField].Value = $value
                }
                else {
                    $obj | Add-Member -NotePropertyName $mappedField -NotePropertyValue $value
                }

                continue
            }

            # --- description ---
            if ($field -eq "description") {

                if ($patch.description -eq "-" -or -not $patch.description) { continue }

                $value = $patch.description

                if ($obj.PSObject.Properties["description"]) {
                    $obj.PSObject.Properties["description"].Value = $value
                }
                else {
                    $obj | Add-Member -NotePropertyName "description" -NotePropertyValue $value
                }

                # name vullen indien leeg
                if (-not $obj.name -or $obj.name.Trim() -eq "") {
                    if ($obj.PSObject.Properties["name"]) {
                        $obj.PSObject.Properties["name"].Value = $value
                    }
                    else {
                        $obj | Add-Member -NotePropertyName "name" -NotePropertyValue $value
                    }
                }

                continue
            }

            # --- normale velden ---
            $mappedField = $field
            $value = $patch.$field

            if ($obj.PSObject.Properties[$mappedField]) {
                $obj.PSObject.Properties[$mappedField].Value = $value
            }
            else {
                $obj | Add-Member -NotePropertyName $mappedField -NotePropertyValue $value
            }
        }
    }

    $patched++
}

Write-Host ""
Write-Host "Samenvatting:"
Write-Host "  Gepatcht     : $patched"
Write-Host "  Overgeslagen : $skipped"

if (-not $WhatIf) {
    $manifest | ConvertTo-Json -Depth 50 | Set-Content $manifestPath
    Write-Host "`nManifest bijgewerkt."
}
else {
    Write-Host "`n[WHATIF] Geen wijzigingen geschreven."
}

Write-Host ""
