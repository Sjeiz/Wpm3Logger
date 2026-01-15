function Read-ISGRuntime {
    param(
        [hashtable]$Mapping = (Get-ISGMapping)
    )

    if (-not $Mapping) {
        throw "Mapping ontbreekt. Zorg dat Shared.psm1 geladen is en Get-ISGMapping beschikbaar is."
    }

    #
    # 1. Selecteer alle runtime-registers op basis van Category="Runtime"
    #
    $addresses = $Mapping.Keys |
        Where-Object { $Mapping[$_].Category -eq "Runtime" } |
        Sort-Object

    if ($addresses.Count -eq 0) {
        Write-Warning "Geen runtime-registers gevonden in de mapping."
        return @()
    }

    #
    # 2. Lees alle runtime-registers
    #
    $rawValues = Read-ISGRegisters -Addresses $addresses -Mapping $Mapping

    #
    # 3. Decodeer alle runtimes
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