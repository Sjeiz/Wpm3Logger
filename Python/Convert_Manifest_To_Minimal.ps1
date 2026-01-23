# Pad naar origineel manifest
$inputFile  = "manifest.json"

# Pad naar minimal manifest
$outputFile = "manifest_minimal.json"

# Origineel JSON inladen
$manifest = Get-Content $inputFile -Raw | ConvertFrom-Json

# Nieuw object voor minimal manifest
$minimal = @{}

foreach ($key in $manifest.PSObject.Properties.Name) {
    $entry = $manifest.$key

    # Bouw minimal entry inclusief 'access'
    $minimal[$key] = [ordered]@{
        id       = $entry.id
        name     = $entry.name
        datatype = $entry.datatype
        scaling  = $entry.scaling
        unit     = $entry.unit
        sentinel = $entry.sentinel
        access   = $entry.access
    }
}

# Minified JSON wegschrijven
$minimal | ConvertTo-Json -Depth 5 -Compress | Set-Content $outputFile