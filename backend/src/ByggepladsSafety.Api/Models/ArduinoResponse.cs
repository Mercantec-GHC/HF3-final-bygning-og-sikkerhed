namespace ByggepladsSafety.Api.Models
{
    public class ArduinoResponse
    {
        public string EventType { get; set; } = string.Empty;
        public string BleUuid { get; set; } = string.Empty;
        public int Rssi { get; set; }
        public double Temperature { get; set; }
        public double Humidity { get; set; }
        public string ZoneState { get; set; } = string.Empty;
    }
}
