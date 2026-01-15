# =====================================================================
#  ISGReaders.psm1
#  Alle readers in één module
#  Architectuur: volledig mapping-gedreven, future-proof
# =====================================================================


# =====================================================================
# 1. Read-ISGAll  (de motor)
# =====================================================================
function Read-ISGAll {
    param(
        [string]$Ip,
        [int]$Port = 502,
        [int]$SlaveId = 1
    )

    $mapping = Get-ISGMapping
    $raw = Read-ISGRegisters -Addresses $mapping.Keys -Mapping $mapping

    # Nodig voor 32-bit reconstructie
    $script:RawRegisters = $raw

    $result = @()

    foreach ($reg in $mapping.Keys | Sort-Object) {
        $def = $mapping[$reg]

        if ($def.ContainsKey("Show") -and -not $def.Show) {
            continue
        }

        $value = $raw[$reg]
        $decoded = Decode-ISGRegisterValue -Reg $reg -Value $value -Raw $raw -Mapping $mapping

        if (-not $decoded) { continue }

        $result += [PSCustomObject]@{
            Register = $reg
            Name     = $def.Name
            RawValue = $value
            Decoded  = $decoded
        }
    }

    $result = Add-CalculatedFields -Data $result
    return $result
}


# =====================================================================
# 2. Read-ISGSystemValues
# =====================================================================
function Read-ISGSystemValues {
    param([hashtable]$Mapping = (Get-ISGMapping))

    $addresses = $Mapping.Keys |
        Where-Object { $Mapping[$_].Category -eq "System" } |
        Sort-Object

    if ($addresses.Count -eq 0) { return @() }

    $raw = Read-ISGRegisters -Addresses $addresses -Mapping $Mapping

    foreach ($reg in $addresses) {
        $decoded = Decode-ISGRegisterValue -Reg $reg -Value $raw[$reg] -AllValues $raw -Mapping $Mapping

        [PSCustomObject]@{
            Register = $reg
            Name     = $Mapping[$reg].Name
            RawValue = $raw[$reg]
            Decoded  = $decoded
        }
    }
}


# =====================================================================
# 3. Read-ISGParameters
# =====================================================================
function Read-ISGParameters {
    param([hashtable]$Mapping = (Get-ISGMapping))

    $addresses = $Mapping.Keys |
        Where-Object { $Mapping[$_].Category -eq "Parameters" } |
        Sort-Object

    if ($addresses.Count -eq 0) { return @() }

    $raw = Read-ISGRegisters -Addresses $addresses -Mapping $Mapping

    foreach ($reg in $addresses) {
        $decoded = Decode-ISGRegisterValue -Reg $reg -Value $raw[$reg] -AllValues $raw -Mapping $Mapping

        [PSCustomObject]@{
            Register = $reg
            Name     = $Mapping[$reg].Name
            RawValue = $raw[$reg]
            Decoded  = $decoded
        }
    }
}


# =====================================================================
# 4. Read-ISGStatus
# =====================================================================
function Read-ISGStatus {
    param([hashtable]$Mapping = (Get-ISGMapping))

    $addresses = $Mapping.Keys |
        Where-Object { $Mapping[$_].Category -eq "Status" } |
        Sort-Object

    if ($addresses.Count -eq 0) { return @() }

    $raw = Read-ISGRegisters -Addresses $addresses -Mapping $Mapping

    foreach ($reg in $addresses) {
        $decoded = Decode-ISGRegisterValue -Reg $reg -Value $raw[$reg] -AllValues $raw -Mapping $Mapping

        [PSCustomObject]@{
            Register = $reg
            Name     = $Mapping[$reg].Name
            RawValue = $raw[$reg]
            Decoded  = $decoded
        }
    }
}


# =====================================================================
# 5. Read-ISGEnergy  (met calculated fields)
# =====================================================================
function Read-ISGEnergy {
    param([hashtable]$Mapping = (Get-ISGMapping))

    $addresses = $Mapping.Keys |
        Where-Object { $Mapping[$_].Category -eq "Energy" } |
        Sort-Object

    if ($addresses.Count -eq 0) { return @() }

    $raw = Read-ISGRegisters -Addresses $addresses -Mapping $Mapping

    $result = foreach ($reg in $addresses) {
        $decoded = Decode-ISGRegisterValue -Reg $reg -Value $raw[$reg] -AllValues $raw -Mapping $Mapping

        [PSCustomObject]@{
            Register = $reg
            Name     = $Mapping[$reg].Name
            RawValue = $raw[$reg]
            Decoded  = $decoded
        }
    }

    # Calculated fields vereisen volledige dataset
    $full = Read-ISGAll
    $full = Add-CalculatedFields -Data $full
    $calc = $full | Where-Object { $_.Register -eq $null }

    return @($result) + @($calc)
}


# =====================================================================
# 6. Read-ISGRuntime
# =====================================================================
function Read-ISGRuntime {
    param([hashtable]$Mapping = (Get-ISGMapping))

    $addresses = $Mapping.Keys |
        Where-Object { $Mapping[$_].Category -eq "Runtime" } |
        Sort-Object

    if ($addresses.Count -eq 0) { return @() }

    $raw = Read-ISGRegisters -Addresses $addresses -Mapping $Mapping

    foreach ($reg in $addresses) {
        $decoded = Decode-ISGRegisterValue -Reg $reg -Value $raw[$reg] -AllValues $raw -Mapping $Mapping

        [PSCustomObject]@{
            Register = $reg
            Name     = $Mapping[$reg].Name
            RawValue = $raw[$reg]
            Decoded  = $decoded
        }
    }
}


# =====================================================================
# 7. Read-ISGHP
# =====================================================================
function Read-ISGHP {
    param([hashtable]$Mapping = (Get-ISGMapping))

    $addresses = $Mapping.Keys |
        Where-Object { $Mapping[$_].Category -eq "HP" } |
        Sort-Object

    if ($addresses.Count -eq 0) { return @() }

    $raw = Read-ISGRegisters -Addresses $addresses -Mapping $Mapping

    foreach ($reg in $addresses) {
        $decoded = Decode-ISGRegisterValue -Reg $reg -Value $raw[$reg] -AllValues $raw -Mapping $Mapping

        [PSCustomObject]@{
            Register = $reg
            Name     = $Mapping[$reg].Name
            RawValue = $raw[$reg]
            Decoded  = $decoded
        }
    }
}


# =====================================================================
# 8. Read-ISGSGReady
# =====================================================================
function Read-ISGSGReady {
    param([hashtable]$Mapping = (Get-ISGMapping))

    $addresses = $Mapping.Keys |
        Where-Object { $Mapping[$_].Category -eq "SGReady" } |
        Sort-Object

    if ($addresses.Count -eq 0) { return @() }

    $raw = Read-ISGRegisters -Addresses $addresses -Mapping $Mapping

    foreach ($reg in $addresses) {
        $decoded = Decode-ISGRegisterValue -Reg $reg -Value $raw[$reg] -AllValues $raw -Mapping $Mapping

        [PSCustomObject]@{
            Register = $reg
            Name     = $Mapping[$reg].Name
            RawValue = $raw[$reg]
            Decoded  = $decoded
        }
    }
}