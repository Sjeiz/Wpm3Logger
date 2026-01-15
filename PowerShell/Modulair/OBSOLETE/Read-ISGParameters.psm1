function Read-ISGParameters {
    param(
        [hashtable]$Mapping = (Get-ISGMapping)
    )

    if (-not $Mapping) {
        throw "Mapping ontbreekt. Zorg dat Shared.psm1 geladen is en Get-ISGMapping beschikbaar is."
    }

    #
    # 1. Selecteer alle parameter-registers op basis van Category="Parameters"
    #
    $addresses = $Mapping.Keys |
        Where-Object { $Mapping[$_].Category -eq "Parameters" } |
        Sort-Object

    if ($addresses.Count -eq 0) {
        Write-Warning "Geen parameter-registers gevonden in de mapping."
        return @()
    }

    #
    # 2. Lees alle parameter-registers
    #
    $rawValues = Read-ISGRegisters -Addresses $addresses -Mapping $Mapping

    #
    # 3. Decodeer alle waarden
    #
    $result = foreach ($reg in $addresses) {
        $raw = $rawValues[$reg]
        $decoded = Decode-ISGRegisterValue -Reg $reg -Value $raw -AllValues $rawValues -Mapping $Mapping

        [pscustomobject]@{
            Register = $reg
            Name     = $Mapping[$reg].Name
            RawValue = $raw
            Decoded  = $decoded
        }
    }

    return $result
}