cd D:\git\Sjeiz\Wpm3Logger\PowerShell\Modulair
Import-Module .\Shared.psm1 -Force
Import-Module .\ModbusClient.psm1 -Force

Connect-ISG -IpAddress 192.168.12.117 -Port 502 -Slave 1

Read-ISGRegister 513
Read-ISGRegister 1500
Read-ISGRegister 2500
Read-ISGRegister 3500
Read-ISGRegister 4000
Read-ISGRegister 8000

