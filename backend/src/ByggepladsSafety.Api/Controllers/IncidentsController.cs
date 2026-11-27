using ByggepladsSafety.Api.Data;
using ByggepladsSafety.Api.Hubs;
using ByggepladsSafety.Api.Models;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.SignalR;
using Microsoft.EntityFrameworkCore;

namespace ByggepladsSafety.Api.Controllers;

[ApiController]
[Route("api/incidents")]
public sealed class IncidentsController : ControllerBase
{
    private static readonly string[] AllowedAckSources = ["PHONE_PWA", "DASHBOARD", "LOCAL_ACK"];
    private readonly AppDbContext db;
    private readonly IHubContext<SafetyHub> hubContext;

    public IncidentsController(AppDbContext db, IHubContext<SafetyHub> hubContext)
    {
        this.db = db;
        this.hubContext = hubContext;
    }

    [HttpGet]
    public async Task<ActionResult<IReadOnlyList<IncidentListItem>>> GetIncidents(
        [FromQuery] bool activeOnly = false,
        CancellationToken cancellationToken = default)
    {
        var query = db.Incidents.AsNoTracking();
        if (activeOnly)
        {
            query = query.Where(incident => incident.Status != "ACKNOWLEDGED");
        }

        var incidents = await query
            .OrderByDescending(incident => incident.CreatedAt)
            .Take(100)
            .Select(incident => new IncidentListItem(
                incident.Id,
                incident.EventType,
                incident.DeviceId,
                incident.BleUuid,
                incident.Status,
                incident.CreatedAt,
                incident.AcknowledgedAt,
                incident.ReactionTimeSeconds,
                incident.AckSource))
            .ToListAsync(cancellationToken);

        return Ok(incidents);
    }

    [HttpPost("{id:guid}/ack")]
    public async Task<ActionResult<IncidentAckResult>> Acknowledge(
        Guid id,
        [FromBody] AcknowledgeIncidentRequest request,
        CancellationToken cancellationToken = default)
    {
        var ackSource = request.AckSource?.Trim().ToUpperInvariant();
        if (ackSource is null || !AllowedAckSources.Contains(ackSource, StringComparer.Ordinal))
        {
            return BadRequest(new { message = "ackSource must be PHONE_PWA, DASHBOARD, or LOCAL_ACK." });
        }

        var incident = await db.Incidents.SingleOrDefaultAsync(
            item => item.Id == id,
            cancellationToken);
        if (incident is null)
        {
            return NotFound();
        }

        if (!string.Equals(incident.Status, "ACKNOWLEDGED", StringComparison.OrdinalIgnoreCase))
        {
            var acknowledgedAt = DateTimeOffset.UtcNow;
            incident.Status = "ACKNOWLEDGED";
            incident.AcknowledgedAt = acknowledgedAt;
            incident.ReactionTimeSeconds = Math.Max(
                0,
                (int)(acknowledgedAt - incident.CreatedAt).TotalSeconds);
            incident.AckSource = ackSource;

            var deviceState = await db.DeviceStates.SingleOrDefaultAsync(
                state => state.DeviceId == incident.DeviceId,
                cancellationToken);
            if (deviceState is not null)
            {
                deviceState.AlarmActive = false;
                deviceState.PendingCommand = "CLEAR_ALARM";
            }

            await db.SaveChangesAsync(cancellationToken);

            await hubContext.Clients.Group(SafetyHub.SafetyGroup).SendAsync(
                "ClearAlarm",
                new AlarmClearedNotification(incident.Id, ackSource, acknowledgedAt),
                cancellationToken);
        }

        return Ok(new IncidentAckResult(
            incident.Id,
            incident.Status,
            incident.AcknowledgedAt,
            incident.ReactionTimeSeconds,
            incident.AckSource));
    }
}

public sealed record AcknowledgeIncidentRequest(string? AckSource);

public sealed record IncidentAckResult(
    Guid Id,
    string Status,
    DateTimeOffset? AcknowledgedAt,
    int? ReactionTimeSeconds,
    string? AckSource);

public sealed record IncidentListItem(
    Guid Id,
    string EventType,
    string DeviceId,
    string? BleUuid,
    string Status,
    DateTimeOffset CreatedAt,
    DateTimeOffset? AcknowledgedAt,
    int? ReactionTimeSeconds,
    string? AckSource);