#pragma once

#include <SPI.h>
#include <Wire.h> 
#include <TinyLoRa.h>

#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#ifdef INTERNALSENSOR
#include <SparkFunBME280.h>
#endif

#if DEBUG
#define LOG(text) Serial.println(text)
#define LOG2(text1, text2) Serial.print(text1); Serial.println(text2)
#else
#define LOG(text)
#define LOG2(text1, text2)
#endif

struct Telemetry {
  double altitude = 0.0;
  double latitude = 0.0;
  double longitude = 0.0;
  double course = 0.0;
  double speed = 0.0;
  uint32_t satellites = 0;
  float battery = 0.0;

  float temperature = 0.0;
  float pressure = 0.0;
  //float humidity = 0.0;
};


class Beacon {
  friend void updateData(UBX_NAV_PVT_data_t ubxDataStruct);
  private:
    double horizon = 900.0;

    SFE_UBLOX_GNSS gnss;
    TinyLoRa lora;
    

    unsigned long lastPacket = 0;
    long randomDelay = 0;

    bool positionValid = false;
    Telemetry position;
    
    // Burst info
    bool didBurst = false;
    Telemetry burstPosition;    

    #ifdef INTERNALSENSOR
    BME280 internalSensor;
    #endif
    
    void SendPacket(bool burst = false);
    bool CanSend();

    bool BelowHorizon() {
      return (!positionValid) || position.altitude < horizon;
    }

    void updatePosition(UBX_NAV_PVT_data_t ubxDataStruct);
  public:
    Beacon();
    void Setup();
    void Update();
};

extern Beacon beacon;