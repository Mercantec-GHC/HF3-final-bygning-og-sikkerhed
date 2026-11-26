using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using ByggepladsSafety.Api.Data;
using ByggepladsSafety.Api.Models;
using ByggepladsSafety.Api.Hubs;
using Microsoft.AspNetCore.SignalR;

namespace ByggepladsSafety.Api.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class TelemetryController : ControllerBase
    {
        private readonly IHubContext<SafetyHub> hubContext;
        private readonly AppDbContext db;

        public TelemetryController(IHubContext<SafetyHub> hubContext, AppDbContext db)
        {
            this.hubContext = hubContext;
            this.db = db;
        }

        [HttpPost("event")]
        public async Task<IActionResult> HandleEvent(ArduinoResponse response) 
        {
            if (string.Equals(response.EventType, "LOCAL_ACK", StringComparison.OrdinalIgnoreCase))
            {
                await hubContext.Clients.Group(SafetyHub.SafetyGroup).SendAsync(
                    "ClearAlarm",
                    new AlarmClearedNotification(null, "ARDUINO", DateTimeOffset.UtcNow));
            }
            else if (IsAlarmEvent(response.EventType))
            {
                var now = DateTimeOffset.UtcNow;
                var incident = new Incident
                {
                    Id = Guid.NewGuid(),
                    DeviceId = response.DeviceId,
                    BleUuid = response.BleUuid,
                    EventType = response.EventType,
                    RssiValue = response.Rssi,
                    Temperature = response.Temperature,
                    Humidity = response.Humidity,
                    Status = "DETECTED",
                    CreatedAt = now
                };

                db.Incidents.Add(incident);
                var deviceState = await db.DeviceStates.SingleOrDefaultAsync(
                    state => state.DeviceId == response.DeviceId);
                if (deviceState is null)
                {
                    deviceState = new DeviceState { DeviceId = response.DeviceId };
                    db.DeviceStates.Add(deviceState);
                }

                deviceState.AlarmActive = true;
                deviceState.PendingCommand = "NONE";
                deviceState.ZoneState = string.IsNullOrWhiteSpace(response.ZoneState)
                    ? "ACTIVE"
                    : response.ZoneState;
                deviceState.LastTemperature = response.Temperature;
                deviceState.LastHumidity = response.Humidity;
                deviceState.LastSeenAt = now;
                deviceState.IsOnline = true;
                await db.SaveChangesAsync();

                await hubContext.Clients.Group(SafetyHub.SafetyGroup).SendAsync(
                    "TriggerAlarm",
                    new AlarmNotification(
                        incident.Id,
                        response.EventType,
                        response.DeviceId,
                        response.BleUuid,
                        response.Rssi,
                        response.Temperature,
                        response.Humidity,
                        DateTimeOffset.UtcNow));
            }

            return Ok(response.EventType);
        }

        private static bool IsAlarmEvent(string eventType)
        {
            return eventType.Equals("CRANE_RADAR", StringComparison.OrdinalIgnoreCase)
                || eventType.Equals("SECURITY_TAMPER", StringComparison.OrdinalIgnoreCase)
                || eventType.Equals("HIGH_TEMP", StringComparison.OrdinalIgnoreCase);
        }
    }
}
