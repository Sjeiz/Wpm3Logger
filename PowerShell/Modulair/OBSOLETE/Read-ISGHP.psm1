function Read-ISGHP {
    param(
        [hashtable]$Mapping = (Get-ISGMapping)
    )

    if (-not $Mapping) {
        throw "Mapping ontbreekt. Zorg dat ISGMapping.psm1 geladen is en Get-ISGMapping beschikbaar is."
    }

    #
    # 1. Selecteer HP-registers op basis van Category="HP"
    #
    $addresses = $Mapping.Keys |
        Where-Object { $Mapping[$_].Category -eq "HP" } |
        Sort-Object

    if ($addresses.Count -eq 0) {
        Write-Warning "Geen HP-registers gevonden in de mapping."
        return @()
    }

    #
    # 2. Lees HP-registers
    #
    $raw = Read-ISGRegisters -Addresses $addresses -Mapping $Mapping

    #
    # 3. Decodeer HP-registers
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

    return $result
}