param(
    [switch]$WhatIf
)

$schemaPath     = ".\schema.json"
$isgTesterPath  = ".\isg_tester.json"
$outputPath     = ".\manifest.json"

Write-Host ""
Write-Host "=== NIEUW MANIFEST GENEREREN ==="
Write-Host "Schema      : $schemaPath"
Write-Host "ISG Tester  : $isgTesterPath"
Write-Host "Output      : $outputPath"
Write-Host "WhatIf      : $WhatIf"
Write-Host ""

# JSON laden
$schema     = Get-Content $schemaPath    -Raw | ConvertFrom-Json
$isgTester  = Get-Content $isgTesterPath -Raw | ConvertFrom-Json

# --- FIX: DIRECTE toegang tot de key ---
$schemaProps  = $schema.patternProperties.'^[0-9]+$'.properties
$schemaFields = $schemaProps.PSObject.Properties.Name

Write-Host "Schema velden:"
$schemaFields | ForEach-Object { Write-Host "  - $_" }
Write-Host ""

# Helper: property check
function Has-Prop($obj, $name) {
    return $obj.PSObject.Properties.Name -contains $name
}

$newManifest = [ordered]@{}
$addedCount = 0

foreach ($prop in $isgTester.PSObject.Properties) {

    $webID = $prop.Name
    $testerEntry = $isgTester.$webID

    Write-Host "[NIEUW] webID $webID wordt opgebouwd"

    $entry = [ordered]@{}

    foreach ($field in $schemaFields) {

        $schemaFieldDef = $schemaProps.$field

        # SUBOBJECT?
        if ($schemaFieldDef.type -contains "object") {

            Write-Host "   - $field is een object → subvelden vullen"

            $subEntry = [ordered]@{}

            foreach ($subField in $schemaFieldDef.properties.PSObject.Properties.Name) {

                if (Has-Prop $testerEntry $subField) {
                    $value = $testerEntry.$subField
                    Write-Host "       - $subField = '$value' (uit isg_tester)"
                    $subEntry[$subField] = $value
                }
                else {
                    Write-Host "       - $subField = null (ontbreekt in isg_tester)"
                    $subEntry[$subField] = $null
                }
            }

            $entry[$field] = $subEntry
        }
        else {
            # GEWOON VELD
            if (Has-Prop $testerEntry $field) {
                $value = $testerEntry.$field
                Write-Host "   - $field = '$value' (uit isg_tester)"
                $entry[$field] = $value
            }
            else {
                Write-Host "   - $field = null (ontbreekt in isg_tester)"
                $entry[$field] = $null
            }
        }
    }

    # id moet gelijk zijn aan webID
    $entry["id"] = [int]$webID
    Write-Host "   - id = $webID (geforceerd)"

    $newManifest[$webID] = $entry
    $addedCount++
}

Write-Host ""
Write-Host "Samenvatting:"
Write-Host "  Aantal webIDs verwerkt : $addedCount"

if ($WhatIf) {
    Write-Host ""
    Write-Host "[WHATIF] Geen manifest geschreven."
}
else {
    $newManifest | ConvertTo-Json -Depth 50 | Set-Content $outputPath
    Write-Host ""
    Write-Host "[KLAAR] Nieuw manifest.json gegenereerd."
}

Write-Host ""
