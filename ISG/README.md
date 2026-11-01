# ISG — alles rond de Stiebel-gateway

Deze map bevat alles wat met de ISG te maken heeft: de bestanden die erop staan, wat we
eruit hebben gehaald, en wat we erover weten.

| map | wat erin staat | gaat naar het apparaat? |
|---|---|---|
| `firmware/` | **alleen onze eigen bestanden**, paden gespiegeld -- uitrollen is kopieren | ja, dit *is* het apparaat |
| `dumps/` | opgehaald van het apparaat: Stiebel-code, configuraties, objecttabellen | nee |
| `kennis/` | wat we over het apparaat weten, met herkomst per gegeven -- zie `kennis/README.md` | nee, nooit |

De scheiding is er met opzet. In `firmware/` staat uitsluitend wat **wij** hebben
toegevoegd of gewijzigd, vastgesteld door de twee firmwareslots te vergelijken: wat in
`rel_b` staat maar niet in `rel_a`, is van ons. Op die manier bleek `start.sh` ons
SSH-blok te bevatten en bleek `isg_api.php` eigen werk.

Stiebel-bestanden horen daar **niet** in, ook niet als we ze nodig hebben: een
firmware-update zet die zelf terug, en terugkopieren uit deze repo zou een nieuwere
versie met een oudere kunnen overschrijven. Ze staan in `dumps/`.


## Waarom dit hier staat

Het rootfs van de ISG is read-only (`/dev/root on / type jffs2 (ro)`). Alleen
`/firmware` is schrijfbaar — dat is de SD-kaart (ext3, `/dev/mmcblk0p1`). Alles wat
hier staat leeft dus op één SD-kaart in een verwarmingsgateway, zonder versiebeheer
op het apparaat zelf. Deze map is het enige exemplaar dat een kaartstoring overleeft.

## Wat een firmware-update overleeft

| | |
|---|---|
| `firmware/` | blijft staan bij een update |
| `firmware/rel_b/` | is het actieve firmwareslot en wordt bij een update vervangen |

Na een firmware-update moet alles onder `rel_b/` dus opnieuw geplaatst worden —
`start.sh` als eerste, want daar hangt de SSH-toegang aan.

## Hoe start.sh wordt aangeroepen

`/etc/rc5.d/S99fwboot` → `/usr/fwsystem/fwboot.sh` → het actieve slot, waar het
`etc/init.d/start.sh` uitvoert (de naam komt uit `GF_FW_StartScript` in
`/usr/fwsystem/fwglobalenv`). Dat script is daarmee het enige uitbreidingspunt in de
boot dat op schrijfbare opslag ligt.

Het is ook de plek waar het read-only rootfs kort open gaat: `remount rw` →
bestanden naar `/etc` kopiëren → `remount ro`. Zo komen de SSH-config en de
geautoriseerde sleutel op hun plek. Een configbestand dat in `/etc` moet staan en een
reboot moet overleven, gaat via dat blok.

## Niet in deze map

De SSH-sleutels horen bij de secrets en blijven buiten deze repo — die is publiek.
Zie `.gitignore`.
