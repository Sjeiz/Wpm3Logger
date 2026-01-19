$manifestPath   = ".\manifest.json"
$schemaPath     = ".\schema.json"
$isgTesterPath  = ".\isg-tester.json"

# Load JSON
$manifest  = Get-Content $manifestPath  -Raw | ConvertFrom-Json
$schema    = Get-Content $schemaPath    -Raw | ConvertFrom-Json
$isgTester = Get-Content $isgTesterPath -Raw | ConvertFrom-Json

# Helper: PSCustomObject → Hashtable
function To-Hash($obj) {
    if ($obj -is [System.Collections.IDictionary]) {
        $h = @{}
        foreach ($k in $obj.Keys) { $h[$k] = To-Hash $obj[$k] }
        return $h
    }
    elseif ($obj -is [System.Collections.IEnumerable] -and $obj -notlike [string]) {
        return @($obj | ForEach-Object { To-Hash $_ })
    }
    else {
        return $obj
    }
}

$manifest  = To-Hash $manifest
$schema    = To-Hash $schema
$isgTester = To-Hash $isgTester

# Deep clone schema for new entries
function Clone-Object($obj) {
    if ($obj -is [System.Collections.IDictionary]) {
        $h = @{}
        foreach ($k in $obj.Keys) { $h[$k] = Clone-Object $obj[$k] }
        return $h
    }
    elseif ($obj -is [System.Collections.IEnumerable] -and $obj -notlike [string]) {
        return @($obj | ForEach-Object { Clone-Object $_ })
    }
    else {
        return $obj
    }
}

# STAP 1: zorg dat alle webIDs uit isg-tester in manifest staan
foreach ($webID in $isgTester.Keys) {

    if (-not $manifest.ContainsKey($webID)) {
        # Nieuwe entry op basis van schema
        $manifest[$webID] = Clone-Object $schema
        $manifest[$webID]["webID"] = $webID
    }
}

# Schrijf terug
$manifest | ConvertTo-Json -Depth 20 | Set-Content $manifestPath

Write-Host "STAP 1 klaar: alle webIDs uit isg-tester staan nu in manifest."
