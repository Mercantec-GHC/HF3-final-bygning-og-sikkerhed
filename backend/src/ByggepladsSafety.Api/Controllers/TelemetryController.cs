using Microsoft.AspNetCore.Mvc;
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

        public TelemetryController(IHubContext<SafetyHub> hubContext)
        {
            this.hubContext = hubContext;
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
                await hubContext.Clients.Group(SafetyHub.SafetyGroup).SendAsync(
                    "TriggerAlarm",
                    new AlarmNotification(
                        Guid.NewGuid(),
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
