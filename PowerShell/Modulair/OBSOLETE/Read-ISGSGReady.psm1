function Read-ISGSGReady {
    param(
        [hashtable]$Mapping = (Get-ISGMapping)
    )

    if (-not $Mapping) {
        throw "Mapping ontbreekt. Zorg dat Shared.psm1 geladen is en Get-ISGMapping beschikbaar is."
    }

    #
    # 1. Selecteer alle SGReady-registers op basis van Category="SGReady"
    #
    $addresses = $Mapping.Keys |
        Where-Object { $Mapping[$_].Category -eq "SGReady" } |
        Sort-Object

    if ($addresses.Count -eq 0) {
        Write-Warning "Geen SGReady-registers gevonden in de mapping."
        return @()
    }

    #
    # 2. Lees alle SGReady-registers
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