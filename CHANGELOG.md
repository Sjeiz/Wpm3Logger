# Changelog
Alle belangrijke wijzigingen in dit project worden in dit bestand gedocumenteerd.

Het formaat is gebaseerd op [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),  
en dit project volgt [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]
- Voorbereiding op volgende features
- Kleine optimalisaties en cleanup

---

## [1.0.0] – 2026-01-06
### Added
- Eerste publieke release van de **Heatpump Modbus Logger**
- Volledige Modbus‑mapping voor ISG/WPM
- Correcte scaling voor alle temperatuur-, druk-, energie- en runtime‑registers
- CSV‑logging met vaste kolomstructuur
- Berekening van:
  - ΔT  
  - Warmtevermogen (kW)  
  - COP  
- Detectie van warmtepompstatus (HEATING / DHW / DEFROST / OFF)
- Ondersteuning voor signed/unsigned registers
- Fysische validatie van flow × ΔT = vermogen
- Compatibiliteit met Home Assistant (waarden matchen 1‑op‑1)
- `.gitignore` toegevoegd voor C#‑projecten
- `.gitattributes` toegevoegd voor consistente line endings
- MIT‑licentie toegevoegd
- README toegevoegd

---

## [0.1.0] – 2026-01-05
### Added
- Eerste werkende prototype
- Basis Modbus‑uitlezing
- Ruwe CSV‑export
- Eerste mapping van registers
