namespace ByggepladsSafety.Api.Models
{
    public class DeviceState
    {
        public string DeviceId { get; set; } = string.Empty;
        public string ZoneState { get; set; } = "ACTIVE";
        public bool AlarmActive { get; set; }
        public string PendingCommand { get; set; } = "NONE";
        public double? LastTemperature { get; set; }
        public double? LastHumidity { get; set; }
        public DateTimeOffset LastSeenAt { get; set; }
        public bool IsOnline { get; set; }
    }
}
