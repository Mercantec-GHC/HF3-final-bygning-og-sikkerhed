using Microsoft.EntityFrameworkCore;
using ByggepladsSafety.Api.Models;

namespace ByggepladsSafety.Api.Data
{
    public class AppDbContext : DbContext
    {
        public AppDbContext(DbContextOptions<AppDbContext> options)
        : base(options) { }

        public DbSet<Worker> Workers => Set<Worker>();
        public DbSet<Shift> Shifts => Set<Shift>();
        public DbSet<Incident> Incidents => Set<Incident>();
        public DbSet<DeviceState> DeviceStates => Set<DeviceState>();

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            base.OnModelCreating(modelBuilder);

            modelBuilder.Entity<Worker>(entity =>
            {
                entity.ToTable("Workers");
                entity.HasKey(worker => worker.Id);

                entity.Property(worker => worker.FullName)
                    .HasMaxLength(200)
                    .IsRequired();
                entity.Property(worker => worker.BleBeaconUuid)
                    .HasMaxLength(100)
                    .IsRequired();
                entity.Property(worker => worker.PushSubscriptionJson)
                    .HasColumnType("text");

                entity.HasIndex(worker => worker.BleBeaconUuid)
                    .IsUnique();
            });

            modelBuilder.Entity<Shift>(entity =>
            {
                entity.ToTable("Shifts");
                entity.HasKey(shift => shift.Id);

                entity.Property(shift => shift.CheckInTime)
                    .HasColumnType("timestamp with time zone");
                entity.Property(shift => shift.CheckOutTime)
                    .HasColumnType("timestamp with time zone");
                entity.Property(shift => shift.LastHeartbeat)
                    .HasColumnType("timestamp with time zone");

                entity.HasOne(shift => shift.Worker)
                    .WithMany(worker => worker.Shifts)
                    .HasForeignKey(shift => shift.WorkerId)
                    .OnDelete(DeleteBehavior.Cascade);
            });

            modelBuilder.Entity<Incident>(entity =>
            {
                entity.ToTable("Incidents");
                entity.HasKey(incident => incident.Id);

                entity.Property(incident => incident.DeviceId)
                    .HasMaxLength(100)
                    .IsRequired();
                entity.Property(incident => incident.BleUuid)
                    .HasMaxLength(100);
                entity.Property(incident => incident.EventType)
                    .HasMaxLength(50)
                    .IsRequired();
                entity.Property(incident => incident.Status)
                    .HasMaxLength(30)
                    .IsRequired();
                entity.Property(incident => incident.AckSource)
                    .HasMaxLength(30);
                entity.Property(incident => incident.CreatedAt)
                    .HasColumnType("timestamp with time zone")
                    .HasDefaultValueSql("CURRENT_TIMESTAMP");
                entity.Property(incident => incident.AcknowledgedAt)
                    .HasColumnType("timestamp with time zone");

                entity.HasOne(incident => incident.Worker)
                    .WithMany(worker => worker.Incidents)
                    .HasForeignKey(incident => incident.WorkerId)
                    .OnDelete(DeleteBehavior.SetNull);
            });

            modelBuilder.Entity<DeviceState>(entity =>
            {
                entity.ToTable("DeviceStates");
                entity.HasKey(device => device.DeviceId);

                entity.Property(device => device.DeviceId)
                    .HasMaxLength(100);
                entity.Property(device => device.ZoneState)
                    .HasMaxLength(20)
                    .IsRequired();
                entity.Property(device => device.PendingCommand)
                    .HasMaxLength(30)
                    .IsRequired();
                entity.Property(device => device.LastSeenAt)
                    .HasColumnType("timestamp with time zone")
                    .HasDefaultValueSql("CURRENT_TIMESTAMP");
            });
        }
    }
}
