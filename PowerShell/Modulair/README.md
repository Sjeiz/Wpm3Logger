ISGReaders
Een expliciete, mapping‑gedreven PowerShell‑module voor het uitlezen van alle ISG‑datapunten zonder heuristiek, zonder ranges en zonder verborgen logica.
Doel
ISGReaders biedt één uniforme, toekomstvaste manier om alle relevante ISG‑waarden uit te lezen via volledig expliciete mappings.
De module vermijdt bewust elke vorm van gokwerk: geen ranges, geen impliciete aannames, geen verborgen logica, geen afhankelijkheid van offsets.
Alles is transparant, uitbreidbaar en stabiel.
Architectuur
De module bestaat uit drie hoofdonderdelen:
- Mappings
Bevat alle expliciete definities van ISG‑datapunten, gegroepeerd per categorie: HK1, HK2, Compressor, Temperatures, Energy, System.
Elke mapping bevat: Id, Name, Unit, Category, optioneel Parser, Description.
De mappings zijn platte lijsten zonder logica.
- Readers (alle .psm1 bestanden)
Elke categorie heeft een eigen readerbestand, bijvoorbeeld:
Read‑ISGHK1.psm1
Read‑ISGHK2.psm1
Read‑ISGCompressor.psm1
Read‑ISGTemperatures.psm1
Read‑ISGEnergy.psm1
Read‑ISGSystem.psm1
Elke reader:
- leest uitsluitend de mapping
- voert geen heuristiek uit
- retourneert een object met stabiele propertynamen
- is volledig onafhankelijk van andere readers
- bevat alleen logica voor het uitlezen van de datapunten uit de mapping
- Read‑ISGAll.psm1
De centrale orchestrator.
Deze functie:
- roept alle readers aan
- combineert de resultaten
- retourneert één stabiel object met vaste top‑level categorieën
- blijft backward‑compatible, ook als mappings worden uitgebreid
- bevat geen logica over individuele datapunten
Installatie
Plaats de module in een standaard PowerShell‑modulemap:
$env:USERPROFILE\Documents\PowerShell\Modules\ISGReaders\
Of importeer handmatig:
Import‑Module ./ISGReaders.psm1
Gebruik
Alles uitlezen:
$data = Read‑ISGAll
$data
Eén categorie uitlezen:
$hk1 = Read‑ISGHK1
$compressor = Read‑ISGCompressor
Specifiek datapunt:
$data = Read‑ISGAll
$data.HK1.FlowTemperature
Ontwerpprincipes
- 100% expliciet: alle datapunten staan in mappings
- Toekomstvast: nieuwe firmware = mappings uitbreiden
- Backward‑compatible: propertynamen veranderen nooit
- Modulair: elke categorie heeft een eigen reader
- Onderhoudbaar: mappings zijn eenvoudig te lezen en te wijzigen
- Geen heuristiek, geen ranges, geen verborgen logica
Structuur
ISGReaders/
ISGReaders.psm1
README.md
Mappings/
HK1.json
HK2.json
Compressor.json
Temperatures.json
Energy.json
System.json
Readers/
Read‑ISGHK1.psm1
Read‑ISGHK2.psm1
Read‑ISGCompressor.psm1
Read‑ISGTemperatures.psm1
Read‑ISGEnergy.psm1
Read‑ISGSystem.psm1
Read‑ISGAll.psm1
Uitbreiden
Nieuw datapunt toevoegen:
- Open de juiste mapping
- Voeg een nieuw object toe met Id, Name, Unit, Category, Description
- Klaar: de reader pakt het automatisch op
Nieuwe categorie toevoegen:
- Maak een nieuwe mapping
- Maak een nieuwe reader in Readers/
- Voeg de reader toe aan Read‑ISGAll
Outputstructuur
Read‑ISGAll retourneert een object met vaste top‑level categorieën:
HK1
HK2
Compressor
Temperatures
Energy
System
Troubleshooting
Waarde ontbreekt: controleer of het datapunt in de mapping staat
Waarde is null: datapunt wordt niet geleverd door deze firmware
Nieuwe firmware: voeg ontbrekende datapunten toe aan de mapping; readers hoeven nooit aangepast te worden
Licentie
MIT‑licentie
Bijdragen
Pull requests zijn welkom. Houd je aan de ontwerpprincipes: expliciet, mapping‑gedreven, geen heuristiek, backward‑compatible, stabiele interface.
