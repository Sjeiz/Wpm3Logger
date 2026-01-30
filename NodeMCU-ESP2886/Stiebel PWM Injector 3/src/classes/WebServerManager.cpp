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
          var refreshInterval = %REFRESH_INTERVAL% * 60 * 1000;
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
        html.replace("%REFRESH_INTERVAL%", String(WEBLOGGER_REFRESH_INTERVAL_MIN));
        server.send(200, "text/html", html);
    });
    server.on("/log", [this]() {
      String htmlLog = webLogger ? webLogger->getLogHtml() : "";
      server.send(200, "text/html", htmlLog);
    });
    server.begin();
}

void WebServerManager::handleClient() {
    server.handleClient();
}
