# ISG — eigen bestanden op de gateway

Alles in deze map staat op de Stiebel ISG en is **niet van Stiebel**. De paden zijn
gespiegeld: het pad onder `ISG/` is het pad op het apparaat. Deployen is kopiëren.

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
