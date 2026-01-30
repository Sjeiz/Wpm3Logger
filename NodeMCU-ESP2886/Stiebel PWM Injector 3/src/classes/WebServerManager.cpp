/*
 * WebServerManager - Log Webinterface (ESP8266)
 *
 * - Gebruikt een statische ringbuffer van LOG_LINES regels, elk maximaal LOG_LINE_LEN tekens.
 * - Elke logregel wordt bij toevoegen afgekapt op LOG_LINE_LEN-1 tekens (inclusief '\0').
 * - De webserver stuurt bij elke refresh de inhoud van de ringbuffer direct naar de client,
 *   regel voor regel, via streaming (server.sendContent of vergelijkbaar).
 * - Hierdoor staat de logging niet dubbel in het geheugen: de logregels staan alleen in de ringbuffer,
 *   en worden bij het opvragen direct naar de webclient gestuurd zonder een extra grote tijdelijke string te maken.
 * - Dit minimaliseert RAM-gebruik en voorkomt dubbele opslag van logregels.
 */
#include "WebServerManager.h"

WebServerManager::WebServerManager(WebLogger* logger)
    : server(80), webLogger(logger) {}

void WebServerManager::setup() {
    server.on("/", [this]() {
        String html = F(R"rawliteral(
      <!DOCTYPE html>
      <html lang='en'>
      <head>
        <meta charset='UTF-8'>
        <meta name='viewport' content='width=device-width,initial-scale=1'>
        <title>Stiebel PWM Injector Log</title>
        <style>
          body { font-family: monospace; background: #222; color: #eee; margin: 0; padding: 0; }
          #log { background: #111; padding: 1em; margin: 0; font-size: 1em; }
          .header {
            background: #333;
            margin: 0;
          }
          .statuslog {
            background: #222;
            border: 1px solid #444;
            border-radius: 6px;
            margin: 0.5em 0;
            padding: 0.5em 1em;
            color: #fff;
            box-shadow: 0 2px 6px #0004;
            font-family: inherit;
            font-size: 1em;
            display: block;
          }
        </style>
        <script>
          var refreshInterval = %REFRESH_INTERVAL% * 1000;
        </script>
      </head>
      <body>
        <div class='header'><h2>Stiebel PWM Injector Log</h2></div>
        <div id='log'></div>
        <script>
          function fetchLog() {
            fetch('/log').then(r => r.text()).then(t => {
              document.getElementById('log').innerHTML = t;
            });
          }
          setInterval(fetchLog, refreshInterval);
          fetchLog();
        </script>
      </body>
      </html>
    )rawliteral");
        html.replace("%REFRESH_INTERVAL%", String(WEBLOGGER_REFRESH_PAGE_SEC));
        server.send(200, "text/html", html);
    });
    server.on("/log", [this]() {
      // Streaming logbuffer direct naar client, geen dubbele opslag in RAM
      if (webLogger) {
        server.setContentLength(CONTENT_LENGTH_UNKNOWN); // Chunked transfer
        server.send(200, "text/html", ""); // Start response
        webLogger->streamLogHtml(server); // Streamt logregels direct
      } else {
        server.send(200, "text/html", "No log available");
      }
    });
    server.begin();
}

void WebServerManager::handleClient() {
    server.handleClient();
}
