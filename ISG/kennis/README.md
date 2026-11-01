# ISG-kennis — wat we van het apparaat weten en hoe zeker dat is

Deze map bevat de geconsolideerde kennis over de Stiebel ISG-gateway. Het was verspreid over
zeven mappen, elk met een deel van het verhaal en onderling tegenstrijdig. Hier staat één
versie, met per gegeven de herkomst en de status.

**De spiegel blijft gescheiden.** `ISG/firmware/` is een exacte kopie van wat op het apparaat
staat — uitrollen is daar kopiëren. Deze map bevat kennis *over* het apparaat en gaat er nooit
naartoe. `ISG/dumps/` bevat wat we eruit hebben gehaald, `ISG/staged/` wat er nog heen moet.

---

## Geldigheid en schaal -- volgens Stiebels eigen code

We hebben dit eerst uit de data proberen af te leiden, met vier onderling tegenstrijdige
sentinel-lijsten als resultaat. Dat was de omweg. De gateway draait Stiebels eigen PHP, en
daar staat het antwoord letterlijk in.

**Bron:** `/firmware/rel_b/www/isg_object.php` en `/firmware/rel_b/www/stablevalues.php`
op het apparaat, plus `_inc/pre7.inc`. Die bestanden staan **niet in deze repo** -- het is
Stiebels auteursrechtelijk beschermde code en deze repo is publiek. Lees ze op het apparaat.

### Ongeldig

`isg_object.php` doet bij het klaarzetten van elke waarde voor de UI:

    als abs(waarde) in {32768, 3276832768}:
        toon '-'   (behalve bij presentation 7: toon abs(waarde))
    anders:
        normaliseer de waarde volgens het webType

Drie dingen vallen daaraan op:

- **`abs()` maakt het datatype irrelevant.** Of je `32768` of `-32768` leest doet niet mee.
  Dat is precies waarom de tester `32768` meldt waar HA via Modbus `-32768` laat zien: het
  is hetzelfde bitpatroon `0x8000`, en Stiebel vangt beide met een absolute waarde.
- **`3276832768` is de 32-bits variant**: decimaal "32768" tweemaal achter elkaar. Bij een
  32-bits register plakt de mapper de twee ongeldige helften aan elkaar.
- `0x8000` is `INT16_MIN`, de waarde waarop de firmware elk object initialiseert. In
  `../dumps/WPM_3_isg_objects.json` staat `value: 32768` bij alle 479 records. Een register
  dat de hardware nooit beschrijft blijft eropstaan.

Daarnaast behandelt `_inc/pre7.inc` `0x9000` als ongeldig -- daar staan de geschaalde vormen
hard in de code (`"3686.4"` is `36864 x 0,1`, `"-2867.2"` is `-28672 x 0,1`). Het staat dus
niet in de centrale lijst maar wel in de presentatielaag.

En bij de samengestelde types (zie hieronder) volgt de ongeldigheid uit het type zelf: een
bytehelft van `0x80` is geen geldige index. Dat is wat `32896` (`0x8080`) is: beide helften
leeg.

Samengevat, als een functie:

    isValid(v, webType):
        a = abs(v)
        als a in {32768, 36864, 3276832768}:            ongeldig
        als webType in {5, 9, 10}:
            als (a >> 8) >= 128 of (a & 255) >= 128:    ongeldig
        anders                                           geldig

**Kruiscontrole.** Die regel markeert op 01-11-2026 194 van de 681 registers als ongeldig.
Onafhankelijk daarvan laat de gedragsanalyse over 5,49 mln waarnemingen uit tien bronnen zien
dat precies 194 registers nooit een geldige waarde geven. Dezelfde 194.

### Schaal

`stablevalues.php` bevat `sf::normalize($value, $type)`, dat de schaal volledig uit het
webType bepaalt. Daarmee is de schaal voor **alle** 681 registers bekend, en zijn de 27
handmatig gekozen factoren in de Modbus-configuratie overbodig:

| webType | wat ermee gebeurt |
|---|---|
| 2, 4 | `/ 10` (een decimaal) |
| 7 | `/ 100` (twee decimalen) |
| 11 | `/ 1000` (drie decimalen) |
| 5 | `[v >> 8, v & 255]` -- een bytepaar, geen getal |
| 9 | `[v / 4, (v % 4) * 15]` -- kwartieren naar uur en minuut |
| 10 | twee van die paren: een start- en een stoptijd |
| 1, 3, 6, 8, 15 | geen schaling |

Dat verklaart ook de `wtype 5`-groep die eerder raadselachtig leek: de hoogste echte waarde
daar is `22624`, en dat is `[88, 96]` -- 22:00 en 24:00 in kwartieren. Een schakeltijdvenster,
geen meetwaarde.

**Let op bij `[min, max]`.** Die staan in de tester-uitvoer al in geschaalde eenheden, terwijl
de waarde ruw is. Vergelijken zonder eerst te schalen levert tientallen valse alarmen.

### Waar HA van Stiebel afwijkt

Bij tien registers gebruikt de Modbus-configuratie in HA een andere schaal dan Stiebel: alle
tien zijn `webType 11`, waar Stiebel `/1000` doet en HA `1.0`. De catalogus vermeldt dat per
register in `opmerking`.

Dat verschil is niet neutraal. Een eerdere gelijktijdige meting van HA naast de tester leek 51
schaalfactoren te "bevestigen", maar die toets laat alleen zien dat beide hetzelfde **ruwe**
getal lezen -- niet dat HA's schaal juist is. Waar de twee verschillen is Stiebels code
gezaghebbend en de HA-waarde eigen werk.

---

## Namen, categorieen en enums -- ook uit Stiebels eigen bestanden

Werkregel die deze sessie opleverde: **bij twijfel naar de Stiebel-broncode, daar moet het
in staan.** Dat gold voor de geldigheidsregel en het gold opnieuw voor de namen, die we
eerst moeizaam uit eigen meetwerk probeerden te reconstrueren.

**`../dumps/WPM_3_config.json.template.txt`** is de configuratie waarmee `isg_object.php` de
Service Welt opbouwt. Die bevat voor 874 WebID's:

- de naam en de plaats in de menuboom (bruikbaar als categorie)
- `options`: de **enum- en bitveldbetekenissen**, per waarde of per bit
- `dependency`: het WebID dat bepaalt of dit register zichtbaar is -- het conditie-mechanisme
  waar `isg_object.php::dependencywebidcheck()` op werkt (30 registers hebben er een)
- `accessRight` en `pageType`

**`/firmware/rel_b/www/obj/sprache/sprache`** is een PHP-geserialiseerd object met 3468
vertaalparen; de taal op dit apparaat staat op Engels. De catalogus gebruikt die voor de
kolom `naam_en` en houdt het Duitse origineel in `naam_de`. Let op: het bestand bevat ook
andere typen dan strings, dus een regex over `s:<len>:"..."` loopt uit de pas -- gebruik een
echte unserialize.

### Waar dit eerdere conclusies omgooit

**WebID 402 is `PUMPENSTATUS` (Engels: PUMP), een bitveld met negen benoemde bits:**
B0 buffer charging pump 1, B1 buffer charging pump 2, B2 source pump, B3 solar circuit pump,
B4 htg circ pump 1, B5 htg circ pump 2, B6 DHW charging pump, B7 mixer open, B8 Mischer-Z.

Dat is lang onbeslist geweest: `metingen/*.csv` noemt het "LAUFZEIT HEIZEN" met de
aantekening dat het empirisch is vastgesteld. Twee onafhankelijke argumenten weerleggen dat.
De broncode geeft de naam en de bits. En een looptijdteller loopt alleen op: over 1226
metingen op een dag gaat WebID 402 73 keer omhoog en **71 keer omlaag**, net als het bekende
bitveld 485 (72 omhoog, 74 omlaag), terwijl de bekende teller 1033 7 keer omhoog en 0 keer
omlaag gaat. Die monotonie-toets is herbruikbaar voor elk register waarvan de aard onduidelijk
is.

### Het schrijfpad, volgens `save.php`

Stiebel doet bij het opslaan van een waarde precies het omgekeerde van het lezen:

    schaal de invoer OMHOOG volgens webType (x10, x100, x1000)
    accepteer als  waarde >= minValue en waarde <= maxValue
    of als presentation == 7 en de waarde -28672 of 36864 is  (0x9000, de 'uit'-stand)

Nuttig voor fase 3: de bereikcontrole die wij willen bouwen bestaat dus al, en de uitzondering
voor `presentation 7` laat zien dat de 'uit'-stand van die registers geschreven moet kunnen
worden.

**Pas op met min/max uit twee interfaces.** Gemeten: de tester en de objecttabel geven
identieke min/max (56 registers, geen afwijking), en de **geschaalde** waarde valt daar
binnen (40 van 41) terwijl de ruwe dat niet doet (1 van 41). Voor ons, die `isg_tester`
gebruiken, zijn min/max dus geschaald. `save.php` vergelijkt tegen een ruwe min/max uit de
php_mapper -- een andere interface met een andere conventie. Niet verwarren.

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

## Aangetoonde fouten

Niet alles in dit dossier is samenvoegen; een deel is rechtzetten. Deze fouten zijn met
meetdata of met de broncode aangetoond, niet vermoed.

### In Stiebels eigen materiaal

- `StiebelDoc_modbus.json` bevat een record met `unit: "6"` -- een getal waar een eenheid
  hoort. Een keer in 331 records.
- De live configuratie wijst **WebID 1123** aan als `VERDICHTER` onder STARTS, maar dat
  register staat op 0; de waarde die de Service Welt toont (625) komt uit **1122**.
- `_inc/pre7.inc` toetst op ongeldige waarden met een **stringvergelijking op geschaalde
  getallen** (`"3686.4"`, `"-2867.2"`). Dat breekt zodra de schaalfactor van dat type
  verandert, en het staat los van de centrale test in `isg_object.php`.
- `0x9000` wordt niet centraal als ongeldig behandeld, `0x8000` wel. De regel zit dus op
  twee plaatsen met twee verschillende mechanismen.
- De `max` uit `isg_tester -p` klopt niet voor 32-bits registers: WebID 60018 meldt
  `max 65535` en levert waarden van 1,77 miljard.

### In ons eigen werk van januari 2026, nu gecorrigeerd

- **21 modbus-adressen** in de afgeleide tabellen wezen naar het verkeerde WebID. Zeven
  daarvan zijn met waarden hard weerlegd; het scherpst bij 30515 en 30516, waar de kolom
  precies een positie verschoven stond. De overige zeven betreffen duplicaatregisters met
  dezelfde waarde en zijn daarmee risicoloos.
- **WebID 402** stond als "LAUFZEIT HEIZEN" met de aantekening dat het empirisch was
  vastgesteld. Het is `PUMPENSTATUS`, een bitveld met elf benoemde pompen en mengers.
- **`scaling`** oogde als vaststelling maar was voor 91% de default `1.0`: van de registers
  met een naam had 32% een niet-triviale schaal, van de naamloze 0,2%.
- **`-200` en `-400`** stonden als universele sentinel in `isg_logger_delta_engine.py`. Het
  zijn de bereikgrenzen van niet-aangesloten sensoren, en dus een eigenschap van het
  register -- bij een werkende sensor is `-40,0` graden een geldige meting.
- `isg_json_exporter.sh` schreef de slotregel `object_count: 681` weg als WebID met lege
  velden, waardoor beide JSON-dumps ongeldig waren.

### In de Modbus-configuratie van Home Assistant

- Tien registers van `wtype 11` staan op `scale: 1.0` waar `sf::normalize` door 1000 deelt.
  Dat zijn de energietellers; zie de paarstructuur hierboven.

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
