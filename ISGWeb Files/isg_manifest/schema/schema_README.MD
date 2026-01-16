Dit project gebruikt een formeel JSON‑schema om de structuur van het register‑manifest te definiëren.
Het schema beschrijft alle velden, hun types, en de manier waarop optionele Modbus‑metadata wordt opgeslagen.

Het doel van schema.json is:

consistentie tussen tooling, documentatie en manifest

validatie van gegenereerde of handmatig bewerkte manifestbestanden

uitbreidbaarheid zonder breaking changes

duidelijkheid voor gebruikers en contributors

scheiding tussen ISG‑tester data en optionele Modbus‑informatie

Het manifest volgt een minimalistische architectuur:

elke entry vertegenwoordigt één ISG‑tester register

alle waarden zijn native (geen reconstructie, geen endianness buiten Modbus)

Modbus‑informatie is optioneel en alleen aanwezig voor officiële registers

alle adressen zijn 1‑based, zoals gedocumenteerd door Stiebel

Het schema valideert onder andere:

id (registernummer)

datatype (int16/uint16/int32/uint32)

unit

access (ro/rw)

min en max

category

show en analyze

optionele modbus‑sectie met:

address

type

datatype

span (1 of 2)

endianness (altijd "low_high")

Het schema is opgeslagen als: schema.json
en kan gebruikt worden met elke JSON‑schema validator om manifestbestanden te controleren.