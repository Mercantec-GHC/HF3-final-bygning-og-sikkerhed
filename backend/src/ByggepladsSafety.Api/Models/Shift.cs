namespace ByggepladsSafety.Api.Models
{
    public class Shift
    {
        public Guid Id { get; set; }
        public Guid WorkerId { get; set; }
        public Worker Worker { get; set; } = null!;

        public DateTimeOffset CheckInTime { get; set; }
        public DateTimeOffset? CheckOutTime { get; set; }
        public DateTimeOffset? LastHeartbeat { get; set; }
        public bool IsOnline { get; set; }
    }
}
