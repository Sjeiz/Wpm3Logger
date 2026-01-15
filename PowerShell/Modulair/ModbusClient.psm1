# ModbusClient.psm1

# 1. NModbus laden

$script:ModuleRoot = Split-Path -Parent $PSCommandPath
$dllPath           = Join-Path $script:ModuleRoot "NModbus.dll"

if (-not (Test-Path $dllPath)) {
    throw "NModbus.dll niet gevonden in $script:ModuleRoot"
}

Add-Type -Path $dllPath

# 2. State

$script:Master = $null
$script:Slave  = 1

function Connect-ISG {
    param(
        [string]$IpAddress,
        [int]   $Port  = 502,
        [byte]  $Slave = 1
    )

    # Sluit oude verbinding af
    if ($script:Master) {
        try { $script:Master.Dispose() } catch {}
        $script:Master = $null
    }

    # Nieuwe TCP-client
    $tcp = New-Object System.Net.Sockets.TcpClient
    $tcp.ReceiveTimeout = 2000
    $tcp.SendTimeout    = 2000

    try {
        $tcp.Connect($IpAddress, $Port)
    }
    catch {
        throw "Kan geen verbinding maken met ISG op $($IpAddress):$($Port)"
    }

    # NModbus master
    $factory       = New-Object NModbus.ModbusFactory
    $script:Master = $factory.CreateMaster($tcp)
    $script:Slave  = $Slave
}

function Disconnect-ISG {
    if ($script:Master -and $script:Master.Transport) {
        try { $script:Master.Dispose() } catch {}
    }
    $script:Master = $null
}

function Get-ISGMaster {
    if (-not $script:Master) {
        throw "ISG Modbus master is niet verbonden. Roep eerst Connect-ISG aan."
    }
    return $script:Master
}

# 3. Central Read-ISGRegister (mapping-gestuurd)

function Read-ISGRegister {
    param(
        [int]$Address,
        [string]$RegisterType = $null
    )

    $master = Get-ISGMaster

    #
    # 1. Als RegisterType bekend is (uit mapping), gebruik dat
    #
    if ($RegisterType -eq "Holding") {
        try {
            return $master.ReadHoldingRegisters($script:Slave, $Address, 1)[0]
        }
        catch {}
    }

    if ($RegisterType -eq "Input") {
        try {
            return $master.ReadInputRegisters($script:Slave, $Address, 1)[0]
        }
        catch {}
    }

    #
    # 2. Anders: tolerant fallback (eerst Holding, dan Input)
    #
    try {
        return $master.ReadHoldingRegisters($script:Slave, $Address, 1)[0]
    }
    catch {}

    try {
        return $master.ReadInputRegisters($script:Slave, $Address, 1)[0]
    }
    catch {}

    #
    # 3. Unknown / niet aanwezig
    #
    return $null
}

function Read-ISGRegisters {
    param(
        [int[]]$Addresses,
        [hashtable]$Mapping
    )

    $master = Get-ISGMaster

    # Bepaal hoogste register zodat we een array kunnen maken
    $max = ($Addresses | Measure-Object -Maximum).Maximum

    # Maak een int[] array groot genoeg
    $raw = New-Object int[] ($max + 1)

    foreach ($reg in $Addresses) {

        $type = $null
        if ($Mapping.ContainsKey($reg)) {
            $type = $Mapping[$reg].RegisterType
        }

        $value = Read-ISGRegister -Address $reg -RegisterType $type

        # Bewaar in array op index = registernummer
        $raw[$reg] = $value
    }

    return $raw
}