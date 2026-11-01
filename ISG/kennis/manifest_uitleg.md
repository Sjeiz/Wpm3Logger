ISG Register Manifest
Een minimalistisch, toekomst‑proof manifest voor alle registers die door de Stiebel‑Eltron ISG‑tester worden blootgelegd — inclusief optionele ondersteuning voor de officiële Modbus‑mapping.

Dit project documenteert de volledige interne datastructuur van de WPM/ISG‑controller, gebaseerd op de ruwe output van de ISG‑tester.
Het doel is helderheid, eenvoud en volledige transparantie in een ecosysteem dat normaal gesloten en slecht gedocumenteerd is.

Waarom dit project bestaat
De ISG‑tester geeft toegang tot de volledige interne waarheid van de warmtepompcontroller:

alle sensoren

alle setpoints

alle flags

alle energie‑tellers

alle engineering‑waarden

alle interne statuscodes

alle UI‑instellingen

Dit is veel meer dan de officiële Modbus‑mapping, die slechts een kleine subset bevat.

Daarom is dit manifest gebaseerd op de ISG‑tester als primaire bron, met Modbus als optionele aanvulling.

Kenmerken
Volledige dekking van alle ISG‑tester registers

Strakke, minimalistische structuur

Geen reconstructie, geen endianness, geen Modbus‑ruis in de kern

Optionele Modbus‑metadata voor officiële registers

1‑based Modbus‑adressering, zoals gedocumenteerd door Stiebel

JSON Schema validatie voor stabiliteit en consistentie

Open‑source‑vriendelijke structuur

Tooling‑agnostisch: bruikbaar voor Python, Node, Go, Rust, Home Assistant, etc.

Bestandsstructuur
Code
/manifest/
  registers.json          ← volledig manifest
  schema.json             ← formeel JSON-schema
  modbus.json             ← officiële Modbus-mapping

/src/
  isg/                    ← ISG-tester parsers en loaders
  modbus/                 ← optionele Modbus-client en mapping tools
  utils/                  ← validatie en merge-tools

/docs/
  architecture.md         ← uitleg van de architectuur
  manifest-format.md      ← documentatie van registers.json
  modbus-support.md       ← uitleg van modbus.json
  isg-tester.md           ← uitleg van de ISG-tester dataset
  contributing.md         ← richtlijnen voor bijdragen

/tests/
  test_schema.py
  test_manifest.py
  test_modbus_mapping.py

README.md
LICENSE
CHANGELOG.md
Manifestformaat
Elke entry in registers.json volgt het schema uit schema.json.

Voorbeeld:

json
"16": {
  "id": 16,
  "description": "HK1 aanvoertemperatuur",
  "datatype": "int16",
  "unit": "°C",
  "access": "rw",
  "min": 5,
  "max": 30,
  "category": "sensor",
  "show": true,
  "analyze": true,
  "modbus": {
    "address": 3003,
    "type": "input_register",
    "datatype": "int16",
    "span": 1,
    "endianness": "low_high"
  }
}
Registers zonder officiële Modbus‑mapping hebben:

json
"modbus": null
Waarom Modbus optioneel is
De ISG‑tester dataset is compleet.
Modbus is slechts een subset en voegt geen nieuwe informatie toe.

Modbus‑metadata is daarom:

optioneel

gescheiden

niet vervuilend

alleen aanwezig voor officiële registers

Dit houdt het manifest schoon en toekomst‑proof.

## KNX‑mapping
De KNX‑sectie in het manifest beschrijft de optionele koppeling tussen een ISG‑Web register en een KNX‑communicatieobject.
Deze sectie bevat uitsluitend protocol‑specifieke metadata en geen duplicatie van datatype, scaling, unit of access.

Voorbeeld:

json
"knx": {
  "commObjID": 161,
  "dpt": "9.001",
  "flags": ["T", "U", "R"],
  "priority": "low"
}
Wanneer een register geen KNX‑mapping heeft:

Code
"knx": null
### KNX‑velden
commObjID
Het communicatieobject‑ID uit de officiële KNX‑mappingtabel.
Dit is het enige verplichte veld.

dpt
Het KNX‑datapoint type.
Dit wordt automatisch afgeleid op basis van:

datatype (int16, uint16, float32, enum, bitmask)

unit (°C, %, kW, etc.)

scaling

semantiek van het register

Voorbeelden:

datatype	unit	scaling	dpt
uint16	°C	0.1	9.001 (temperatuur)
uint16	%	0.1	9.007 (vochtigheid)
float32	kW	1	14.xxx (4‑byte float)
int16	none	1	8.xxx (signed 2‑byte)
uint16	none	1	7.xxx (unsigned 2‑byte)
bitmask	none	n/a	1.xxx of 20.xxx
Wanneer geen DPT kan worden bepaald:
"dpt": null

### flags
KNX‑objectflags bepalen hoe het communicatieobject zich gedraagt op de bus.
Mogelijke waarden:

T — Transmit (zendt waarde bij verandering)

U — Update (ontvangt updates van het apparaat)

R — Read (kan worden uitgelezen)

W — Write (kan worden geschreven)

C — Communication (algemene communicatieflag)

De flags worden automatisch bepaald op basis van:

access (RO/RW)

sensor/actuator‑semantiek

bitmask/enum‑gedrag

Regels:

RO → ["R", "U"]

RW → ["R", "W", "U"]

sensorwaarden → ["T", "U", "R"]

setpoints → ["W", "R", "U"]

bitmask → ["T"]

Wanneer flags onbekend zijn:
"flags": []

### priority
KNX‑prioriteit bepaalt hoe belangrijk telegrammen zijn op de bus.

Mogelijke waarden:

low (default)

normal

alarm (voor foutcodes / diagnosis)

Regels:

foutcodes → "alarm"

setpoints → "normal"

alle andere registers → "low"

Wanneer onbekend:
"priority": null

### Voorbeeld van een volledige entry met KNX
json
"60033": {
  "id": 60033,
  "name": "SystemStatus",
  "description": "",
  "datatype": "uint16",
  "unit": "",
  "scaling": 1,
  "access": "RO",
  "min": 0,
  "max": 65535,
  "category": "system",
  "show": true,
  "analyze": false,

  "modbus": null,

  "knx": {
    "commObjID": 161,
    "dpt": "9.001",
    "flags": ["T", "U", "R"],
    "priority": "low"
  }
}

## Value interpretation rules
De ISG‑Web API gebruikt speciale numerieke waarden (sentinels) om aan te geven dat een register:

nog niet gezet is (not set)

tijdelijk niet beschikbaar is (not available)

Deze sentinelwaarden zijn datatype‑afhankelijk en soms register‑specifiek.

1. Register‑specifieke sentinelwaarden (manifest)
Sommige registers hebben unieke sentinelwaarden, zoals:

bivalente temperatuur: -20 = notset

engineering registers: 0 = notavailable

bitmask: 65535 = notavailable

Deze worden expliciet vastgelegd in het manifest:

json
"sentinel": {
  "notset": -20,
  "notavailable": 32767
}
Wanneer deze sectie aanwezig is:

👉 Deze waarden hebben altijd voorrang op universele regels.

2. Universele sentinelwaarden (fallback)
Als een register geen sentinel‑sectie heeft, gelden de universele regels:

int16
32767 → not available

-32768 → not set

int32
2147483647 → not available

-2147483648 → not set

bitmask
65535 → not available

float32
NaN → not available

-1.0 → sometimes not set (register‑dependent)

3. Interpretatieregel (beslisboom)
Als het manifest een sentinel‑sectie heeft:

match notset

match notavailable

anders: waarde is geldig

Anders:

gebruik universele datatype‑regels

4. Gedrag in tooling
CSV‑analyse negeert notset

CSV‑analyse markeert notavailable als gat

UI toont — of n.v.t.

KNX/Modbus‑export slaat sentinelwaarden over


Validatie
Je kunt het manifest valideren met elke JSON‑schema validator:

Code
python src/utils/validate.py manifest/registers.json manifest/schema.json
Doelen van dit project
Een open standaard creëren voor ISG‑registerdocumentatie

Een betrouwbare basis bieden voor tooling, dashboards en integraties

De chaos van Stiebel‑documentatie vervangen door een helder, logisch model

Een community‑vriendelijke bron worden voor iedereen die met ISG/WPM werkt

Bijdragen
Bijdragen zijn welkom.
Zie /docs/contributing.md voor richtlijnen.

Licentie
Vrij te gebruiken onder een open‑source licentie (bijv. MIT of Apache 2.0).