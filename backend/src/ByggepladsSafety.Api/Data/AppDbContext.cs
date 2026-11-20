using Microsoft.EntityFrameworkCore;
using ByggepladsSafety.Api.Models;

namespace ByggepladsSafety.Api.Data
{
    public class AppDbContext : DbContext
    {
        public AppDbContext(DbContextOptions<AppDbContext> options)
        : base(options) { }

        public DbSet<Worker> Worker { get; set; }
        public DbSet<Shift> Shift { get; set; }
        public DbSet<Incidents> Incidents { get; set; }
    }
}
