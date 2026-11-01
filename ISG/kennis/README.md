# ISG-kennis — wat we van het apparaat weten en hoe zeker dat is

Deze map bevat de geconsolideerde kennis over de Stiebel ISG-gateway. Het was verspreid over
zeven mappen, elk met een deel van het verhaal en onderling tegenstrijdig. Hier staat één
versie, met per gegeven de herkomst en de status.

**De spiegel blijft gescheiden.** `ISG/firmware/` is een exacte kopie van wat op het apparaat
staat — uitrollen is daar kopiëren. Deze map bevat kennis *over* het apparaat en gaat er nooit
naartoe. `ISG/dumps/` bevat wat we eruit hebben gehaald, `ISG/staged/` wat er nog heen moet.

---

## De geldigheidsregels

Dit is de hoofdvondst en het vervangt alle eerdere sentinel-lijsten. Er waren er vier, die
elkaar deels tegenspraken (`isg_logger.py`, `isg_logger_delta_engine.py`, `manifest_uitleg.md`
en de objecttabel). Geen van de vier was nodig.

De Stiebel-software moet het onderscheid tussen een meting en een ongeldige waarde
deterministisch kunnen maken, zonder magische getallen te kennen. Ze doet dat zo:

### Regel 1 — nooit beschreven

Een waarde van `32768` tot en met `65535` is ongeldig.

`32768` is `0x8000`: bit 15 gezet, en voor een signed 16-bits register precies `INT16_MIN`.
De firmware initialiseert elk object op die waarde; een register dat de hardware nooit
beschrijft, blijft eropstaan. Dat is te zien in `../dumps/WPM_3_isg_objects.json`, waar
`value` bij **alle** 479 records `32768` is — ook bij registers die in bedrijf een echte
waarde hebben.

Waarom de drempel veilig is: over alle 681 registers is de hoogste waarde die ooit een echte
meting bleek `22624`. Er is dus geen enkele overlap met het gebied vanaf `0x8000`. Per
`wtype` ligt het maximum nog lager. Welke exacte waarde een type gebruikt, verschilt
(`wtype 5` gebruikt `0x8080`, de rest `0x8000`, soms `0x9000`), maar voor de regel maakt dat
niet uit.

Een waarde **boven** `65535` is juist geldig: dat is een 32-bits register, en daar klopt de
`max` uit de tester niet.

### Regel 2 — sensor ontbreekt of is defect

Staat er een echt bereik in de tester-uitvoer, dan is `waarde × schaal` buiten `[min, max]`
ongeldig.

Dit vangt wat regel 1 ontglipt: een niet-aangesloten NTC levert de ondergrens van zijn
meetbereik, en dat is een gewoon negatief getal. Zulke registers zijn te herkennen doordat ze
over duizenden metingen exact dezelfde waarde houden — een werkende sensor ruist altijd, dus
een waarde die op precies de bereikgrens vastligt meet niet.

**Let op de eenheden:** `min` en `max` in de tester-uitvoer zijn *geschaalde* grootheden, de
waarde is *ruw*. Vergelijken zonder te schalen levert tientallen valse alarmen.

### Uitzondering — bitvelden

Een bitveld kan legitiem bit 15 gebruiken, en dan slaat regel 1 een geldige waarde weg. In de
gemeten data gebeurt dat niet (de hoogste gecombineerde bitmaske is `0x4041`, dus bit 14 wel
en bit 15 nergens), maar dat is waarneming, geen garantie.

Daarom markeert de catalogus deze registers in de kolom `bitveld`. **Pas regel 1 daar niet
toe.** Het zijn er weinig.

---

## De catalogus

`webid_catalogus.csv` — één regel per WebID, met per veld de bron en de status. De kolommen
`status`, `schaal_status` en `gedrag_gemeten` zeggen hoe hard het gegeven is; vul die niet aan
zonder de bron bij te werken.

Betrouwbaarheid loopt van hard naar zacht:

| status | waar het op rust |
|---|---|
| `zeker (HA-bevestigd)` | HA leest het register via Modbus, de tester direct; gelijktijdig gemeten en gelijk na schaling |
| `zeker (empirisch)` | Erik heeft het zelf vastgesteld; staat in `verificatie_erik` |
| `waarschijnlijk` | één bron geeft een naam, niet geverifieerd |
| `onbekend` | geen enkele bron zegt iets; de schaal is daar `1.0` als **default**, niet als vaststelling |

Die laatste regel is belangrijk. De schaal is voor de overgrote meerderheid niet onderzocht,
en `1.0` betekent daar "niet gemeten". Alleen waar `schaal_status` `feit` of `geverifieerd`
is, mag je erop bouwen.

**Het apparaat is altijd de bron** voor `wtype`, `access`, `min`, `max` en de eenheid. Die
komen uit `isg_tester -p` en worden nooit door ons opgeschreven. Een conflict met een manifest
wordt in het voordeel van het apparaat beslecht.

---

## Wat hier staat en waar het vandaan komt

| bestand | herkomst | wat erin staat |
|---|---|---|
| `webid_catalogus.csv` | samengesteld | de geconsolideerde kennis per WebID |
| `metingen/*.csv` | Erik, januari 2026 | vier snapshots in verschillende bedrijfstoestanden, met een kolom `empirisch` die per register vastlegt wat is nagetrokken; dit is de rijkste bron die er is |
| `manifest_681_webids.json` | pipeline januari 2026 | alle WebID's met de Modbus- en KNX-mapping; eenheid deels gevuld, schaal grotendeels niet |
| `manifest_126_webids.json` | eerdere pipeline | kleinere set, wel met schaal en een `show`-vlag |
| `manifest_uitleg.md` | Erik | het ontwerp achter het manifest. De tweedeling register-specifiek versus universeel klopt en is nu gemeten; de beschreven **mapstructuur bestaat niet** — het is een intentieverklaring |
| `manifest_schema.json`, `manifest_enums.json` | pipeline | het formele schema. Het schema noemt het veld `analyze`, de loggers lezen `analyse` — die inconsistentie is nog niet opgelost |
| `modbus_mapping_2026-01-13.yaml` | Erik | de Modbus-mapping met een `use`-vlag per register; de eerste schrijf-whitelist, met `reset_command` en `restart_isg` al op `false` |

---

## Hoe de koppeling met HA is gecontroleerd

De sterkste verificatie die beschikbaar is, en herhaalbaar: HA leest de registers via Modbus
met eigen schaalfactoren, de tester leest ze direct. Beide lezen dezelfde bron, dus na
schaling moeten ze hetzelfde geven.

De keten is `HA-entiteit → modbus-adres → Modicon → WebID → ruwe waarde`, waarbij
`Modicon = adres + 30001` voor input registers en `+ 40001` voor holding registers, en
`../firmware/rel_b/etc/config/WPM-isg-modbus.csv` de laatste stap levert.

Eén uitkomst daarvan is blijvend relevant: waar de tester `32768` geeft, laat HA `-32768` zien.
Hetzelfde bitpatroon, door HA als int16 en door de tester als uint16 gelezen. Dat is de
herkomst van de foutenruis van de Modbus-integratie, en het verklaart waarom de oude
sentinel-lijsten zowel `32768` als `-32768` noemden.

---

## Open punten

1. **De schaal van de registers zonder naam.** Afleiden uit `min`, `max` en de ruwe waarde
   werkt in principe — het spreekt de door HA bevestigde schalen nergens tegen — maar één
   momentopname laat te veel kandidaten open. Met het waardenbereik over een langere periode
   wordt het scherper. Niet gedaan.
2. **WebID 402.** `metingen/*.csv` noemt dit "LAUFZEIT HEIZEN" met schaal 0,1 en de
   aantekening dat het empirisch is vastgesteld. Daartegen pleit dat de waarde daalt (een
   looptijdteller doet dat niet), dat het zich als bitveld gedraagt, en dat dit het enige
   register met `attribute=10` is. Vermoedelijk een pompen- of mengerbitveld. Onbeslist.
3. **WebID 485** is een statusbitveld waarvan de bitbetekenis niet is uitgewerkt; dat staat zo
   in `verificatie_erik`.
4. **WebID 481** komt in het stappenplan voor als "reststilstand" maar **bestaat niet** in de
   681 die de tester geeft.
5. **Registers die op dit apparaat dood zijn** terwijl ze in de analyse-opzet nodig waren:
   onder meer de compressorfrequentie. De catalogus wijst ze aan via `gedrag_gemeten`; het
   toerental komt wel door, via andere WebID's.
6. **Vier verschillende `schema.json`-bestanden** stonden in de repo. Hier staat er één; de
   andere zijn niet vergeleken op inhoudelijke verschillen.
