# Heatpump Modbus Logger

Een snelle, robuuste en schaalbare **C#‑logger** voor het uitlezen van een **Stiebel Eltron / WPM / ISG** warmtepomp via **Modbus TCP**.  
De logger leest alle relevante registers uit, converteert waarden naar fysische eenheden en schrijft deze weg naar een CSV‑bestand voor verdere analyse, visualisatie of integratie in Home Assistant.

## Features

- Volledige Modbus‑uitlezing van ISG‑exposed registers  
- Correcte signed/unsigned interpretatie  
- Automatische scaling (°C, bar, l/min, kW, kWh, h)  
- CSV‑logging met vaste kolomstructuur  
- Thermisch consistente berekeningen  
  - ΔT  
  - Warmtevermogen (kW)  
  - COP  
- Stabiele werking zonder pendelen of timeouts  
- Compatibel met Home Assistant (waarden matchen 1‑op‑1)

## CSV‑kolommen

De logger schrijft per sample o.a.:

- `timestamp`  
- `flow_temp_c`  
- `deltaT_K`  
- `hotgas_c`  
- `water_pressure_bar`  
- `flow_l_min`  
- `hk1_set_c`, `hk1_flow_c`  
- `hk2_set_c`, `hk2_flow_c`, `hk2_return_c`  
- `outside_temp_c`  
- `room_temp_c`  
- `hotwater_set_c`, `hotwater_actual_c`  
- `compressor_return_c`, `compressor_flow_c`  
- `compressor_lowpress_bar`, `compressor_midpress_bar`, `compressor_highpress_bar`  
- `energy_heat_kwh`, `energy_water_kwh`, `energy_nhz_kwh`  
- `runtime_heat_h`, `runtime_water_h`, `runtime_cool_h`, `runtime_nhz_h`  
- `cop`  
- `heatOut_kW`  
- `powerIn_W`  
- `state`, `status`, `is_defrost`

Alle waarden zijn fysisch gecontroleerd en consistent met de warmtepomp.

## Installatie

1. Clone de repository  
2. Open het project in Visual Studio, VS Code of Rider  
3. Pas indien nodig de Modbus‑instellingen aan in `Program.cs`  
4. Build & run

## Modbus instellingen

- **Protocol:** Modbus TCP  
- **Addressing:** 0‑based (ISG‑mapping)  
- **Port:** 502  
- **Scaling:** per register gedefinieerd in de code  

De logger gebruikt een vaste mapping die 1‑op‑1 overeenkomt met Home Assistant.

## Waarom dit project?

De standaard ISG‑Modbus mapping is:

- slecht gedocumenteerd  
- inconsistent tussen WPM‑versies  
- deels 0‑based, deels 1‑based  
- met onduidelijke scaling  

Deze logger lost dat op door:

- een correcte registermapping  
- fysische validatie (flow × ΔT = vermogen)  
- CSV‑logging voor analyse en debugging  

## Toekomstige uitbreidingen

- JSON‑export  
- MQTT‑publish  
- Live grafieken  
- Automatische detectie van defrost‑cycli  
- Integratie met Home Assistant via autodiscovery  

## Licentie

MIT License — vrij te gebruiken, aan te passen en te delen.

