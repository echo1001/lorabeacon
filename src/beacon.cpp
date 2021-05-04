#include "beacon.h"

Beacon::Beacon(): gps(), lora(7, 8, 4) {

}

void Beacon::Setup() {

  #ifdef DEBUG
  Serial.begin(115200);
  while (!Serial);
  #endif

  Serial1.begin(9600);
  if(!lora.begin())
  {
    #ifdef DEBUG
    Serial.println("RFM95 not detected");
    #endif
    delay(5000);
    return;
  }
  #ifdef DEBUG
  Serial.println("Initializing...");
  #endif
  lora.setChannel(MULTI);
  lora.setDatarate(SF10BW125);
  lora.setPower(20);
}

bool Beacon::CanSend() {
  unsigned long sendWait = 0;
  if (!positionValid) return false; // No proper fix, don't beacon

  if (BelowHorizon()) {
    // 20s in lower altitudes. In lower altitudes, we use a higher spread factor and to prevent accidentally violating FCC
    // regulations, 20s is ideal;
    sendWait = 20000L; 
  } else {
    sendWait = 10000L; // Slightly quicker since we have some airtime available
  }

  if (lastPacket == 0 || (lastPacket + sendWait + randomDelay) <= millis())
    return true;

  return false;
}

void Beacon::Update() {
  while(Serial1.available()) {
    auto d = Serial1.read();
    gps.encode(d);
  }

  #ifdef DEBUG
  if (Serial.available()) {
    auto c = Serial.read();
    switch (c) {
      case 'h': // Simulate high altitude
        horizon = 0.0;
        break;
      case 'l': // Simulate low altitude
        horizon = 900.0;
        break;
      case 'b': // Test burst
        burstPosition = position;
        burstPosition.altitude += 301.0;
        didBurst = false;
        break;
      default:
        break;
    }
  }
  #endif

  if (gps.location.isUpdated() && gps.altitude.isUpdated()) {
    position.altitude = gps.altitude.meters();
    position.latitude = gps.location.lat();
    position.longitude = gps.location.lng();
    position.satellites = gps.satellites.value();
    position.course = gps.course.deg();
    position.speed = gps.speed.mps();

    position.battery = analogRead(A9);
    position.battery *= 2;   
    position.battery *= 3.3; 
    position.battery /= 1024;

    if (position.satellites > 4)
      positionValid = true;
    else positionValid = false;

    if (positionValid && position.altitude > burstPosition.altitude) {
      burstPosition = position;
    }
  }

  if (CanSend()) { // Are we in our xmit window?
    SendPacket(false);
  } else if (!didBurst && (burstPosition.altitude - position.altitude) > 300) {
    // The balloon has burst, transmit outside of the standard beacon window
    SendPacket(true);
    didBurst = true;
  }
}

// 2 = Low altitude position
// 3 = High altitude telemetry
// 4 = Burst info

void Beacon::SendPacket(bool burst) {
  unsigned char data[64];
  unsigned int dataLen;

  double    altitude    = 0.0;
  double    latitude    = 0.0;
  double    longitude   = 0.0;
  double    course      = 0.0;
  double    speed       = 0.0;
  uint32_t  satellites  = 0;
  float     battery     = 0.0;

  if (burst) {

    altitude   = burstPosition.altitude  ;
    latitude   = burstPosition.latitude  ;
    longitude  = burstPosition.longitude ;
    course     = burstPosition.course    ;
    speed      = burstPosition.speed     ;
    satellites = burstPosition.satellites;
    battery    = burstPosition.battery   ;

  } else {

    altitude   = position.altitude  ;
    latitude   = position.latitude  ;
    longitude  = position.longitude ;
    course     = position.course    ;
    speed      = position.speed     ;
    satellites = position.satellites;
    battery    = position.battery   ;

  }

  long _altitude = (long)altitude;
  double gpsLat = latitude;
  double gpsLon = longitude;

  _altitude = min(max(0, _altitude), 0xFFFF);
  long _gpsLat = (int)gpsLat;
  long _gpsLon = (int)gpsLon;
  
  _gpsLat = gpsLat * 90000;
  _gpsLon = gpsLon * 45000;

  _gpsLat += 8100000;
  _gpsLon += 8100000;


  int _course = round(course / ((double)360 / (double)0x7FF));
  _course = min(max(0, _course), 0x7FF);

  int _speed = round(speed / (double)0.25);
  _speed = min(max(0, _speed), 0xFF);

  int _satelites = min(max(0, (int)satellites), 0xf);
  uint8_t port = 2;

  data[0] = (_gpsLat >> 16) & 0xff;
  data[1] = (_gpsLat >> 8) & 0xff;
  data[2] = _gpsLat & 0xff;

  data[3] = (_gpsLon >> 16) & 0xff;
  data[4] = (_gpsLon >> 8) & 0xff;
  data[5] = _gpsLon & 0xff;
  
  data[6] = (_altitude >> 8) & 0xFF;
  data[7] = _altitude & 0xFF;

  data[8] = ((_course >> 8) & 0xFF) + (_satelites << 4) + 8;
  data[9] = _course & 0xFF;

  data[10] = _speed & 0xFF;

  dataLen = 11;
  
  if (!BelowHorizon() || burst) {
    data[11] = min(max(( (battery - 2.7) / 1.6 ) * 255.0, 0.0), 255.0);
    dataLen += 1;
    port++;
  }
  if (burst) port = 4;

  #ifdef DEBUG
  if (burst) {Serial.print("Burst @"); Serial.println(burstPosition.altitude);}
  else Serial.println("Beaconing");
  #endif

  if (BelowHorizon() && !burst) lora.setDatarate(SF10BW125);
  else lora.setDatarate(SF8BW125);

  lora.sendData(data, dataLen, lora.frameCounter, port);
  lora.frameCounter++;

  lastPacket = millis();
  randomDelay = random(0, 1000);
}