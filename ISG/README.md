# ISG — alles rond de Stiebel-gateway

Deze map bevat alles wat met de ISG te maken heeft: de bestanden die erop staan, wat we
eruit hebben gehaald, en wat we erover weten.

| map | wat erin staat | gaat naar het apparaat? |
|---|---|---|
| `firmware/` | onze eigen bestanden op het apparaat, **paden gespiegeld** — uitrollen is kopiëren | ja, dit *is* het apparaat |
| `staged/` | van ons, hoort op het apparaat, staat er nog niet | nog niet |
| `dumps/` | opgehaald ván het apparaat: objecttabellen, configtemplates, directory listings | nee |
| `kennis/` | wat we over het apparaat weten, met herkomst en status per gegeven — zie `kennis/README.md` | nee, nooit |

De scheiding is er met opzet: zodra er iets anders dan apparaatbestanden in `firmware/`
belandt, is de spiegel niets meer waard en kun je niet meer blind kopiëren.

Alles onder `firmware/` is **niet van Stiebel** — het is eigen werk.

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
