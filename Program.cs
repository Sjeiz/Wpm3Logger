using System;
using System.Globalization;
using System.IO;
using System.Net.Sockets;
using System.Text.Json;
using System.Threading.Tasks;
using NModbus;

class Program
{
    static async Task Main()
    {
        Console.WriteLine("WPM3 / ISG Modbus logger gestart.");

        string baseDir = AppContext.BaseDirectory;
        string configPath = Path.Combine(baseDir, "config.json");
        string currentHour = "";
        string hourlyCsvPath = "";

        if (!File.Exists(configPath))
        {
            Console.WriteLine($"❌ Config file not found at: {configPath}");
            return;
        }

        var json = File.ReadAllText(configPath);
        var config = JsonSerializer.Deserialize<LoggerConfig>(json)
                     ?? throw new Exception("Config kon niet worden geladen.");

        Console.WriteLine($"IP: {config.Ip}, Port: {config.Port}, SlaveId: {config.SlaveId}");
        Console.WriteLine($"Poll interval: {config.PollSeconds}s, Retentie: {config.DaysToKeep} dagen");

        string logDir = string.IsNullOrWhiteSpace(config.LogDirectory)
            ? baseDir
            : config.LogDirectory;

        Console.WriteLine($"Log directory: {logDir}");
        Directory.CreateDirectory(logDir);

        string currentDate = "";
        string currentCsvPath = "";
        string errorLogPath = Path.Combine(logDir, "error_log.txt");

        TcpClient? client = null;
        IModbusMaster? master = null;

        while (true)
        {
            try
            {
                // Reconnect indien nodig
                if (client == null || !client.Connected)
                {
                    Console.WriteLine("🔌 Verbinden met WPM3/ISG...");

                    client?.Dispose();
                    client = new TcpClient();
                    client.Connect(config.Ip, config.Port);

                    var factory = new ModbusFactory();
                    master = factory.CreateMaster(client);

                    Console.WriteLine("✅ Verbonden.");
                }

                // Uurlijkse CSV rotatie
                string hourKey = DateTime.UtcNow.ToString("yyyy-MM-dd_HH");

                if (hourKey != currentHour)
                {
                    currentHour = hourKey;
                    hourlyCsvPath = Path.Combine(logDir, $"Wpm3Logger_{hourKey}_00.csv");

                    if (!File.Exists(hourlyCsvPath))
                    {
                        string header = string.Join(",",
                            "timestamp",
                            "flow_temp_c",
                            "deltaT_K",
                            "hotgas_c",
                            "water_pressure_bar",
                            "flow_l_min",
                            "hk1_set_c",
                            "hk1_flow_c",
                            "hk2_set_c",
                            "hk2_flow_c",
                            "hk2_return_c",
                            "outside_temp_c",
                            "room_temp_c",
                            "hotwater_set_c",
                            "hotwater_actual_c",
                            "compressor_return_c",
                            "compressor_flow_c",
                            "compressor_lowpress_bar",
                            "compressor_midpress_bar",
                            "compressor_highpress_bar",
                            "energy_heat_kwh",
                            "energy_water_kwh",
                            "energy_nhz_kwh",
                            "runtime_heat_h",
                            "runtime_water_h",
                            "runtime_cool_h",
                            "runtime_nhz_h",
                            "cop",
                            "dFlow",
                            "dHotgas",
                            "sg_ready",
                            "status",
                            "state",
                            "is_defrost",
                            "heatOut_kW",
                            "powerIn_W"
                        );

                        File.WriteAllText(hourlyCsvPath, header + Environment.NewLine);
                    }
                }

                // Snapshot
                await TakeSnapshot(master!, hourlyCsvPath, errorLogPath, config);                // Opschonen oude files
                CleanupOldFiles(logDir, config.DaysToKeep);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ Fout in hoofdloop: {ex.Message}");
                LogError(errorLogPath, $"Hoofdloop-fout: {ex}");

                client?.Dispose();
                client = null;
                master = null;
            }

            await Task.Delay(config.PollSeconds * 1000);
        }
    }

    // ---------------------------------------------------------
    // Snapshot
    // ---------------------------------------------------------

    static async Task TakeSnapshot(IModbusMaster master, string csvPath, string errorLogPath, LoggerConfig config)
    {
        try
        {
            byte slave = config.SlaveId;
            Console.WriteLine("📡 Snapshot...");

            // Modbus registers are shown 0-based here.
            // Empirisch bepaald, uit C# modbus mapping

            /* Het is een bekend en gedocumenteerd fenomeen dat:
            WPM3 / WPM‑familie (de warmtepompregelaar)
            → Vorlauf = aanvoer naar het afgiftesysteem  
            → Rücklauf = retour uit het afgiftesysteem
            maar dat:
            ISG‑API (de webinterface)
            → Vorlauftemperatur WP1 = aanvoer UIT de compressor, richting binnenunit  
            → Rücklauftemperatur WP1 = retour NAAR de compressor
            */

            //
            // TEMPERATUREN
            //
            double roomTemp       = ReadScaled(master, slave, 502, 0.1); //done: webid 21 RAUMTEMPERATUR IST, modbus 30503 ISTTEMPERATUR FEK
            double roomSet        = ReadScaled(master, slave, 503, 0.1); //done: webid 25 RAUMTEMPERATUR SOL, modbus 30504 SOLLTEMPERATUR FEK
            double outsideTemp    = ReadSigned(master, slave, 506, 0.1); //done: webid 27 AUSSENTEMPERATUR, modbus 30507 AUSSENTEMPERATUR

            double hk1Flow        = ReadScaled(master, slave, 507, 0.1); //done: webid 118 ISTTEMPERATUR HK1, modbus 30508 ISTTEMPERATUR HK1
            double hk1Set         = ReadScaled(master, slave, 509, 0.1); //done: webid 34 SOLLTEMPERATUR HK1, modbus 30510 SOLLTEMPERATUR HK1

            double hk2Flow        = ReadScaled(master, slave, 510, 0.1); //done: webid 33 ISTTEMPERATUR_HK2, modbus 30511 ISTTEMPERATUR_HK2
            double hk2Set         = ReadScaled(master, slave, 511, 0.1); //done: webid 26 SOLLTEMPERATUR HK 2, modbus 30512 SOLLTEMPERATUR HK 2

            // HK2 RETOUR – deze wordt WÉL gelogd
            double hk2Return      = ReadScaled(master, slave, 515, 0.1); //done: webid 37 RUECKLAUFTEMPERATUR_WP1, modbus 30516 RUECKLAUFTEMPERATUR_WP1

            double flowTemp       = ReadScaled(master, slave, 514, 0.1); //done: webid 36 VORLAUFISTTEMPERATUR_WP1, modbus 30515 VORLAUFISTTEMPERATUR_WP1

            double hotwaterActual = ReadScaled(master, slave, 521, 0.1); //done: webid 24 ISTTEMPERATUR WARMWASSER, modbus 30515 ISTTEMPERATUR WARMWASSER
            double hotwaterSet    = ReadScaled(master, slave, 522, 0.1); //done: webid 1001 SOLLTEMPERATUR WARMWASSER, modbus 30523 SOLLTEMPERATUR WARMWASSER

            //
            // COMPRESSOR
            //
            double compressorReturn = ReadScaled(master, slave, 541, 0.1); //unsure: modbus 30542 RUECKLAUFTEMPERATUR WP1
            double compressorFlow   = ReadScaled(master, slave, 542, 0.1); //unsure: modbus 30543 VORLAUFISTTEMPERATUR WP1
            double hotgas           = ReadScaled(master, slave, 543, 0.1); //done: webid 1030 HEISSGASTEMPERATUR WP1, modbus 30544 HEISSGASTEMPERATUR WP1

            double compLow          = ReadScaled(master, slave, 544, 0.01); //done: webid 1032 DRUCK NIEDERDRUCK WP1, modbus 30545 DRUCK NIEDERDRUCK WP1
            double compMid          = ReadScaled(master, slave, 545, 0.01); //done: webid 1076 DRUCK MITTELDRUCK WP1, modbus 30546 DRUCK MITTELDRUCK WP1
            double compHigh         = ReadScaled(master, slave, 546, 0.01); //done: webid 1031 DRUCK HOCHDRUCK WP1, modbus 30546 DRUCK HOCHDRUCK WP1

            //
            // HYDRAULIEK
            //
            double waterPressure    = ReadScaled(master, slave, 519, 0.01);  //done: webid 435 WASSERDRUCK, modbus 30520 WASSERDRUCK
            double flowRate         = ReadScaled(master, slave, 520, 0.01);  //unsure: modbus 30521 VOLUMENSTROM or modbus 30548 WP WASSERVOLUMENSTROM WP1`?

            //
            // ENERGIE (32-bit)
            //
            uint energyHeat         = ReadUInt32(master, slave, 3501); //unsure: webid 501 VERBRAUCH HEIZUNG, modbus 33502 VERBRAUCH HEIZUNG
            uint energyWater        = ReadUInt32(master, slave, 3504); //unsure: webid 502 VERBRAUCH WARMWASSER, modbus 33505 VERBRAUCH WARMWASSER
            uint energyNhz          = ReadUInt32(master, slave, 3506); //unsure: webid 503 VERBRAUCH NHZ, modbus 33507 VERBRAUCH NHZ

            //
            // RUNTIME (16-bit)
            //
            uint runtimeHeat        = master.ReadInputRegisters(slave, 3538, 1)[0]; //unsure: webid 551 LAUFZEIT HEIZUNG, modbus 33539 LAUFZEIT HEIZUNG
            uint runtimeWater       = master.ReadInputRegisters(slave, 3541, 1)[0]; //unsure: webid 552 LAUFZEIT WARMWASSER, modbus 33542 LAUFZEIT WARMWASSER
            uint runtimeCool        = master.ReadInputRegisters(slave, 3544, 1)[0]; //unsure: webid 553 LAUFZEIT KUEHLUNG, modbus 33545 LAUFZEIT KUEHLUNG
            uint runtimeNhz         = master.ReadInputRegisters(slave, 3545, 1)[0]; //unsure: webid 554 LAUFZEIT NHZ, modbus 33546 LAUFZEIT NHZ

            //
            // STATUS
            //
            ushort sgReady          = master.ReadHoldingRegisters(slave, 4000, 1)[0]; //unsure: modbus 4001 SG READY
            ushort statusBits       = master.ReadInputRegisters(slave, 2500, 1)[0]; //done: webid 485 STATUS, modbus 32501 STATUS. BITMAP = "HK 1 PUMPE":B0, "HK 2 PUMPE":B1, "AUFHEIZPROGRAMM":B2, "NHZ STUFEN IN BETRIEB":B3, "WP IM HEIZBETRIEB":B4, "WP IM WARMWASSERBETRIEB":B5, "VERDICHTER IN BETRIEB":B6, "SOMMERBETRIEB AKTIV":B7, "KUEHLBETRIEB AKTIV":B8, "MIN. EINE IWS IM ABTAUBETRIEB":B9, "SILENTMODE 1 AKTIV":B10, "SILENTMODE 2 AKTIV (WP AUS)":B11
            string state            = DecodeState(statusBits);

            //
            // ΔT (compressorzijde)
            //
            double deltaT = compressorFlow - compressorReturn;

            //
            // COP via Home Assistant vermogen
            //
            double? powerIn = await GetHomeAssistantPower(config);
            double heatOut = deltaT * flowRate * 0.06977; // kW

            string copStr = "N/A";
            if (powerIn.HasValue && powerIn.Value > 0)
            {
                double cop = heatOut / (powerIn.Value / 1000.0);
                copStr = F(cop);
            }

            //
            // DEFROST detectie
            //
            bool isDefrost = deltaT < 0 ||
                            (copStr != "N/A" &&
                            double.Parse(copStr, CultureInfo.InvariantCulture) < 0);

            //
            // DELTA LOGGING
            //
            double dFlow   = lastFlow.HasValue   ? flowTemp - lastFlow.Value   : 0;
            double dHotgas = lastHotgas.HasValue ? hotgas - lastHotgas.Value : 0;

            lastFlow = flowTemp;
            lastHotgas = hotgas;

            //
            // CSV REGEL
            //
            string timestamp = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ");

            string line = string.Join(",",
                timestamp,
                F(flowTemp),
                F(deltaT),
                F(hotgas),
                F(waterPressure),
                F(flowRate),
                F(hk1Set),
                F(hk1Flow),
                F(hk2Set),
                F(hk2Flow),
                F(hk2Return),
                F(outsideTemp),
                F(roomTemp),
                F(hotwaterSet),
                F(hotwaterActual),
                F(compressorReturn),
                F(compressorFlow),
                F(compLow),
                F(compMid),
                F(compHigh),
                FU(energyHeat),
                FU(energyWater),
                FU(energyNhz),
                FU(runtimeHeat),
                FU(runtimeWater),
                FU(runtimeCool),
                FU(runtimeNhz),
                copStr,
                F(dFlow),
                F(dHotgas),
                sgReady.ToString(CultureInfo.InvariantCulture),
                statusBits.ToString(CultureInfo.InvariantCulture),
                state,
                isDefrost ? "1" : "0",
                F(heatOut),
                powerIn.HasValue ? F(powerIn.Value) : "N/A"
            );

            await File.AppendAllTextAsync(csvPath, line + Environment.NewLine);

            Console.WriteLine("✅ Snapshot OK");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"❌ Snapshot-fout: {ex.Message}");
            LogError(errorLogPath, $"Snapshot-fout: {ex}");
            throw;
        }
    }


    // ---------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------

    static async Task<double?> GetHomeAssistantPower(LoggerConfig config)
    {
        try
        {
            using var client = new HttpClient();
            client.DefaultRequestHeaders.Authorization =
                new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", config.HomeAssistantToken);

            string url = $"{config.HomeAssistantUrl}/api/states/{config.HomeAssistantPowerSensor}";
            var response = await client.GetAsync(url);

            if (!response.IsSuccessStatusCode)
                return null;

            var json = await response.Content.ReadAsStringAsync();
            using var doc = JsonDocument.Parse(json);

            string state = doc.RootElement.GetProperty("state").GetString() ?? "0";

            if (double.TryParse(state, NumberStyles.Any, CultureInfo.InvariantCulture, out double value))
                return value;

            return null;
        }
        catch
        {
            return null;
        }
    }


    // Mooie afronding
    static string F(double v) => Math.Round(v, 2).ToString(CultureInfo.InvariantCulture);
    static string FU(uint v) => v.ToString(CultureInfo.InvariantCulture);

    // 32-bit register
    static uint ReadUInt32(IModbusMaster master, byte slave, ushort lowAddr)
    {
        ushort low  = master.ReadInputRegisters(slave, lowAddr, 1)[0];
        ushort high = master.ReadInputRegisters(slave, (ushort)(lowAddr + 1), 1)[0];
        return ((uint)high << 16) | low;
    }

    // 16-bit + scaling
    static double ReadScaled(IModbusMaster master, byte slave, ushort addr, double scale)
    {
        ushort raw = master.ReadInputRegisters(slave, addr, 1)[0];
        return raw * scale;
    }

    // 16-bit signed + scaling
    // Variant voor registers die negatieve waarden kunnen bevatten (zoals buitentemperatuur).
    static double ReadSigned(IModbusMaster master, byte slave, ushort addr, double scale) 
    { 
        short raw = (short)master.ReadInputRegisters(slave, addr, 1)[0]; 
        return raw * scale; 
    }

    // State machine
    static string DecodeState(ushort status)
    {
        List<string> flags = new();

        // --- POMPEN ---
        if ((status & (1 << 0)) != 0) flags.Add("HK1_PUMP");     // B0
        // if ((status & (1 << 1)) != 0) flags.Add("HK2_PUMP");  // B1 (genegeerd volgens contract)

        // --- PROGRAMMA'S / MODES ---
        // if ((status & (1 << 2)) != 0) flags.Add("HEATUP");    // B2: HEAT-UP PROGRAM (niet gebruikt)
        if ((status & (1 << 3)) != 0) flags.Add("NHZ");          // B3: NHZ STAGES RUNNING
        if ((status & (1 << 4)) != 0) flags.Add("HEATING");      // B4: HP IN HEATING MODE
        if ((status & (1 << 5)) != 0) flags.Add("WATER");        // B5: HP IN DHW MODE
        if ((status & (1 << 6)) != 0) flags.Add("COMPRESSOR");   // B6: COMPRESSOR RUNNING
        if ((status & (1 << 7)) != 0) flags.Add("SUMMER");       // B7: SUMMER MODE ACTIVE
        if ((status & (1 << 8)) != 0) flags.Add("COOLING");      // B8: COOLING MODE ACTIVE
        if ((status & (1 << 9)) != 0) flags.Add("DEFROST");      // B9: DEFROST MODE

        // --- SILENT MODES ---
        // if ((status & (1 << 10)) != 0) flags.Add("SILENT1");  // B10: SILENT MODE 1 ACTIVE
        // if ((status & (1 << 11)) != 0) flags.Add("SILENT2");  // B11: SILENT MODE 2 ACTIVE (HP OFF)

        if (flags.Count == 0)
            return "IDLE";

        return string.Join(" ", flags);
    }

    // ASCII grafiek
    static string Bar(double value, double max = 70)
    {
        int len = (int)(value / max * 40);
        if (len < 0) len = 0;
        if (len > 40) len = 40;
        return new string('█', len);
    }

    // Delta‑logging opslag
    static double? lastFlow = null;
    static double? lastReturn = null;
    static double? lastHotgas = null;


    static void CleanupOldFiles(string dir, int days)
    {
        string pattern = "Wpm3Logger_*.csv";
        DateTime cutoff = DateTime.UtcNow.AddDays(-days);

        foreach (var file in Directory.GetFiles(dir, pattern))
        {
            // Bestandsnaam: Wpm3Logger_2026-01-09_14_00.csv
            string name = Path.GetFileNameWithoutExtension(file);

            // Extract date + hour
            // name = "Wpm3Logger_2026-01-09_14_00"
            string[] parts = name.Split('_');
            if (parts.Length < 3)
                continue;

            string datePart = parts[1]; // 2026-01-09
            string hourPart = parts[2]; // 14

            if (!DateTime.TryParseExact(
                    $"{datePart} {hourPart}:00",
                    "yyyy-MM-dd HH:mm",
                    CultureInfo.InvariantCulture,
                    DateTimeStyles.AssumeUniversal,
                    out DateTime fileDate))
            {
                continue;
            }

            if (fileDate < cutoff)
            {
                File.Delete(file);
            }
        }
    }

    static void LogError(string path, string msg)
    {
        File.AppendAllText(path, $"{DateTime.UtcNow:O} - {msg}{Environment.NewLine}");
    }
}

// ---------------------------------------------------------
// Config
// ---------------------------------------------------------

public class LoggerConfig
{
    public bool DebugMode { get; set; } = false;
    public string Ip { get; set; } = "";
    public int Port { get; set; }
    public byte SlaveId { get; set; }
    public int PollSeconds { get; set; }
    public int DaysToKeep { get; set; }
    public string LogDirectory { get; set; } = "";

    // Home Assistant integratie
    public string HomeAssistantUrl { get; set; } = "";
    public string HomeAssistantToken { get; set; } = "";
    public string HomeAssistantPowerSensor { get; set; } = "";
}

