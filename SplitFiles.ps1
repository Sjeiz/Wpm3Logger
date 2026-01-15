# Pad naar het originele CSV-bestand
$inputFile = "D:\git\Sjeiz\Wpm3Logger\bin\Debug\net8.0\wpm3_2026-01-07.csv"

# Aantal regels per deel (inclusief header)
$chunkSize = 500

# Lees alle regels
$lines = Get-Content $inputFile

# Header is altijd de eerste regel
$header = $lines[0]

# Start bij regel 1 (dus zonder header)
$dataLines = $lines[1..($lines.Count - 1)]

# Teller voor bestandsnamen
$part = 1

# Loop door de data in blokken van $chunkSize - 1 (want header telt mee)
for ($i = 0; $i -lt $dataLines.Count; $i += ($chunkSize - 1)) {

    # Bepaal de output-bestandsnaam
    $outputFile = "{0}_part{1}.csv" -f $inputFile, $part

    # Pak een blok regels
    $chunk = $dataLines[$i..([Math]::Min($i + ($chunkSize - 2), $dataLines.Count - 1))]

    # Schrijf header + blok naar nieuw bestand
    $output = @($header) + $chunk
    $output | Set-Content $outputFile -Encoding UTF8

    Write-Host "Geschreven: $outputFile"

    $part++
}