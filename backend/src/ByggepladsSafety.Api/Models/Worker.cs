namespace ByggepladsSafety.Api.Models
{
    public class Worker
    {
        public int Id { get; set; }
        public string? FirstName { get; set; }
        public string? LastName { get; set; }
        public int? Age { get; set; }
        public ICollection<Shift> Shifts { get; } = new List<Shift>();
        public ICollection<Incidents> Incidents { get; } = new List<Incidents>();
    }
}
