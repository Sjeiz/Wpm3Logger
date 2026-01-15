param(
    [string]$IpAddress = "192.168.12.117",
    [int]   $Port      = 502,
    [byte]  $Slave     = 1,
    [ValidateSet('Seperate','Aggregate')]
    [string]$Mode      = 'Aggregate',
    [switch]$AnalyseOnly
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# 1.Modbus
Import-Module (Join-Path $scriptDir "ModbusClient.psm1")         -Force -DisableNameChecking

# 2. Mappings & Helpers
Import-Module (Join-Path $scriptDir "ISGMapping.psm1")           -Force -DisableNameChecking
Import-Module (Join-Path $scriptDir "ISGHelpers.psm1")           -Force -DisableNameChecking
Import-Module (Join-Path $scriptDir "ISGCalculatedFields.psm1")  -Force -DisableNameChecking

# 3. Readers
Import-Module (Join-Path $scriptDir "ISGReaders.psm1")           -Force -DisableNameChecking

Connect-ISG -IpAddress $IpAddress -Port $Port -Slave $Slave

$data = Read-ISGAll

# Helper: vlakke tabel
function Convert-ToFlat {
    param(
        [array]$Items,
        [hashtable]$Mapping
    )

    foreach ($i in $Items) {
        # 👉 Guard tegen calculated fields / lege items
        if (-not $i.Register) {
            Write-Warning "Skipping item without Register"
            continue
        }

        $map = $Mapping[$i.Register]

        if (-not $map) {
            Write-Warning "Register $($i.Register) not found in mapping"
            continue
        }

        $value = $null
        $unit  = ""

        if ($i.Decoded -match '=\s*([-\d\.,]+)\s+([^\d].*)$') {
            $value = $Matches[1] -replace ',', '.'
            $value = [double]$value
            $unit  = $Matches[2].Trim()
        }
        elseif ($i.Decoded -match '=\s*([-\d\.,]+)\s*$') {
            $value = $Matches[1] -replace ',', '.'
            $value = [double]$value
        }
        elseif ($i.Decoded -match '=\s*(.*)$') {
            $value = $Matches[1].Trim()
        }
        else {
            $value = $i.Decoded
        }

        [PSCustomObject]@{
            Register = $i.Register
            Name     = $map.Name
            RawValue = $i.RawValue
            Value    = $value
            Unit     = $map.Unit
            Analyse  = $map.Analyse
            Category = $map.Category
        }
    }
}

# 1. Mapping ophalen
$mapping = Get-ISGMapping

# 2. Categorieën opbouwen op basis van mapping
$categories =
    $mapping.GetEnumerator() |
    Group-Object { $_.Value.Category } |
    ForEach-Object {
        [PSCustomObject]@{
            Name      = $_.Name
            Registers = $_.Group.Key
        }
    }

# 3. Weergave afhankelijk van modus
switch ($Mode) {
    'Aggregate' {
        $all =
            $categories |
            ForEach-Object {
                $cat = $_
                $data |
                    Where-Object { $_.Register -in $cat.Registers }
            } |
            Sort-Object Register

        if ($AnalyseOnly) {
            $all = $all | Where-Object { $mapping[$_.Register].Analyse }
        }

        Convert-ToFlat -Items $all -Mapping $mapping| Out-GridView -Title "ISG Aggregate Data"
    }

    'Seperate' {
        foreach ($cat in $categories) {
            $subset =
                $data |
                Where-Object { $_.Register -in $cat.Registers } |
                Sort-Object Register

            if ($AnalyseOnly) {
                $subset = $subset | Where-Object { $mapping[$_.Register].Analyse }
            }

            Convert-ToFlat -Items $subset -Mapping $mapping | Out-GridView -Title "ISG $($cat.Name)"
        }
    }
}

# Disconnect alleen als de functie bestaat
if (Get-Command Disconnect-ISG -ErrorAction SilentlyContinue) {
    Disconnect-ISG
}