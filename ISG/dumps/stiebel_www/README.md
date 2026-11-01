# Stiebel-broncode van de gateway — referentie

Deze bestanden komen van `/firmware/rel_b/www/` op de ISG en zijn **van Stiebel Eltron**,
niet van ons. Ze staan hier als referentie, om twee redenen:

1. Ze bevatten het gezaghebbende antwoord op hoe het apparaat geldigheid en schaling
   bepaalt — zie `../../kennis/README.md`, dat deze code citeert en uitlegt.
2. Een firmware-update vervangt `rel_b/` volledig. Met deze kopie kun je na een update
   zien óf en wat er veranderd is.

## Waarom niet onder `ISG/firmware/`

Die map is de deploy-spiegel: wat daarin staat wordt bij een herstel teruggekopieerd naar
het apparaat. Stiebel-code hoort daar niet in — een firmware-update zet die zelf terug, en
terugkopiëren zou een nieuwere versie met een oudere kunnen overschrijven.

## Wat waarin zit

| bestand | waarvoor het hier staat |
|---|---|
| `isg_object.php` | de centrale geldigheidstest: `abs(waarde) in {32768, 3276832768}` |
| `stablevalues.php` | `sf::normalize()` — de schaal per webType, en de samengestelde types |
| `save.php` | de schrijfkant: schaalt invoer omhoog en toetst tegen min/max |
| `_inc/presentation.inc` | kiest op grond van `presentation` welke `preN.inc` rendert |
| `_inc/pre7.inc` | behandelt `0x9000` als ongeldig, met de geschaalde vormen hard in de code |
| `_inc/pre2.inc` … `pre8.inc`, `pre22.inc` | de overige presentatiehandlers |

Niets hiervan is gecontroleerd op secrets-gevoelige inhoud bevonden; de bestanden zijn
gelezen voordat ze hier zijn geplaatst.

## De configuratiebestanden ernaast

| bestand in `../` | wat het is |
|---|---|
| `config.json.template` | de echte template uit `rel_b/etc/config/`, 14 kB -- generiek voor alle toestellen |
| `config_live_2026-11-01.json` | de **live** configuratie van dit toestel, 467 kB, 554 WebID's |
| `WPM_3_config.json.template.txt` | een eerdere live config (januari 2026), ondanks de naam |

De live configuratie is wat `isg_object.php` inleest via `$GLOBALS['json_template']`, en
hij bepaalt welke registers de Service Welt toont en onder welke naam. Hij staat op
`/var/volatile/cache/config.json`, en **`/var/volatile` is tmpfs**: na een reboot is dat
bestand weg tot de firmware het opnieuw genereert. Daarom staat hier een kopie met datum.
