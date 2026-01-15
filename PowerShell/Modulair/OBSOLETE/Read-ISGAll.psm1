# Read-ISGAll.psm1
# ---------------------------------------------------------
# Vereist:
#   Import-Module Shared
#   Import-Module ISGMapping
# ---------------------------------------------------------

function Read-ISGAll {
    param(
        [string]$Ip,
        [int]$Port = 502,
        [int]$SlaveId = 1
    )

    # 1. Mapping ophalen
    $mapping = Get-ISGMapping

    # 2. Alle registers ophalen (jouw bestaande Modbus-read functie)
    #    Dit moet een int[] array opleveren met raw values per register.
    $raw = Read-ISGRegisters -Addresses $mapping.Keys -Mapping $mapping

    # 3. Ruwe registers opslaan zodat de decoder 32-bit kan combineren
    $script:RawRegisters = $raw

    # 4. Outputlijst
    $result = @()

    foreach ($reg in $mapping.Keys | Sort-Object) {

        $def = $mapping[$reg]

        # Skip registers die Use=$false hebben
        if ($def.ContainsKey("Show") -and -not $def.Show) {
            continue
        }

        # Raw value ophalen
        $value = $raw[$reg]

        # Decoderen
        $decoded = Decode-ISGRegisterValue -Reg $reg -Value $value -Raw $raw -Mapping $mapping

        # Als decoder $null teruggeeft → skip
        if (-not $decoded) { continue }

        # Outputobject
        $result += [PSCustomObject]@{
            Register = $reg
            Name     = $def.Name
            RawValue = $value
            Decoded  = $decoded
        }
    }

    # 5. Calculated fields toevoegen
    $result = Add-CalculatedFields -Data $result


    return $result
}