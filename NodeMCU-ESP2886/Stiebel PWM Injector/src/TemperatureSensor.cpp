#include "TemperatureSensor.h"
#include "Config.h"

TemperatureSensor::TemperatureSensor(uint8_t sensorPin) 
  : pin(sensorPin),
    state(TEMP_DISCONNECTED),
    lastValidTemperature(0.0),
    lastReadTime(0),
    lastRetryTime(0),
    requestPending(false) {
  
  oneWire = new OneWire(pin);
  sensors = new DallasTemperature(oneWire);
}

TemperatureSensor::~TemperatureSensor() {
  delete sensors;
  delete oneWire;
}

void TemperatureSensor::begin() {
  // Enable internal pull-up resistor on data pin
  pinMode(pin, INPUT_PULLUP);
  
  sensors->begin();
  
  // Check if sensor is connected
  int deviceCount = sensors->getDeviceCount();
  if (DEBUG_MODE) {
    Serial.print("DEBUG: DS18B20 initialization - Found ");
    Serial.print(deviceCount);
    Serial.println(" device(s)");
  }
  
  if (deviceCount > 0) {
    // Print device address for debugging
    if (DEBUG_MODE) {
      DeviceAddress addr;
      if (sensors->getAddress(addr, 0)) {
        Serial.print("DEBUG: DS18B20 address: ");
        for (uint8_t i = 0; i < 8; i++) {
          if (addr[i] < 16) Serial.print("0");
          Serial.print(addr[i], HEX);
          if (i < 7) Serial.print(":");
        }
        Serial.println();
      }
    }
    
    state = TEMP_OK;
    sensors->setResolution(11);  // 11-bit resolution = 0.125°C
    sensors->setWaitForConversion(false);  // Non-blocking mode
    sensors->requestTemperatures();
    requestPending = true;
    lastReadTime = millis();
    if (DEBUG_MODE) {
      Serial.println("DEBUG: DS18B20 state = OK (11-bit resolution)");
    }
  } else {
    state = TEMP_DISCONNECTED;
    lastRetryTime = millis();
    if (DEBUG_MODE) {
      Serial.println("DEBUG: DS18B20 state = DISCONNECTED (check wiring!)");
    }
  }
}

void TemperatureSensor::update() {
  unsigned long currentTime = millis();
  
  switch (state) {
    case TEMP_OK:
      if (requestPending) {
        // Check if conversion is complete
        if (currentTime - lastReadTime >= CONVERSION_TIME) {
          float temp = sensors->getTempCByIndex(0);
          
          if (temp != DEVICE_DISCONNECTED_C && temp > -50.0 && temp < 125.0) {
            // Valid reading
            lastValidTemperature = temp;
            requestPending = false;
          } else {
            // Read failed
            state = TEMP_FAILED;
            lastRetryTime = currentTime;
            requestPending = false;
          }
        }
      } else {
        // Time to request new reading
        if (currentTime - lastReadTime >= READ_INTERVAL) {
          sensors->requestTemperatures();
          requestPending = true;
          lastReadTime = currentTime;
        }
      }
      break;
      
    case TEMP_FAILED:
      // Retry every 30 seconds
      if (currentTime - lastRetryTime >= RETRY_INTERVAL) {
        if (DEBUG_MODE) {
          Serial.println("DEBUG: DS18B20 retrying after FAILED state...");
        }
        sensors->begin();  // Re-initialize
        int deviceCount = sensors->getDeviceCount();
        if (DEBUG_MODE) {
          Serial.print("DEBUG: DS18B20 found ");
          Serial.print(deviceCount);
          Serial.println(" device(s)");
        }
        
        if (deviceCount > 0) {
          state = TEMP_OK;
          sensors->setResolution(11);  // 11-bit resolution = 0.125°C
          sensors->setWaitForConversion(false);
          sensors->requestTemperatures();
          requestPending = true;
          lastReadTime = currentTime;
          if (DEBUG_MODE) {
            Serial.println("DEBUG: DS18B20 recovered - state = OK (11-bit resolution)");
          }
        } else {
          state = TEMP_DISCONNECTED;
          lastRetryTime = currentTime;
          if (DEBUG_MODE) {
            Serial.println("DEBUG: DS18B20 still disconnected");
          }
        }
      }
      break;
      
    case TEMP_DISCONNECTED:
      // Retry every 30 seconds
      if (currentTime - lastRetryTime >= RETRY_INTERVAL) {
        if (DEBUG_MODE) {
          Serial.println("DEBUG: DS18B20 retrying after DISCONNECTED state...");
        }
        sensors->begin();
        int deviceCount = sensors->getDeviceCount();
        if (DEBUG_MODE) {
          Serial.print("DEBUG: DS18B20 found ");
          Serial.print(deviceCount);
          Serial.println(" device(s)");
        }
        
        if (deviceCount > 0) {
          state = TEMP_OK;
          sensors->setResolution(11);  // 11-bit resolution = 0.125°C
          sensors->setWaitForConversion(false);
          sensors->requestTemperatures();
          requestPending = true;
          lastReadTime = currentTime;
          if (DEBUG_MODE) {
            Serial.println("DEBUG: DS18B20 connected - state = OK (11-bit resolution)");
          }
        } else {
          lastRetryTime = currentTime;
          if (DEBUG_MODE) {
            Serial.println("DEBUG: DS18B20 still disconnected - check wiring!");
          }
        }
      }
      break;
  }
}

bool TemperatureSensor::isAvailable() const {
  return state == TEMP_OK;
}

float TemperatureSensor::getTemperature() const {
  return lastValidTemperature;
}

TempSensorState TemperatureSensor::getState() const {
  return state;
}

String TemperatureSensor::getStateString() const {
  switch (state) {
    case TEMP_OK:
      return "OK";
    case TEMP_FAILED:
      return "FAILED";
    case TEMP_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
  }
}
