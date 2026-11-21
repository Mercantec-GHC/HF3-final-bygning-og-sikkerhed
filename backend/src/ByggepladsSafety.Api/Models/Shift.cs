namespace ByggepladsSafety.Api.Models
{
    public class Shift
    {
        public int Id { get; set; }
        public int WorkerId { get; set; }
        public Worker? Worker { get; set; }

        public DateTimeOffset CheckInTime { get; set; }
        public DateTimeOffset? CheckOutTime { get; set; }
        public DateTimeOffset? LastHeartbeat { get; set; }
        public bool IsOnline { get; set; }

        public ICollection<Incidents> Incidents { get; } = new List<Incidents>();
    }
}