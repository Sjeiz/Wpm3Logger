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

            //
            // TEMPERATUREN
            //
            double roomTemp       = ReadScaled(master, slave, 502, 0.1);
            double roomSet        = ReadScaled(master, slave, 503, 0.1);
            double outsideTemp    = ReadSigned(master, slave, 506, 0.1);

            double hk1Flow        = ReadScaled(master, slave, 507, 0.1);
            double hk1Set         = ReadScaled(master, slave, 509, 0.1);

            double hk2Flow        = ReadScaled(master, slave, 510, 0.1);
            double hk2Set         = ReadScaled(master, slave, 511, 0.1);

            // HK2 RETOUR – deze wordt WÉL gelogd
            double hk2Return      = ReadScaled(master, slave, 515, 0.1);

            double flowTemp       = ReadScaled(master, slave, 514, 0.1);

            double hotwaterActual = ReadScaled(master, slave, 521, 0.1);
            double hotwaterSet    = ReadScaled(master, slave, 522, 0.1);

            //
            // COMPRESSOR
            //
            double compressorReturn = ReadScaled(master, slave, 541, 0.1);
            double compressorFlow   = ReadScaled(master, slave, 542, 0.1);
            double hotgas           = ReadScaled(master, slave, 543, 0.1);

            double compLow          = ReadScaled(master, slave, 544, 0.01);
            double compMid          = ReadScaled(master, slave, 545, 0.01);
            double compHigh         = ReadScaled(master, slave, 546, 0.01);

            //
            // HYDRAULIEK
            //
            double waterPressure    = ReadScaled(master, slave, 519, 0.01);
            double flowRate         = ReadScaled(master, slave, 520, 0.01);

            //
            // ENERGIE (32-bit)
            //
            uint energyHeat         = ReadUInt32(master, slave, 3501);
            uint energyWater        = ReadUInt32(master, slave, 3504);
            uint energyNhz          = ReadUInt32(master, slave, 3506);

            //
            // RUNTIME (16-bit)
            //
            uint runtimeHeat        = master.ReadInputRegisters(slave, 3538, 1)[0];
            uint runtimeWater       = master.ReadInputRegisters(slave, 3541, 1)[0];
            uint runtimeCool        = master.ReadInputRegisters(slave, 3544, 1)[0];
            uint runtimeNhz         = master.ReadInputRegisters(slave, 3545, 1)[0];

            //
            // STATUS
            //
            ushort sgReady          = master.ReadHoldingRegisters(slave, 4000, 1)[0];
            ushort statusBits       = master.ReadInputRegisters(slave, 2500, 1)[0];
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
        if ((status & 0b00000001) != 0) return "HEATING";
        if ((status & 0b00000010) != 0) return "WATER";
        if ((status & 0b00000100) != 0) return "COOLING";
        if ((status & 0b00001000) != 0) return "DEFROST";
        if ((status & 0b00010000) != 0) return "PUMP_ONLY";
        return "IDLE";
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

