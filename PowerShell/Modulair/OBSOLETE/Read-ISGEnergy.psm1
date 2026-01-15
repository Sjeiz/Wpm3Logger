function Read-ISGEnergy {
    param(
        [hashtable]$Mapping = (Get-ISGMapping)
    )

    if (-not $Mapping) {
        throw "Mapping ontbreekt. Zorg dat ISGMapping.psm1 geladen is en Get-ISGMapping beschikbaar is."
    }

    #
    # 1. Selecteer registers op basis van Category="Energy"
    #
    $addresses = $Mapping.Keys |
        Where-Object { $Mapping[$_].Category -eq "Energy" } |
        Sort-Object

    if ($addresses.Count -eq 0) {
        Write-Warning "Geen Energy-registers gevonden in de mapping."
        return @()
    }

    #
    # 2. Lees alleen de Energy-registers
    #
    $raw = Read-ISGRegisters -Addresses $addresses -Mapping $Mapping

    #
    # 3. Decodeer Energy-registers
    #
    $result = foreach ($reg in $addresses) {
        $decoded = Decode-ISGRegisterValue -Reg $reg -Value $raw[$reg] -AllValues $raw -Mapping $Mapping

        [pscustomobject]@{
            Register = $reg
            Name     = $Mapping[$reg].Name
            RawValue = $raw[$reg]
            Decoded  = $decoded
        }
    }

    #
    # 4. Calculated fields toevoegen (vereist volledige dataset)
    #
    $full = Read-ISGAll
    $full = Add-CalculatedFields -Data $full

    $calc = $full | Where-Object { $_.Register -eq $null }

    #
    # 5. Combineer Energy + Calculated
    #
    return @($result) + @($calc)
}