$manifestPath = ".\manifest.json"
$manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json

foreach ($webid in $manifest.PSObject.Properties.Name) {

    $entry = $manifest.$webid

    # skip als geen modbus
    if (-not $entry.modbus) { continue }

    # span ophalen (werkt voor PSCustomObject én Hashtable)
    $span = $entry.modbus.span
    if ($span -eq $null) { $span = $entry.modbus["span"] }

    # datatype ophalen
    $dt = $entry.datatype

    # filter
    if ($span -eq 1 -and $dt -in @("int32", "uint32")) {
        Write-Host "$webid → datatype=$dt, span=$span"
    }
}
