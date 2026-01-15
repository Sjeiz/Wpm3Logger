# Pad naar input en output
$input  = "D:\git\Sjeiz\Wpm3Logger\bin\Debug\net8.0\wpm3_2026-01-09.csv"
$output = "D:\git\Sjeiz\Wpm3Logger\bin\Debug\net8.0\wpm3_2026-01-09-small.csv"

# Lees alle regels
$lines = Get-Content $input

# Header is altijd de eerste regel
$header = $lines[0]

# Selecteer elke 6e regel (index 3, 7, 11, ...)
$data = $lines |
    Select-Object -Skip 1 |
    Where-Object { ($_ | ForEach-Object { $global:i++; $global:i }) -and (($global:i % 6) -eq 0) }

# Schrijf header + geselecteerde regels weg
$header | Out-File $output -Encoding UTF8
$data   | Out-File $output -Append -Encoding UTF8