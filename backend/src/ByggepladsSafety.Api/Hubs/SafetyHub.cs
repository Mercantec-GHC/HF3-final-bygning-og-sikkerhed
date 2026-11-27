using Microsoft.AspNetCore.SignalR;

namespace ByggepladsSafety.Api.Hubs;

public sealed class SafetyHub : Hub
{
    public const string SafetyGroup = "safety";

    public Task JoinSafetyChannel()
    {
        return Groups.AddToGroupAsync(Context.ConnectionId, SafetyGroup);
    }

    public Task LeaveSafetyChannel()
    {
        return Groups.RemoveFromGroupAsync(Context.ConnectionId, SafetyGroup);
    }
}

public sealed record AlarmNotification(
    Guid IncidentId,
    string EventType,
    string DeviceId,
    string? BleUuid,
    int? Rssi,
    double? Temperature,
    double? Humidity,
    DateTimeOffset CreatedAt);

public sealed record AlarmClearedNotification(Guid? IncidentId, string Source, DateTimeOffset ClearedAt);
