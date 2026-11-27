namespace ByggepladsSafety.Api.Models
{
    public class Incident
    {
        public Guid Id { get; set; }
        public Guid? WorkerId { get; set; }
        public Worker? Worker { get; set; }

        public string DeviceId { get; set; } = string.Empty;
        public string? BleUuid { get; set; }
        public string EventType { get; set; } = string.Empty;
        public int? RssiValue { get; set; }
        public double? Temperature { get; set; }
        public double? Humidity { get; set; }
        public string Status { get; set; } = "DETECTED";
        public DateTimeOffset CreatedAt { get; set; }
        public DateTimeOffset? AcknowledgedAt { get; set; }
        public int? ReactionTimeSeconds { get; set; }
        public string? AckSource { get; set; }
    }
}
