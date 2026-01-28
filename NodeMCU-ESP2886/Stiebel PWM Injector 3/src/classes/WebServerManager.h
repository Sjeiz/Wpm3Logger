#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <ESP8266WebServer.h>
#include <Arduino.h>
#include "config.h"
#include "../loggers/WebLogger.h"

class WebServerManager {
public:
    WebServerManager(WebLogger* logger);
    void setup();
    void handleClient();
private:
    ESP8266WebServer server;
    WebLogger* webLogger;
};

#endif // WEBSERVERMANAGER_H
