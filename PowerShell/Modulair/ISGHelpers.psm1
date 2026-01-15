# Shared.psm1
# ---------------------------------------------------------
# 1. Helpers
# ---------------------------------------------------------

function To-Int16 {
    param([int]$Value)
    if ($Value -ge 32768) { return $Value - 65536 }
    return $Value
}

function Fix-HashtableKeys {
    param([hashtable]$h)

    $new = @{}
    foreach ($k in $h.Keys) {
        $clean = $k.ToString().Trim().Replace([string][char]0xFEFF, "")
        if ($clean -as [int]) {
            $new[[int]$clean] = $h[$k]
        }
        else {
            $new[$clean] = $h[$k]
        }
    }
    return $new
}

function Decode-ScaledValue {
    param(
        [hashtable]$Def,
        [double]$Value
    )

    # N/A markers
    if ($Value -eq 32768 -or $Value -eq 36864 -or $Value -eq $null) {
        return "N/A"
    }

    $scaled = $Value * $Def.Scale
    if ($Def.Unit -ne "") {
        return "$scaled $($Def.Unit)"
    }
    return "$scaled"
}

function Decode-SGReady {
    param([int]$Enabled, [int]$In1, [int]$In2)

    if ($Enabled -eq 0 -or $Enabled -eq 32768 -or $Enabled -eq 36864) { return "Disabled" }

    $key = "${In1},${In2}"
    $modes = @{
        "0,0" = "Automatic"
        "1,0" = "Accelerated"
        "0,1" = "Standby/Defrost"
        "1,1" = "Maximum"
    }

    if ($modes.ContainsKey($key)) { return $modes[$key] }
    return "Unknown SG-Ready state ($key)"
}

# ---------------------------------------------------------
# 2. Type‑gedreven decoder
# ---------------------------------------------------------

function Decode-ISGRegisterValue {
    param(
        [int]$Reg,
        [int]$Value,
        [int[]]$Raw,
        [hashtable]$Mapping
    )

    if (-not $Mapping.ContainsKey($Reg)) {
        if ($Value -eq 32768 -or $Value -eq 36864 -or $Value -eq $null) {
            return "Unknown register $Reg = N/A"
        }
        return "Unknown register $Reg = $Value"
    }

    $d = $Mapping[$Reg]

    # Skip unused registers
    if ($d.ContainsKey("Show") -and -not $d.Show) {
        return $null
    }

    # N/A markers
    if ($Value -eq 32768 -or $Value -eq 36864 -or $Value -eq $null) {
        return "$($d.Name) = N/A"
    }

    switch ($d.Type) {

        "UInt16" {
            $scaled = $Value * $d.Scale
            return "$($d.Name) = $scaled $($d.Unit)"
        }

        "Int16" {
            $v = To-Int16 $Value
            $scaled = $v * $d.Scale
            return "$($d.Name) = $scaled $($d.Unit)"
        }

        "UInt32" {
            $low  = $Value
            $high = $Raw[$Reg + 1]
            $combined = ($high -shl 16) -bor $low
            $scaled = $combined * $d.Scale
            return "$($d.Name) = $scaled $($d.Unit)"
        }

        "Int32" {
            $low  = $Value
            $high = $Raw[$Reg + 1]
            $combined = ($high -shl 16) -bor $low
            $signed = [int32]$combined
            $scaled = $signed * $d.Scale
            return "$($d.Name) = $scaled $($d.Unit)"
        }

        "enum" {
            if ($d.Values.ContainsKey($Value)) {
                return "$($d.Name) = $($d.Values[$Value])"
            }
            return "$($d.Name) = Unknown ($Value)"
        }

        "bitfield" {
            $bits = @()
            foreach ($b in $d.Bits.Keys) {
                if ($Value -band (1 -shl $b)) {
                    $bits += $d.Bits[$b]
                }
            }
            if ($bits.Count -eq 0) { return "$($d.Name) = None" }
            return "$($d.Name) = " + ($bits -join ", ")
        }

        default {
            return "$($d.Name) = $Value"
        }
    }
}

function Add-CalculatedFields {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory, ValueFromPipeline)]
        [System.Collections.IEnumerable]$Data
    )

    # Maak een index per register voor snelle lookup
    $byReg = @{}
    foreach ($row in $Data) {
        if ($null -ne $row.Register) {
            $byReg[$row.Register] = $row
        }
    }

    # Helper om veilig waarde op te halen
    function Get-Val {
        param([int]$Reg)
        if ($byReg.ContainsKey($Reg) -and $byReg[$Reg].PSObject.Properties.Match('Value')) {
            return [double]$byReg[$Reg].Value
        }
        return 0.0
    }

    $calc = @()

    # 1. Totaal geleverde warmte (MWh)
    $calc += [pscustomobject]@{
        Register = $null
        Name     = 'Total Heat All Circuits'
        RawValue = $null
        Value    = (Get-Val 3501) + (Get-Val 3504) + (Get-Val 3506) + (Get-Val 3508)
        Unit     = 'MWh'
    }

    # 2. Totaal elektrisch verbruik (MWh)
    $calc += [pscustomobject]@{
        Register = $null
        Name     = 'Total Power All Circuits'
        RawValue = $null
        Value    = (Get-Val 3511) + (Get-Val 3514)
        Unit     = 'MWh'
    }

    # 3. Totale dagwarmte (kWh)
    $calc += [pscustomobject]@{
        Register = $null
        Name     = 'Total Heat Day All Circuits'
        RawValue = $null
        Value    = (Get-Val 3500) + (Get-Val 3503)
        Unit     = 'kWh'
    }

    # 4. Totale dagconsumptie (kWh)
    $calc += [pscustomobject]@{
        Register = $null
        Name     = 'Total Power Day All Circuits'
        RawValue = $null
        Value    = (Get-Val 3510) + (Get-Val 3513)
        Unit     = 'kWh'
    }

    return @($Data) + $calc
}