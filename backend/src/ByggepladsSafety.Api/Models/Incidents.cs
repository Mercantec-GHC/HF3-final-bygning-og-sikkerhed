namespace ByggepladsSafety.Api.Models
{
    public class Incidents
    {
        public int Id { get; set; }
        public int ShiftId { get; set; }
        public Shift? Shift { get; set; }
        public int WorkerId { get; set; }
        public Worker? Worker { get; set; }
    }
}
