using Microsoft.AspNetCore.Mvc;
using ByggepladsSafety.Api.Models;

namespace ByggepladsSafety.Api.Controllers
{
    [ApiController]
    [Route("api/controller")]
    public class TelemetryController : ControllerBase
    {
        [HttpPost]
        public async Task<IActionResult> HandleEvent(ArduinoResponse response) 
        {
            return Ok(response.EventType);
        }
    }
}
