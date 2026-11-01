# Werkwijze: hoe je iets over de ISG uitzoekt

Dit document bestaat om gepuzzel te voorkomen. De catalogus in deze map is op 01-11-2026
in een lange sessie opgebouwd, en een flink deel van die tijd ging naar omwegen die niet
nodig waren geweest. Hieronder staat wat werkt, in welke volgorde, en waar de valkuilen
zitten.

## De regel

**Bij twijfel over hoe het apparaat zich gedraagt: lees de Stiebel-code op het apparaat.
Daar moet het in staan.** Niet uit meetdata reconstrueren.

Dat is twee keer bewezen. De geldigheidsregel voor ongeldige waarden is eerst uit 5,49
miljoen waarnemingen afgeleid -- met vier tegenstrijdige sentinel-lijsten en een te ruime
drempel als resultaat -- terwijl `isg_object.php` de regel letterlijk bevat. Daarna
hetzelfde met de registernamen, die in een configuratiebestand staan dat al in deze repo lag.

Wat van een hobby-implementatie komt (onze HA-YAML, onze eigen manifesten) weegt lichter dan
de leverancierscode, want de vraag is "wat doet het apparaat".

## De bronnen, in volgorde van gezag

| bron | pad | wat je eruit haalt |
|---|---|---|
| de centrale logica | `www/isg_object.php` | de geldigheidstest, het dependency-mechanisme |
| de schaaltabel | `www/stablevalues.php` | `sf::normalize()`: schaal per webType, samengestelde types |
| het schrijfpad | `www/save.php` | hoe invoer omhoog geschaald en tegen min/max getoetst wordt |
| de presentatie | `www/_inc/preN.inc` | per `presentation`-waarde; `pre7.inc` bevat `0x9000` |
| de UI-boom | `WPM_3_config.json.template.txt` | naam, menupad, enum- en bitveldbetekenissen, `dependency` |
| de vertalingen | `www/obj/sprache/sprache` | 3468 paren; de taal van dit apparaat staat op Engels |
| het apparaat zelf | `bin/isg_tester -p` | live waarde, wtype, min, max, access, eenheid |

Kopieën van de eerste vier plus de UI-boom staan in `../dumps/`. De vertaaltabel staat
geparsed in `stiebel_vertalingen.csv`.

## Verbinden met het apparaat

    ssh -o HostKeyAlgorithms=+ssh-rsa,ssh-dss \
        -o PubkeyAcceptedKeyTypes=+ssh-rsa \
        -o KexAlgorithms=+diffie-hellman-group1-sha1,diffie-hellman-group14-sha1 \
        -o Ciphers=+aes128-cbc,3des-cbc \
        -i Secrets/id_rsa_ISGweb root@servicewelt.iot.cheizoo.lan

De oude crypto-opties zijn nodig: het is een BusyBox uit 2009.

## Herbruikbare toetsen

**De monotonie-toets -- teller of bitveld?** Een teller loopt alleen op. Zet het verloop van
het onbekende register naast een bekende teller en een bekend bitveld en tel de stappen
omhoog en omlaag. WebID 402 gaf 73 omhoog en 71 omlaag, net als het bekende bitveld 485
(72/74) en in niets als de bekende teller 1033 (7/0). Daarmee was "LAUFZEIT HEIZEN" weerlegd
voordat de broncode de echte naam gaf.

**De ruis-toets -- meet die sensor echt?** Een werkende sensor ruist altijd. Staat een
register over duizenden metingen op exact dezelfde waarde, en is die waarde precies de
ondergrens van het meetbereik, dan is de sensor niet aangesloten. Zo bleken `-400` (= -40,0
graden) bij WebID 18 en 558 geen metingen maar open circuits.

**Gedrag over bedrijfstoestanden.** `metingen/*.csv` bevat vier snapshots (idle, verwarmen,
warmwater, ontdooien). Een register dat in alle vier hetzelfde doet, doet niets.

**Gelijktijdig HA naast de tester.** Nuttig om de keten te verifieren
(entiteit -> modbus-adres -> Modicon -> WebID -> waarde), met
`Modicon = adres + 30001` voor input registers en `+ 40001` voor holding registers.
**Maar let op wat het bewijst:** alleen dat beide hetzelfde *ruwe* getal lezen -- niet dat
onze schaal in HA juist is. Zie de valkuilen.

## Valkuilen

Dit is de lijst die het meeste tijd had gescheeld.

**`isg_tester -r` geeft geen wtype en geen eenheid.** Alleen `-p` levert die kolommen. Een
enkel register opvragen is dus geen vervanging voor de volledige dump.

**`isg_tester -p` kost ~2 s en blokkeert alle PHP op het apparaat.** `lighttpd.conf` zet
`max-procs = 1` met `PHP_FCGI_CHILDREN = 0`, dus er is een worker. Tijdens de aanroep staat
de Service Welt stil. Dat begrenst het pollinterval.

**min/max betekenen iets anders per interface.** `isg_tester` geeft ze *geschaald* (5 tot 30
graden), de php_mapper die `save.php` gebruikt geeft ze *ruw*. Neem je `save.php`'s
vergelijking letterlijk over met grenzen uit de tester, dan weiger je elke schrijfactie.
Onze controle moet `waarde x schaal` tegen min/max zetten.

**De objecttabel-dump is een fabriekstemplate, geen toestand.** In
`../dumps/WPM_3_isg_objects.json` staan `value`, `timestamp` en `conditionID` bij **alle**
479 records op nul respectievelijk 32768. Je kunt er dus geen geldigheid uit afleiden; die
vraag is er principieel niet mee te beantwoorden.

**HA naast de tester bewijst de schaal niet.** Een gelijktijdige meting leek 51
schaalfactoren te bevestigen, maar toonde alleen dat beide hetzelfde ruwe getal lezen. Bij
tien registers (`webType 11`) heeft onze HA-config het mis en Stiebel het goed.

**De logbestanden hebben twee verschillende formaten.** `Python/MetDelta/*.csv.txt` is long
(`time,key,value`), `Python/MetDelta/Logging/*.csv.txt` is wide (`time,w1,w3,...`). Een
parser voor het ene formaat slaat het andere **stil** over -- vier logdagen zijn daardoor
eerst buiten beeld gebleven. De bestanden met `_kv` in de naam zijn derivaten van een enkele
dag, door `isg_logger.py` gefilterd; die mogen niet als onafhankelijke bron meetellen.

**De vertaaltabel is PHP-geserialiseerd en bevat meer dan strings.** Een regex over
`s:<len>:"..."` loopt uit de pas zodra er een integer tussen staat, en levert dan sleutels
op in plaats van vertalingen. Gebruik een echte unserialize. De PHP op het apparaat is de
CGI-variant en kent `-r` niet, dus parsen gaat lokaal.

**Kap bitveld- en enumdefinities niet af.** Ze stonden eerst ingekort in een vrij tekstveld;
daardoor leek WebID 402 negen bits te hebben in plaats van elf, en leken er 13 bitvelden te
zijn in plaats van 23. Ze staan nu volledig in `webid_codes.csv`.

**BusyBox v1.2.1 mist veel.** Geen `head -N` (gebruik `head -n N`), geen `sed -E`, geen
`grep -o`. `find -newermt` faalt **stil** en geeft een lege uitslag, wat eruitziet als "er is
niets".

**Het rootfs is read-only.** Alleen `/firmware` (de SD-kaart) is schrijfbaar. Zie
`../README.md` voor het remount-patroon in `start.sh` en voor wat een firmware-update
overleeft.

## Naambronnen, en waar ze ophouden

Er zijn drie Stiebel-bronnen voor namen, en ze overlappen en dekken samen niet alles:

| bron | dekking | aard |
|---|---|---|
| `WPM_3_config.json.template.txt` | 874 WebID's | de menuboom van de Service Welt |
| `www/obj/sprache/sprache` | 3468 termen | de Engelse vertaling daarvan |
| `firmware/rel_b/etc/config/WPM-isg-modbus.csv` | 63 WebID's | **derde kolom**, makkelijk te missen: de eerste regels zijn leeg |

Daarna is het op. Onderzocht en afgevallen:

- `loxwiki-extra-webids.json` in de repo (126 entries) is een afgeleide van Stiebels
  Modbus-documentatie: dezelfde registers die al in de mapping-CSV staan.
- `www/obj/help_txt_box/` op het apparaat bevat twee HTML-templates, geen helpteksten
  per register.
- De Modbus-handleiding als PDF levert met een eenvoudige stream-extractie geen bruikbare
  tekst op.

Voor de registers die in geen van de drie staan blijven twee wegen: de loxwiki online
(community-bron, dus een hypothese waard en geen bewijs), of zelf meten door het verloop te
correleren met een bekend signaal. De kolom `onderzoek` wijst aan welke dat waard zijn.

## Wat de catalogus wel en niet zegt

Volledig voor alle 681 registers: `wtype`, `access`, `min`, `max`, `schaal`, de geldigheid
en het gemeten gedrag. Die komen van het apparaat of uit Stiebels code.

Een **lege kolom is meestal een antwoord, geen gat**: een leeg eenheidsveld betekent dat dit
register geen eenheid heeft, en een leeg `modbus`-veld dat het niet in de Modbus-mapping
voorkomt. Dat laatste geldt voor 521 van de 681 registers, en is precies het argument voor
de overstap naar de directe route.

De kolom `naam_en_bron` houdt vast waar een naam vandaan komt: van Stiebel (372), een eigen
vertaling van een Duitse Stiebel-naam (41), of uit onze HA-config voor registers die Stiebel
niet benoemt (7). Alleen de eerste groep is hard.

De kolom `onderzoek` zegt per naamloos register of nader uitzoeken zin heeft.
