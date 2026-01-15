#requires -Version 5.1
<#
.SYNOPSIS
    Calculated fields for ISG decoded register data.

.DESCRIPTION
    Deze module bevat functies die afgeleide waarden berekenen op basis van
    gedecodeerde ISG-registers. De mapping blijft declaratief in ISGMapping.psm1,
    terwijl deze module business logic bevat zoals totalen, dagtotalen en later
    diagnostische velden.

.NOTES
    Auteur: Erik & Copilot
    Module: ISGCalculatedFields
#>

function Add-CalculatedFields {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory, ValueFromPipeline)]
        [System.Collections.IEnumerable]$Data
    )

    #
    # Bouw een index op registernummer voor snelle lookup
    #
    $byReg = @{}
    foreach ($row in $Data) {
        if ($null -ne $row.Register) {
            $byReg[$row.Register] = $row
        }
    }

    #
    # Helperfunctie om veilig een waarde op te halen
    #
    function Get-Val {
        param([int]$Reg)
        if ($byReg.ContainsKey($Reg) -and $byReg[$Reg].PSObject.Properties.Match('Value')) {
            return [double]$byReg[$Reg].Value
        }
        return 0.0
    }

    #
    # Calculated fields verzamelen
    #
    $calc = @()

    # 1. Totaal geleverde warmte (alle circuits, MWh)
    $calc += [pscustomobject]@{
        Register = $null
        Name     = 'Total Heat All Circuits'
        RawValue = $null
        Value    = (Get-Val 3501) + (Get-Val 3504) + (Get-Val 3506) + (Get-Val 3508)
        Unit     = 'MWh'
    }

    # 2. Totaal elektrisch verbruik (alle circuits, MWh)
    $calc += [pscustomobject]@{
        Register = $null
        Name     = 'Total Power All Circuits'
        RawValue = $null
        Value    = (Get-Val 3511) + (Get-Val 3514)
        Unit     = 'MWh'
    }

    # 3. Totale dagwarmte (vloer + water, kWh)
    $calc += [pscustomobject]@{
        Register = $null
        Name     = 'Total Heat Day All Circuits'
        RawValue = $null
        Value    = (Get-Val 3500) + (Get-Val 3503)
        Unit     = 'kWh'
    }

    # 4. Totale dagconsumptie (vloer + water, kWh)
    $calc += [pscustomobject]@{
        Register = $null
        Name     = 'Total Power Day All Circuits'
        RawValue = $null
        Value    = (Get-Val 3510) + (Get-Val 3513)
        Unit     = 'kWh'
    }

    #
    # Combineer originele data + calculated fields
    #
    return @($Data) + $calc
}

