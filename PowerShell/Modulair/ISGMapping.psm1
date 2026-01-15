# ISGMapping.psm1
# ---------------------------------------------------------
# Mappingmodule voor ISG registers
# ---------------------------------------------------------

# ---------------------------------------------------------
# ProcessData 513–547
# Category: System
# Beschrijving:
#   Fysieke sensoren zoals temperaturen, drukken, flows,
#   bufferwaarden en compressor-sensoren.
# ---------------------------------------------------------
$script:ProcessData = @{
    513 = @{ Name="NHZ Flow Temperature";             Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    514 = @{ Name="Flow Temperature";                 Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    515 = @{ Name="Return Temperature";               Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    516 = @{ Name="Fixed Temperature";                Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    517 = @{ Name="Buffer Actual Temperature";        Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    518 = @{ Name="Buffer Set Temperature";           Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    519 = @{ Name="Water Pressure";                   Type="UInt16"; Scale=0.01; Unit="bar";   Show=$true; Category="System"; Analyse=$true }
    520 = @{ Name="Flow Rate";                        Type="UInt16"; Scale=0.01; Unit="l/min"; Show=$true; Category="System"; Analyse=$true }
    521 = @{ Name="Hot Water Actual Temperature";     Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    522 = @{ Name="Hot Water Set Temperature";        Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    525 = @{ Name="Floor Actual Temperature";         Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    526 = @{ Name="Floor Set Temperature";            Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    541 = @{ Name="Compressor Return Temperature";    Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    542 = @{ Name="Compressor Flow Temperature";      Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    543 = @{ Name="Compressor Hot Gas Temperature";   Type="UInt16"; Scale=0.1;  Unit="°C";    Show=$true; Category="System"; Analyse=$true }
    544 = @{ Name="Compressor Low Pressure";          Type="UInt16"; Scale=0.01; Unit="bar";   Show=$true; Category="System"; Analyse=$true }
    545 = @{ Name="Compressor Mean Pressure";         Type="UInt16"; Scale=0.01; Unit="bar";   Show=$true; Category="System"; Analyse=$true }
    546 = @{ Name="Compressor High Pressure";         Type="UInt16"; Scale=0.01; Unit="bar";   Show=$true; Category="System"; Analyse=$true }
    547 = @{ Name="Compressor Flow Rate";             Type="UInt16"; Scale=0.1;  Unit="l/min"; Show=$true; Category="System"; Analyse=$true }
}

# ---------------------------------------------------------
# SystemParameters 1500–1520
# Category: Parameters
# Beschrijving:
#   Instellingen, curves, setpoints en commando-registers.
# ---------------------------------------------------------
$script:SystemParameters = @{
    1500 = @{
        Name="Operating Mode"
        Type="enum"
        Values=@{
            0="Emergency Operation"
            1="Standby Mode"
            2="Programmed Operation"
            3="Comfort Mode"
            4="Eco Mode"
            5="DHW Mode"
        }
        Show=$true
        Category="Parameters"
        Analyse=$true      # Mode bepaalt gedrag → essentieel
    }

    1501 = @{ Name="Comfort Temperature HK1"; Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="Parameters"; Analyse=$true }
    1502 = @{ Name="Eco Temperature HK1";     Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="Parameters"; Analyse=$true }
    1503 = @{ Name="Heating Curve HK1";       Type="UInt16"; Scale=0.01; Unit="";  Show=$true; Category="Parameters"; Analyse=$true }

    1504 = @{ Name="Comfort Temperature HK2"; Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="Parameters"; Analyse=$true }
    1505 = @{ Name="Eco Temperature HK2";     Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="Parameters"; Analyse=$true }
    1506 = @{ Name="Heating Curve HK2";       Type="UInt16"; Scale=0.01; Unit="";  Show=$true; Category="Parameters"; Analyse=$true }

    1507 = @{ Name="Fixed Value Temperature"; Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="Parameters"; Analyse=$false }
    1508 = @{ Name="Dual Mode Temperature HZF"; Type="Int16"; Scale=0.1; Unit="°C"; Show=$true; Category="Parameters"; Analyse=$false }

    1509 = @{ Name="Comfort Temperature DHW"; Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="Parameters"; Analyse=$true }
    1510 = @{ Name="Eco Temperature DHW";     Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="Parameters"; Analyse=$true }

    1512 = @{ Name="Bivalent Temperature NHZ"; Type="Int16"; Scale=0.1; Unit="°C"; Show=$true; Category="Parameters"; Analyse=$false }

    1513 = @{ Name="Set Flow Temperature (Floor Cooling)"; Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="Parameters"; Analyse=$true }
    1517 = @{ Name="Set Flow Temperature (Fan Cooling)";   Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="Parameters"; Analyse=$false }
    1518 = @{ Name="Flow Temp Hysteresis (Fan Cooling)";   Type="UInt16"; Scale=0.1; Unit="K";  Show=$true; Category="Parameters"; Analyse=$false }

    1519 = @{ Name="Reset Command";   Type="UInt16"; Scale=1.0; Unit=""; Show=$true; Category="Parameters"; Analyse=$false }
    1520 = @{ Name="Restart ISG Command"; Type="UInt16"; Scale=1.0; Unit=""; Show=$true; Category="Parameters"; Analyse=$false }
}

# ---------------------------------------------------------
# Status 2500–2507
# Category: Status
# Beschrijving:
#   Statusbits, foutcodes, busstatus en defroststatus.
# ---------------------------------------------------------
$script:Status = @{
    2500 = @{
        Name="Operating Status (General)"
        Type="bitfield"
        Bits=@{
            0="Pump HK1 active"
            1="Pump HK2 active"
            2="Heat-up Program active"
            3="Electrical Heater active"
            4="Heating Floor active"
            5="Heating Water active"
            6="Compressor active"
            7="Summer Mode active"
            8="Cooling Mode active"
            9="Defrost Mode active"
            10="Silent Mode 1 active"
            11="Silent Mode 2 active"
        }
        Show=$true
        Category="Status"
        Analyse=$true      # Kernstatus voor analyse
    }

    2501 = @{
        Name="Power-Off Status"
        Type="bitfield"
        Bits=@{ 0="Power-Off active" }
        Show=$true
        Category="Status"
        Analyse=$false     # Niet relevant voor thermische analyse
    }

    2502 = @{
        Name="Operating Status (Compressors & Buffer Pumps)"
        Type="bitfield"
        Bits=@{
            0="Compressor 1 active"
            1="Compressor 2 active"
            2="Compressor 3 active"
            3="Compressor 4 active"
            4="Compressor 5 active"
            5="Compressor 6 active"
            6="Buffer Charging Pump 1 active"
            7="Buffer Charging Pump 2 active"
            8="Buffer Charging Pump 3 active"
            9="Buffer Charging Pump 4 active"
            10="Buffer Charging Pump 5 active"
            11="Buffer Charging Pump 6 active"
            12="NHZ Stage 1 active"
            13="NHZ Stage 2 active"
        }
        Show=$true
        Category="Status"
        Analyse=$true      # Essentieel voor compressor/NHZ analyse
    }

    2503 = @{
        Name="Fault Status"
        Type="enum"
        Values=@{
            0="No Fault"
            1="Fault Active"
        }
        Show=$true
        Category="Status"
        Analyse=$true      # Fouten beïnvloeden gedrag → belangrijk
    }

    2504 = @{
        Name="Bus Status"
        Type="enum"
        Values=@{
            0="Status OK"
            -1="Status Error"
            -2="Error Passive"
            -3="Bus Off"
            -4="Physical Error"
        }
        Show=$true
        Category="Status"
        Analyse=$true      # Busproblemen kunnen uitlezing beïnvloeden
    }

    2505 = @{
        Name="Defrost Initiated"
        Type="enum"
        Values=@{
            0="Off"
            1="Initiated"
        }
        Show=$true
        Category="Status"
        Analyse=$true      # Defrost is cruciaal voor analyse
    }

    2506 = @{
        Name="Active Error Number"
        Type="UInt16"
        Scale=1.0
        Unit=""
        Show=$true
        Category="Status"
        Analyse=$true      # Foutcodes zijn relevant voor diagnose
    }

    2507 = @{
        Name="Message Number"
        Type="UInt16"
        Scale=1.0
        Unit=""
        Show=$true
        Category="Status"
        Analyse=$false     # Administratief, geen analysewaarde
    }
}

# ---------------------------------------------------------
# SGReady 4000–4002
# Category: SGReady
# Beschrijving:
#   Warmtepomp-specifieke SG-Ready registers.
# ---------------------------------------------------------
$script:SGReady = @{
    4000 = @{ Name="SG-Ready Enabled"; Type="UInt16"; Scale=1.0; Unit=""; Show=$true; Category="SGReady"; Analyse=$true }
    4001 = @{ Name="SG-Ready Input 1"; Type="UInt16"; Scale=1.0; Unit=""; Show=$true; Category="SGReady"; Analyse=$true }
    4002 = @{ Name="SG-Ready Input 2"; Type="UInt16"; Scale=1.0; Unit=""; Show=$true; Category="SGReady"; Analyse=$true }
}

# ---------------------------------------------------------
# HardwareConfig 8000–8024
# Category: System
# Beschrijving:
#   Hardwareconfiguratie, systeemlimieten, aanwezigheid van modules.
# ---------------------------------------------------------
$script:HardwareConfig = @{
    8000 = @{ Name="Minimum Buffer Temperature";         Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="System"; Analyse=$true }
    8001 = @{ Name="System Frost Protection";            Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="System"; Analyse=$true }

    8002 = @{ Name="Buffer 2 Present";                   Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$true }
    8003 = @{ Name="DHW 2 Present";                      Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$true }
    8004 = @{ Name="HK1 Present";                        Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$true }
    8005 = @{ Name="HK2 Present";                        Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$true }
    8006 = @{ Name="DHW Present";                        Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$true }
    8007 = @{ Name="Mixing Valve Present";               Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$true }

    8008 = @{ Name="HK Frost Protection";                Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="System"; Analyse=$false }

    8009 = @{ Name="Solar Present";                      Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$false }
    8010 = @{ Name="Pool Present";                       Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$false }

    8011 = @{ Name="Buffer Sensor Present";              Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$true }
    8012 = @{ Name="Buffer 2 Sensor Present";            Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$true }
    8013 = @{ Name="DHW 2 Sensor Present";               Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$true }

    8014 = @{ Name="Compressor 2 Present";               Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$true }
    8015 = @{ Name="Compressor Type";                    Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$true }

    8016 = @{ Name="Extra CAN Heat Pump Present";        Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$false }
    8017 = @{ Name="CAN Active";                         Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$false }

    8018 = @{ Name="Cooling Type";                       Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$false }
    8019 = @{ Name="Cooling Mixing Circuit";             Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$false }

    8020 = @{ Name="Extra Sensor";                       Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$false }
    8021 = @{ Name="Extra Module";                       Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$false }
    8022 = @{ Name="Reserved";                           Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$false }

    8023 = @{ Name="Buffer Type";                        Type="UInt16"; Scale=1.0; Unit="";  Show=$true; Category="System"; Analyse=$true }

    8024 = @{ Name="Cooling Changeover Temperature";     Type="UInt16"; Scale=0.1; Unit="°C"; Show=$true; Category="System"; Analyse=$false }
}

# ---------------------------------------------------------
# Block3500 3500–3547
# Categories:
#   Energy  → Warmte, vermogen, dagtotalen
#   Runtime → Draaitijden, uren, NHZ runtime
#   HP      → Warmtepomp-specifieke totalen (HP1–HP6)
# ---------------------------------------------------------
$script:Block3500 = @{
    # Energy (DAY values → Analyse = $true)
    3500 = @{ Name="Generated Heat Floor Day";        Type="UInt16"; Scale=0.01; Unit="kWh"; Show=$true; Category="Energy";  Analyse=$true }
    3501 = @{ Name="Generated Heat Floor Total";      Type="UInt32"; Scale=0.001; Unit="MWh"; Show=$true; Category="Energy"; Analyse=$false }
    3503 = @{ Name="Generated Heat Water Day";        Type="UInt16"; Scale=0.01; Unit="kWh"; Show=$true; Category="Energy";  Analyse=$true }
    3504 = @{ Name="Generated Heat Water Total";      Type="UInt32"; Scale=0.001; Unit="MWh"; Show=$true; Category="Energy"; Analyse=$false }
    3506 = @{ Name="Generated Heat Floor NHZ Total";  Type="UInt32"; Scale=0.001; Unit="MWh"; Show=$true; Category="Energy"; Analyse=$false }
    3508 = @{ Name="Generated Heat Water NHZ Total";  Type="UInt32"; Scale=0.001; Unit="MWh"; Show=$true; Category="Energy"; Analyse=$false }

    3510 = @{ Name="Power Consumed Floor Day";        Type="UInt16"; Scale=0.01; Unit="kWh"; Show=$true; Category="Energy";  Analyse=$true }
    3511 = @{ Name="Power Consumed Floor Total";      Type="UInt32"; Scale=0.001; Unit="MWh"; Show=$true; Category="Energy"; Analyse=$false }
    3513 = @{ Name="Power Consumed Water Day";        Type="UInt16"; Scale=0.01; Unit="kWh"; Show=$true; Category="Energy";  Analyse=$true }
    3514 = @{ Name="Power Consumed Water Total";      Type="UInt32"; Scale=0.001; Unit="MWh"; Show=$true; Category="Energy"; Analyse=$false }

    # Runtime (all relevant → Analyse = $true)
    3538 = @{ Name="Runtime Heating";                 Type="UInt16"; Scale=1.0; Unit="h"; Show=$true; Category="Runtime"; Analyse=$true }
    3541 = @{ Name="Runtime Water";                   Type="UInt16"; Scale=1.0; Unit="h"; Show=$true; Category="Runtime"; Analyse=$true }
    3544 = @{ Name="Runtime Cooling";                 Type="UInt16"; Scale=1.0; Unit="h"; Show=$true; Category="Runtime"; Analyse=$true }
    3545 = @{ Name="Runtime NHZ1";                    Type="UInt16"; Scale=1.0; Unit="h"; Show=$true; Category="Runtime"; Analyse=$false }
    3546 = @{ Name="Runtime NHZ2";                    Type="UInt16"; Scale=1.0; Unit="h"; Show=$true; Category="Runtime"; Analyse=$false }
    3547 = @{ Name="Runtime NHZ1/2";                  Type="UInt16"; Scale=1.0; Unit="h"; Show=$true; Category="Runtime"; Analyse=$false }

    # HP totals (never used for analysis → Analyse = $false)
    3520 = @{ Name="HP1 Total Heat";                  Type="UInt32"; Scale=1.0; Unit="kWh"; Show=$false; Category="HP"; Analyse=$false }
    3521 = @{ Name="HP1 Total Heat (major)";          Type="UInt16"; Scale=1.0; Unit="";   Show=$false; Category="HP"; Analyse=$false }
    3522 = @{ Name="HP2 Total Heat";                  Type="UInt32"; Scale=1.0; Unit="kWh"; Show=$false; Category="HP"; Analyse=$false }
    3523 = @{ Name="HP2 Total Heat (major)";          Type="UInt16"; Scale=1.0; Unit="";   Show=$false; Category="HP"; Analyse=$false }
    3524 = @{ Name="HP3 Total Heat";                  Type="UInt32"; Scale=1.0; Unit="kWh"; Show=$false; Category="HP"; Analyse=$false }
    3525 = @{ Name="HP3 Total Heat (major)";          Type="UInt16"; Scale=1.0; Unit="";   Show=$false; Category="HP"; Analyse=$false }
    3526 = @{ Name="HP4 Total Heat";                  Type="UInt32"; Scale=1.0; Unit="kWh"; Show=$false; Category="HP"; Analyse=$false }
    3527 = @{ Name="HP4 Total Heat (major)";          Type="UInt16"; Scale=1.0; Unit="";   Show=$false; Category="HP"; Analyse=$false }
    3528 = @{ Name="HP5 Total Heat";                  Type="UInt32"; Scale=1.0; Unit="kWh"; Show=$false; Category="HP"; Analyse=$false }
    3529 = @{ Name="HP5 Total Heat (major)";          Type="UInt16"; Scale=1.0; Unit="";   Show=$false; Category="HP"; Analyse=$false }
    3530 = @{ Name="HP6 Total Heat";                  Type="UInt32"; Scale=1.0; Unit="kWh"; Show=$false; Category="HP"; Analyse=$false }
    3531 = @{ Name="HP6 Total Heat (major)";          Type="UInt16"; Scale=1.0; Unit="";   Show=$false; Category="HP"; Analyse=$false }
}

function Get-ISGMapping {
    $all = @{}
    foreach ($map in @(
        $script:ProcessData,
        $script:SystemParameters,
        $script:Status,
        $script:HardwareConfig,
        $script:SGReady,
        $script:Block3500
    )) {
        foreach ($k in $map.Keys) {
            $all[$k] = $map[$k]
        }
    }
    return $all
}