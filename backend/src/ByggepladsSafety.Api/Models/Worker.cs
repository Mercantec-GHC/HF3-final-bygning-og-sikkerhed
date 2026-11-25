namespace ByggepladsSafety.Api.Models
{
    public class Worker
    {
        public Guid Id { get; set; }
        public string FullName { get; set; } = string.Empty;
        public string BleBeaconUuid { get; set; } = string.Empty;
        public string? PushSubscriptionJson { get; set; }

        public ICollection<Shift> Shifts { get; set; } = new List<Shift>();
        public ICollection<Incident> Incidents { get; set; } = new List<Incident>();
    }
}
