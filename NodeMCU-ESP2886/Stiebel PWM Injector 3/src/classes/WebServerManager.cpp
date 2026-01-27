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
          #log { white-space: pre-wrap; background: #111; padding: 1em; margin: 0; font-size: 1em; }
          .header {
            background: #333;
            margin: 0;
          }
        </style>
      </head>
      <body>
        <div class='header'><h2>Stiebel PWM Injector Log</h2></div>
        <div id='log'></div>
        <script>
          function fetchLog() {
            fetch('/log').then(r => r.text()).then(t => {
              document.getElementById('log').textContent = t;
            });
          }
          setInterval(fetchLog, 2000);
          fetchLog();
        </script>
      </body>
      </html>
    )rawliteral");
        server.send(200, "text/html", html);
    });
    server.on("/log", [this]() {
      String reversedLog = webLogger ? webLogger->getLogText() : "";
      server.send(200, "text/plain", reversedLog);
    });
    server.begin();
}

void WebServerManager::handleClient() {
    server.handleClient();
}
