#pragma once

#include <TinyGPS++.h>
#include <TinyLoRa.h>
#include <SPI.h>

struct PositionInfo {
  double altitude = 0.0;
  double latitude = 0.0;
  double longitude = 0.0;
  double course = 0.0;
  double speed = 0.0;
  uint32_t satellites = 0;
  float battery = 0.0;
};

class Beacon {
  private:
    double horizon = 900.0;

    TinyGPSPlus gps;
    TinyLoRa lora;
    unsigned long lastPacket = 0;
    long randomDelay = 0;

    bool positionValid = false;
    PositionInfo position;
    
    // Burst info
    bool didBurst = false;
    PositionInfo burstPosition;    
    
    void SendPacket(bool burst = false);
    bool CanSend();

    bool BelowHorizon() {
      return (!positionValid) || position.altitude < horizon;
    }
  public:
    Beacon();
    void Setup();
    void Update();
};