namespace ByggepladsSafety.Api.Models
{
    public class Worker
    {
        public int Id { get; set; }
        public string? FirstName { get; set; }
        public string? LastName { get; set; }
        public int? Age { get; set; }
        public List<Shift?>? Shifts { get; set; }
        public List<Incidents?>? Incidents { get; set; }
    }
}
