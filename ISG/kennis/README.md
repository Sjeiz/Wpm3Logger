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

**Bron:** `isg_object.php`, `stablevalues.php` en `_inc/pre7.inc`. Die staan als referentie in
`../dumps/stiebel_www/`, met het origineel op het apparaat onder `/firmware/rel_b/www/`. Het is
Stiebel-code; wij hebben hem niet geschreven en wijzigen hem niet.

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

**Kruiscontrole.** De regel markeert op een momentopname een aantal registers als onbruikbaar.
Onafhankelijk daarvan wijst de gedragsanalyse over alle beschikbare meetdata aan welke registers
nooit een geldige waarde geven. Dat zijn **exact dezelfde registers** -- twee methoden die elkaar
niet kennen, met dezelfde uitkomst.

### Wat een onbruikbare waarde precies betekent

Niet elke waarde die geen meting is, betekent hetzelfde. De code onderscheidt er drie, en de
catalogus doet dat nu ook in de kolom `geldig_01nov`:

| waarde | geldt bij | betekenis | verandert het ooit? |
|---|---|---|---|
| `0x8000` | elk type | **niet aanwezig** -- deze installatie heeft die functie of hardware niet; het object staat nog op zijn initiele waarde | nee, niet zonder verbouwing |
| `0x9000` | `presentation 7` | **uitgeschakeld** -- de functie bestaat wel, maar staat bewust uit. `_inc/pre7.inc` toont die waarde als *AUS* | ja, zodra je hem aanzet |
| `0x8080` | `webType 5` | **leeg tijdvenster** -- er is geen schakeltijd ingesteld; beide bytehelften staan op `0x80` | ja, zodra je een tijd instelt |

Dat onderscheid is geen haarkloverij. `0x8000` bij WebID 455 (`WW-PWM`) betekent dat je toestel
die regeling niet heeft; `0x9000` bij WebID 43 (`EINSATZGRENZE HZG`) betekent dat de inzetgrens
bestaat en door jou is uitgezet. Het eerste hoeft nooit een entiteit in HA, het tweede is een
instelling die je wilt kunnen zien en zetten.

**En een vierde geval dat niet uit de waarde af te leiden is.** Een instelling kan op de rand van
zijn bereik staan en daarmee functioneel "uit" betekenen, terwijl het technisch een geldige waarde
is. WebID 41 en 42 (`BIVALENZTEMPERATUR HZG/WW`) staan op `-20,0`, het minimum van hun bereik: de
bijverwarming schakelt dus nooit in.

Een algemene regel daarvoor bestaat niet, en die moet je ook niet verzinnen. Van de schrijfbare
registers staan er tientallen op hun minimum of maximum, met heel verschillende betekenis:
`ANTILEGIONELLENBEHANDLUNG` op 0 is uit, maar `WAERMEPUMPE AUS` op 0 betekent juist dat de
warmtepomp **niet** uit staat, en `TAG` op 1 is gewoon de eerste van de maand. Dit is dus een
interpretatie per register, niet iets wat de PHP kan afleiden.

**Kan een register van geldig naar onbruikbaar gaan?** Ja, maar zelden. Over alle waarnemingen
deed precies een register dat: een SG-Ready-ingang die af en toe op `0x8000` stond en verder
gewoon 0 of 1 gaf -- tijdelijk niet leesbaar, geen verdwenen hardware.

Belangrijk daarbij: een functie die je **uitzet** gaat niet naar `0x8000` maar naar de uit-waarde
van zijn type. Dat volgt uit `pre7.inc`, niet uit een meting -- en dat kan ook niet, want die
overgang zit niet in de waarneming.

**Wat de meetdata wel en niet kan zeggen.** De waarnemingen komen uit januari en februari 2026 plus
een momentopname in november. Dat zijn vensters, geen doorlopende reeks, en in die periode zijn de
installatie-instellingen niet aangepast. Een register dat in maart van toestand veranderde, of dat
alleen in de zomer beweegt, of alleen tijdens een storing, ziet er in deze data uit als onveranderd.

Lees de kolom `gedrag_gemeten` dus als **"in de waargenomen vensters"** en niet als een uitspraak
over het register als zodanig. "Nooit beschreven" is sterk bewijs dat een functie ontbreekt wanneer
het samenvalt met `0x8000`, maar "constant" is dat niet: dat kan net zo goed een instelling zijn
die in die periode niet is aangeraakt.

De koppeling moet er dus op voorbereid zijn dat een register onbruikbaar wordt zonder dat er iets
aan de installatie is veranderd, ook al gebeurt het bijna nooit.

**Een vijfde geval, voor de volledigheid:** een register kan geldig zijn en toch niets zeggen
omdat het nooit beweegt -- een ongebruikte functie. Dat is alleen aan het gedrag over tijd te
zien, vandaar de kolom `gedrag_gemeten`.

### Wat de Service Welt wel en niet toont

De web-UI bouwt zich op uit de configuratie en rendert per register het bestand
`_inc/pre<presentation>.inc`. Bestaat dat bestand niet, dan wordt er niets getoond.

Dat heeft twee gevolgen die het nemen van de directe route onderbouwen:

- Een deel van de registers in de configuratie heeft `presentation 0`, en een `pre0.inc` bestaat
  niet. Die worden dus overgeslagen. Geen verlies: geen ervan beweegt.
  *Waarom Stiebel daar geen handler voor heeft, weten we niet.* Afgeschermd zijn ze niet -- ze
  hebben dezelfde `accessRight` als de meeste registers die wel getoond worden. Het is dus geen
  uitspraak over hun bruikbaarheid, alleen over wat de UI rendert.
- Zwaarwegender: **een groot deel van de registers staat helemaal niet in de UI-configuratie.**
  Daarvan bewegen er tientallen, waaronder de volledige energietellerreeks op installatieniveau.

De Service Welt laat dus een subset zien, en mist daarbij registers die wel degelijk meten. De
tester geeft ze alle. Dat is precies de reden dat de koppeling niet via de web-UI loopt maar via
`isg_tester`.

Zie `webid_catalogus.csv` voor welke registers het betreft; filter op een gevulde `naam` met een
`gedrag_gemeten` dat met "beweegt" begint.

### accessRight -- bruikbaar voor de schrijf-whitelist

De configuratie geeft per register een `accessRight`. Het is geen zichtbaarheidsvlag maar een
toegangsniveau, en het loopt gelijk op met hoe ingrijpend een instelling is:

| waarde | wat voor registers |
|---|---|
| 1 | gewone bediening -- bedrijfsmodus, gewenste temperaturen |
| 2 | ingrijpend -- hier zitten de reset-registers |
| 4 | installateurswaarden -- bivalentietemperatuur, inzetgrenzen, ontwerptemperatuur |

Voor fase 3 is dat een bruikbaar filter: een whitelist die alleen niveau 1 toelaat sluit de resets
en de installateursinstellingen vanzelf uit, zonder dat we een lijst met uitzonderingen hoeven bij
te houden.

Het staat per register in de catalogus, kolom `toegangsniveau`, naast `access`. Die twee zeggen
iets anders: `access` is wat het apparaat technisch toestaat (RO, RW, CRW), `toegangsniveau` is
voor wie de instelling bedoeld is. Registers die niet in de UI-configuratie voorkomen hebben geen
niveau; daar zegt het veld dus niets, en dat is iets anders dan niveau 1.

### Wat dat betekent voor de koppeling

De PHP hoeft alleen de eerste drie te kennen; die volgen uit waarde en type. Of hij het verschil
ook moet **melden** is een ontwerpkeuze: je kunt alles wat geen meting is uit de payload weglaten,
waarna de HA-sensor vanzelf `unavailable` wordt. Dat is het eenvoudigst en dekt alle gevallen.

Een reden-code wordt pas nuttig als je onderscheid wilt tussen "was er nooit" en "is net
weggevallen" -- bijvoorbeeld om op uitval te alarmeren. In de gemeten periode wisselde geen enkel
register tussen geldig en ongeldig, dus dringend is het niet.

### Schaal

`stablevalues.php` bevat `sf::normalize($value, $type)`, dat de schaal volledig uit het
webType bepaalt. Daarmee is de schaal voor **alle** registers bekend, en zijn de handmatig
gekozen factoren in de Modbus-configuratie overbodig:

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

## Fouten in Stiebels materiaal

Deze zitten er nog in en kunnen je werk raken. Elk met meetdata of broncode aangetoond.

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
