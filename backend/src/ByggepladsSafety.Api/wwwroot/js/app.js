(() => {
    const screen = document.getElementById("status-screen");
    const icon = document.getElementById("status-icon");
    const title = document.getElementById("status-title");
    const message = document.getElementById("status-message");
    const details = document.getElementById("alarm-details");
    const acknowledgeButton = document.getElementById("acknowledge-button");
    const connectionStatus = document.getElementById("connection-status");

    let activeIncidentId = null;

    function showSafeState(source = "") {
        screen.className = "safe";
        screen.setAttribute("aria-label", "Alt er sikkert");
        icon.textContent = "✓";
        title.textContent = "Alt er sikkert";
        message.textContent = source === "" ? "Ingen aktive alarmer" : `Alarm kvitteret (${source})`;
        details.textContent = "";
        acknowledgeButton.hidden = true;
        activeIncidentId = null;
    }

    function showAlarm(notification) {
        screen.className = "alarm";
        screen.setAttribute("aria-label", "Aktiv alarm");
        icon.textContent = "!";
        title.textContent = "ALARM";
        message.textContent = notification.eventType || "Fare registreret";

        const values = [notification.deviceId, notification.bleUuid].filter(Boolean);
        details.textContent = values.length > 0 ? `Enhed: ${values.join(" · ")}` : "Forlad farezonen og kvitter, når du er i sikkerhed.";
        acknowledgeButton.hidden = false;
        activeIncidentId = notification.incidentId || null;
        acknowledgeButton.focus({ preventScroll: true });
    }

    async function acknowledgeAlarm() {
        acknowledgeButton.disabled = true;
        try {
            if (activeIncidentId) {
                const response = await fetch(`/api/incidents/${encodeURIComponent(activeIncidentId)}/ack`, {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({ ackSource: "PHONE_PWA" })
                });

                if (!response.ok && response.status !== 404) {
                    throw new Error(`ACK failed: ${response.status}`);
                }
            }

            showSafeState("PWA");
        } catch (error) {
            message.textContent = "Kvittering kunne ikke sendes. Prøv igen.";
            console.error(error);
        } finally {
            acknowledgeButton.disabled = false;
        }
    }

    function setConnectionStatus(text) {
        connectionStatus.textContent = text;
    }

    acknowledgeButton.addEventListener("click", acknowledgeAlarm);

    if (!window.signalR) {
        setConnectionStatus("SignalR-biblioteket kunne ikke indlæses");
        return;
    }

    const connection = new signalR.HubConnectionBuilder()
        .withUrl("/hubs/safety")
        .withAutomaticReconnect([0, 2000, 5000, 10000])
        .configureLogging(signalR.LogLevel.Warning)
        .build();

    connection.on("TriggerAlarm", showAlarm);
    connection.on("ClearAlarm", notification => showSafeState(notification?.source || "hardware"));
    connection.onreconnecting(() => setConnectionStatus("Forbindelse mistet – prøver igen…"));
    connection.onreconnected(async () => {
        setConnectionStatus("Forbundet");
        await connection.invoke("JoinSafetyChannel");
    });
    connection.onclose(() => setConnectionStatus("Ikke forbundet"));

    async function startConnection() {
        try {
            await connection.start();
            await connection.invoke("JoinSafetyChannel");
            setConnectionStatus("Forbundet");
        } catch (error) {
            setConnectionStatus("Forbindelse mislykkedes – prøver igen…");
            console.error(error);
            window.setTimeout(startConnection, 5000);
        }
    }

    startConnection();
})();
