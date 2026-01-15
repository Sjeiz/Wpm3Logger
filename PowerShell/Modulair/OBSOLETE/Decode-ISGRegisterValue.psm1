function Decode-ISGRegisterValue {
    param(
        [int]$Reg,
        [int]$Value,
        [hashtable]$Mapping
    )

    # N/A waarden
    if ($Value -eq 32768 -or $Value -eq 36864 -or $Value -eq $null) {
        if ($Mapping.ContainsKey($Reg)) {
            return "$($Mapping[$Reg].Name) = N/A"
        }
        return "Unknown register $Reg = N/A"
    }

    # Bestaat deze mapping?
    if (-not $Mapping.ContainsKey($Reg)) {
        return "Unknown register $Reg = $Value"
    }

    $def = $Mapping[$Reg]

    #
    # 1. 32-bit runtime reconstructie
    #
    switch ($Reg) {
        3538 { return "Compressor Runtime = $((($Value -shl 16) -bor ($Mapping[3539].LastValue))) h" }
        3539 { return "Compressor Runtime = $(((($Mapping[3538].LastValue) -shl 16) -bor $Value)) h" }

        3541 { return "Heating Runtime = $((($Value -shl 16) -bor ($Mapping[3542].LastValue))) h" }
        3542 { return "Heating Runtime = $(((($Mapping[3541].LastValue) -shl 16) -bor $Value)) h" }

        3544 { return "DHW Runtime = $((($Value -shl 16) -bor ($Mapping[3545].LastValue))) h" }
        3545 { return "DHW Runtime = $(((($Mapping[3544].LastValue) -shl 16) -bor $Value)) h" }

        3546 { return "Cooling Runtime = $((($Value -shl 16) -bor ($Mapping[3547].LastValue))) h" }
        3547 { return "Cooling Runtime = $(((($Mapping[3546].LastValue) -shl 16) -bor $Value)) h" }
    }

    #
    # 2. Bitfields
    #
    if ($def.ContainsKey('Type') -and $def.Type -eq 'bitfield') {
        $bits = @()
        foreach ($b in $def.Bits.Keys) {
            if ($Value -band (1 -shl $b)) {
                $bits += $def.Bits[$b]
            }
        }
        if ($bits.Count -eq 0) {
            return "$($def.Name) = None"
        }
        return "$($def.Name) = " + ($bits -join ', ')
    }

    #
    # 3. Enums
    #
    if ($def.ContainsKey('Type') -and $def.Type -eq 'enum') {
        if ($def.Values.ContainsKey($Value)) {
            return "$($def.Name) = $($def.Values[$Value])"
        }
        return "$($def.Name) = Unknown ($Value)"
    }

    #
    # 4. Geschaalde waarden
    #
    if ($def.ContainsKey('Scale')) {
        $scaled = Decode-ScaledValue -Def $def -Value $Value
        return "$($def.Name) = $scaled"
    }

    #
    # 5. Fallback
    #
    return "$($def.Name) = $Value"
}