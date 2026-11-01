# ISG-kennis — wat we van het apparaat weten en hoe zeker dat is

Deze map bevat de geconsolideerde kennis over de Stiebel ISG-gateway. Het was verspreid over
zeven mappen, elk met een deel van het verhaal en onderling tegenstrijdig. Hier staat één
versie, met per gegeven de herkomst en de status.

**De spiegel blijft gescheiden.** `ISG/firmware/` is een exacte kopie van wat op het apparaat
staat — uitrollen is daar kopiëren. Deze map bevat kennis *over* het apparaat en gaat er nooit
naartoe. `ISG/dumps/` bevat wat we eruit hebben gehaald, `ISG/staged/` wat er nog heen moet.

---

## De geldigheidsregel

Dit is de hoofdvondst. Er waren vier sentinel-lijsten die elkaar deels tegenspraken
(`isg_logger.py`, `isg_logger_delta_engine.py`, `manifest_uitleg.md` en de objecttabel).
Geen van de vier is nodig.

### Regel 1 -- het gebied vanaf 0x8000

**Een waarde van `32768` tot en met `65535` is ongeldig.** Voor elk register, elk datatype.

`32768` is `0x8000`: bit 15 gezet, voor een signed 16-bits register precies `INT16_MIN`.
De firmware initialiseert elk object op die waarde -- in `../dumps/WPM_3_isg_objects.json`
staat `value: 32768` bij **alle** 479 records, ook bij registers die in bedrijf een echte
waarde geven. Een register dat de hardware nooit beschrijft, blijft eropstaan.

Getoetst op **5,49 miljoen waarnemingen uit tien onafhankelijke bronnen** (zeven logdagen,
twee JSON-reeksen, vier toestandsnapshots). In het gebied vanaf `0x8000` komen precies drie
waarden voor:

| waarde | | bij welk type | hoe vaak |
|---|---|---|---|
| 32768 | `0x8000` | alle types behalve 5 | 18.514 |
| 32896 | `0x8080` | uitsluitend `wtype 5` | 678.650 |
| 36864 | `0x9000` | `presentation 7` | 38.780 |

De waarde is dus typegebonden, maar voor de regel maakt dat niet uit -- alle drie liggen
boven de drempel. Dat `wtype 5` een eigen waarde heeft past bij de aard van dat type: het
is een bytepaar (de hoogste echte waarde `22624` = `0x5860` is 88 en 96, een schakeltijd in
kwartieren), en `0x8080` is dan "beide helften leeg". In de objecttabel staat bij die
registers letterlijk `maxValue: 32896`.

**Waarom de drempel veilig is.** Per type is het gat tussen de hoogste waargenomen echte
meting en de eerste ongeldige waarde minstens tienduizend:

| wtype | hoogste echte meting | gat tot de ongeldige waarde |
|---|---|---|
| 1 | 995 | 31.773 |
| 2 | 2.390 | 30.378 |
| 5 | 22.624 | 10.272 |
| 6 | 16.449 | 16.319 |
| 7 | 3.609 | 29.159 |
| 8 | 100 | 32.668 |

Over alle 681 registers geeft er precies **één** zowel waarden onder als boven de drempel:
WebID 60030 (een SG-Ready-ingang) stond 4 van de 9.695 keer op `32768`. Die vier metingen
waren ook werkelijk ongeldig. De regel kent dus geen tegenvoorbeeld.

Een waarde **boven** `65535` is juist geldig: dat is een 32-bits register (timestamps,
grote tellers), en daar klopt de `max` uit de tester niet.

### Regel 2 -- sensor ontbreekt of is defect

Staat er een echt bereik in de tester-uitvoer, dan is `waarde x schaal` buiten `[min, max]`
ongeldig.

Dit vangt wat regel 1 niet ziet: een niet-aangesloten NTC levert de ondergrens van zijn
meetbereik, en dat is een gewoon negatief getal. Zulke registers houden over tienduizenden
metingen exact dezelfde waarde -- een werkende sensor ruist altijd, dus een waarde die op
precies de bereikgrens vastligt meet niet.

**Let op de eenheden:** `min` en `max` in de tester-uitvoer zijn *geschaalde* grootheden,
de waarde is *ruw*. Vergelijken zonder te schalen levert tientallen valse alarmen.

### Wat niet nodig blijkt

- **Geen tabel met sentinelwaarden per register.** De drempel volstaat.
- **Geen uitzondering voor bitvelden.** Een bitveld zou bit 15 kunnen gebruiken en dan door
  regel 1 sneuvelen, maar dat gebeurt nergens: over alle registers is de hoogste
  gecombineerde bitmaske van waargenomen geldige waarden `0x4041`. Bit 14 wordt gebruikt,
  bit 15 nergens. De kolom `bitmasker_gezien` in de catalogus legt dat per register vast,
  zodat het controleerbaar blijft als er ooit een register bijkomt.
- **De veldbeschrijvingen helpen hier niet.** `presentation` en `step` uit de objecttabel
  scheiden geen bitvelden van getallen: `presentation 2` bevat temperaturen en
  energietellers, en `step` leeg is vooral de `wtype 5`-groep. Er is geen bitveld-vlag.

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
