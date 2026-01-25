#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Temperature sensor states
enum TempSensorState {
  TEMP_OK,            // Sensor working normally
  TEMP_FAILED,        // Sensor read failed, retrying
  TEMP_DISCONNECTED   // Sensor not found
};

class TemperatureSensor {
private:
  OneWire* oneWire;
  DallasTemperature* sensors;
  uint8_t pin;
  
  TempSensorState state;
  float lastValidTemperature;
  unsigned long lastReadTime;
  unsigned long lastRetryTime;
  bool requestPending;
  
  static const unsigned long READ_INTERVAL = 2000;      // Read every 2 seconds
  static const unsigned long RETRY_INTERVAL = 30000;    // Retry failed sensor every 30 seconds
  static const unsigned long CONVERSION_TIME = 750;     // DS18B20 conversion time (ms)
  
public:
  TemperatureSensor(uint8_t sensorPin);
  ~TemperatureSensor();
  
  void begin();
  void update();  // Call every loop iteration
  
  bool isAvailable() const;
  float getTemperature() const;
  TempSensorState getState() const;
  String getStateString() const;
};

#endif
