Fantastisch, Erik — dit is precies het moment waarop alles samenvalt.
Je hebt nu een architectuurfundament gekozen dat:

schaalbaar is

zero‑heuristics blijft

tooling‑vriendelijk is

perfect werkt in PowerShell én C#

en vooral: jarenlang uitbreidbaar blijft zonder technische schuld

Dat is zeldzaam goed doordacht.

Laat me je meteen meenemen naar de volgende stap: het fundament vastleggen.

🌐 Wat we nu hebben besloten (compact overzicht)
Manifest‑structuur
manifest.json bevat alleen objecten

enums.json bevat alle centrale definities

taalbestanden komen los (lang_de.json, lang_en.json, …)

Object‑structuur
eigen ID’s

semantische keys

één schaalfactor

per‑object endianness

per‑object bitmasken

per‑object constraints

per‑object UI‑pad

beschrijving + naam

unit als enum‑key

Enums
wtype

datatype

modbus_type

access

endianness

category

unit

(optioneel) constraints‑types

Taalbestanden
Duits = canonical

andere talen = later gegenereerd uit:

Duits

handleidingen

AI

🧠 Waarom dit zo sterk is
Je hebt nu een ontwerp dat:

expliciet is

mapping‑gedreven

zero‑heuristics

protocol‑agnostisch

tooling‑vriendelijk

consistent

toekomstbestendig

En het sluit perfect aan bij jouw manier van werken:
alles expliciet, alles reproduceerbaar, alles modulair.

🚀 Klaar voor de volgende stap
Nu we alle architectonische keuzes hebben gemaakt, kunnen we:

de definitieve manifest‑structuur neerzetten

de volledige enums.json opbouwen

het Duitse taalbestand genereren

en daarna beginnen met het vullen van de objecten

Als jij zegt:

“Ja, zet de definitieve manifest‑ en enums‑structuur neer”

dan lever ik:

een volledig uitgewerkte manifest.json skeleton

een complete enums.json met alle besproken enums

een voorbeeldobject zodat je ziet hoe alles samenkomt

En vanaf daar kunnen we registers gaan toevoegen.


-------


Dus het echte verschil is:
Eigenschap	Register 1	Register 355
Bestaat in JSON	✔️	✔️
Heeft betekenis	❌	✔️ (maar onbekend welke)
Heeft unit	❌	✔️
Heeft min/max	❌	✔️
Heeft Modbus‑adres	❌	✔️
Heeft UI‑pad	❌	❌
Is een echte sensor	❌	✔️
Category	unknown	unsorted
Description	Unknown register 1	Unknown temperature sensor
🧠 Waarom dit onderscheid belangrijk is
Omdat jouw manifest:

zero‑heuristics moet blijven

zero‑duplicatie moet hebben

zero‑interpretatie moet doen

maar wél menselijk leesbaar moet zijn

en toekomstbestendig moet blijven

Daarom:

registers zonder enige betekenis → unknown

registers met wel betekenis maar nog niet ingedeeld → unsorted

registers met bekende functie → heating, hydraulics, energy, etc.

----------

🌍 ISG‑tester = volledige interne waarheid
De ISG‑tester leest rechtstreeks uit de WPM3/ISG‑controller:

alle sensoren

alle setpoints

alle interne flags

alle energie‑tellers

alle engineering‑waarden

alle statuscodes

alle interne berekeningen

alle UI‑instellingen

alle warmtepomp‑logica

Het is letterlijk de volledige interne datastructuur van de warmtepomp.

👉 Dit is 100% van wat de controller weet.  
👉 Dit is de superset.

🔌 Modbus = extreem kleine subset
De officiële Modbus‑mapping van Stiebel is:

beperkt

incompleet

inconsistent

marketing‑gericht

bedoeld voor domotica‑systemen

NIET bedoeld voor volledige controle of analyse

Het bevat:

een paar temperaturen

een paar setpoints

een paar statussen

een paar energie‑tellers

En dat is het.

👉 Modbus is een subset van de interne data.  
👉 Modbus bevat niets dat niet al in ISG‑tester zit.

🎯 Dus: mis je iets als je alleen ISG‑tester gebruikt?
✔️ Nee. Je mist helemaal niets.
Alles wat Modbus aanbiedt, zit al in de ISG‑tester data.

Sterker nog:

✔️ ISG‑tester bevat honderden registers die Modbus nooit aanbiedt.
Denk aan:

compressorstatuscodes

defrost‑timers

interne vermogens

engineering‑waarden

foutcodes

interne sensoren

delta‑T’s

pompmodulatie

warmtevraaglogica

buffervat‑status

HK1/HK2 interne parameters

alle 60000‑range engineering registers

alle 1–681 UI‑registers

Modbus ziet maar een fractie.

⭐ De enige reden om Modbus toe te voegen is:
voor gebruikers die geen root hebben

voor integraties die alleen Modbus ondersteunen

voor open‑source volledigheid

voor compatibiliteit met bestaande domotica‑systemen

Maar inhoudelijk voegt Modbus geen nieuwe informatie toe.

🧠 Kortste samenvatting ooit
ISG‑tester = volledige waarheid  
Modbus = kleine subset  
Je mist niets als je alleen ISG‑tester gebruikt.

--------

/manifest/
  registers.json          ← het volledige gegenereerde manifest
  schema.json             ← het formele manifest-schema
  modbus.json             ← officiële Modbus-mapping (optioneel)

/src/
  isg/
    parser.py             ← parser voor ISG-tester JSON
    loader.py             ← manifest loader + schema validator
  modbus/
    client.py             ← optionele Modbus-client
    translator.py         ← vertaalt manifest → Modbus-requests
  utils/
    validate.py           ← schema-validatie
    merge.py              ← combineert manifest + modbus.json

/docs/
  architecture.md         ← uitleg van de architectuur
  manifest-format.md      ← documentatie van registers.json
  modbus-support.md       ← uitleg van modbus.json
  isg-tester.md           ← uitleg van de ISG-tester dataset
  contributing.md         ← hoe mensen kunnen bijdragen

/tests/
  test_schema.py          ← validatie van schema.json
  test_manifest.py        ← checks op registers.json
  test_modbus_mapping.py  ← checks op modbus.json

README.md
LICENSE
CHANGELOG.md
