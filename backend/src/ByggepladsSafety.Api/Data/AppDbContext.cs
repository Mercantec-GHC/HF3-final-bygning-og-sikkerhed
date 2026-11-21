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

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            base.OnModelCreating(modelBuilder);
            modelBuilder.Entity<Worker>()
                .HasMany(worker => worker.Shifts)
                .WithOne(shift => shift.Worker!)
                .HasForeignKey(shift => shift.WorkerId)
                .OnDelete(DeleteBehavior.Restrict);

            modelBuilder.Entity<Worker>()
                .HasMany(worker => worker.Incidents)
                .WithOne(incident => incident.Worker)
                .HasForeignKey(incident => incident.WorkerId)
                .OnDelete(DeleteBehavior.Restrict);

            modelBuilder.Entity<Shift>()
                .HasMany(shift => shift.Incidents)
                .WithOne(incident => incident.Shift)
                .HasForeignKey(incident => incident.ShiftId)
                .OnDelete(DeleteBehavior.Restrict);
        }
    }
}
