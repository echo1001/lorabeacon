#include "beacon.h"

Beacon::Beacon(): 
  gnss(), 
  lora(3, 8, 4)
  #ifdef INTERNALSENSOR
  , internalSensor()
  #endif 
{

}

void updateData(UBX_NAV_PVT_data_t ubxDataStruct) {
  beacon.updatePosition(ubxDataStruct);
}

void Beacon::updatePosition(UBX_NAV_PVT_data_t ubxDataStruct) {

  position.altitude = (double)ubxDataStruct.hMSL / 1000.0;
  position.latitude = (double)ubxDataStruct.lat / (double)10000000.0;
  position.longitude = (double)ubxDataStruct.lon / (double)10000000.0;
  position.satellites = ubxDataStruct.numSV;
  position.course = (double)ubxDataStruct.headMot / (double)100000.0;
  position.speed = (double)ubxDataStruct.gSpeed / 1000.0;


  position.battery = analogRead(A7);
  position.battery *= 2;   
  position.battery *= 3.3; 
  position.battery /= 1024;

  #ifdef INTERNALSENSOR
  position.pressure    = internalSensor.readFloatPressure() / 100.0;
  position.temperature = internalSensor.readTempC();
  #endif

  if (position.satellites > 4 && ubxDataStruct.lat != 0 && ubxDataStruct.lon != 0) // Reduce bogus nav data, 3D fix only available with >=4 sattelites
    positionValid = true;
  else positionValid = false;

  if (!didBurst && positionValid && position.altitude > burstPosition.altitude) {
    burstPosition = position;
  }

}

void Beacon::Setup() {
  delay(2000);
  #ifdef DEBUG
  Serial.begin(115200);
  while (!Serial);
  #endif

  LOG("Initializing...");


  Wire.begin();

  if (!gnss.begin())
  {
    LOG("GNSS not detected");
    while (1);
  }

  gnss.setI2COutput(COM_TYPE_UBX);
  gnss.setDynamicModel(DYN_MODEL_AIRBORNE2g);
  gnss.setNavigationFrequency(1); 
  gnss.setAutoPVTcallback(&updateData); // Enable automatic NAV PVT messages with callback to printPVTdata

  #ifdef INTERNALSENSOR
  if (!internalSensor.beginI2C()) {
    LOG("Interal sensor not detected");
    while (1);
  }
  #endif
  
  if(!lora.begin())
  {
    LOG("RFM95 not detected");
    delay(5000);
    
    if(!lora.begin())
    {
      return;
    }
  }

  lora.setChannel(MULTI);
  lora.setDatarate(SF9BW125);
  lora.setPower(20);
}

bool Beacon::CanSend() {
  unsigned long sendWait = 0;
  if (!positionValid) return false; // No proper fix, don't beacon

  sendWait = 15000L;

  if (lastPacket == 0 || (lastPacket + sendWait + randomDelay) <= millis())
    return true;

  return false;
}

void Beacon::Update() {
  gnss.checkUblox();
  gnss.checkCallbacks();

  #ifdef DEBUG
  if (Serial.available()) {
    auto c = Serial.read();
    switch (c) {
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

  if (CanSend()) { // Are we in our xmit window?
    SendPacket(false);
  } else if (!didBurst && (burstPosition.altitude - position.altitude) > 300) {
    // The balloon has burst, transmit outside of the standard beacon window
    SendPacket(true);
    didBurst = true;
  }
}

// 3 = Telemetry
// 4 = Burst info

void Beacon::SendPacket(bool burst) {
  unsigned char data[64];
  unsigned int dataLen;

  Telemetry telemetry = burst ? burstPosition : position;

  long _altitude = (long)telemetry.altitude;
  long _maxaltitude = (long)burstPosition.altitude;
  
  double gpsLat = telemetry.latitude;
  double gpsLon = telemetry.longitude;

  _altitude = min(max(0, _altitude), 0xFFFF);
  _maxaltitude = min(max(0, _maxaltitude), 0xFFFF);
  long _gpsLat = (int)gpsLat;
  long _gpsLon = (int)gpsLon;
  
  _gpsLat = gpsLat * 90000;
  _gpsLon = gpsLon * 45000;

  _gpsLat += 8100000;
  _gpsLon += 8100000;

  int _course = round(telemetry.course / ((double)360 / (double)0x7FF));
  _course = min(max(0, _course), 0x7FF);

  int _speed = round(telemetry.speed / (double)0.25);
  _speed = min(max(0, _speed), 0xFF);

  int _satelites = min(max(0, (int)telemetry.satellites), 0xf);
  uint8_t port = 3;

  int _temperature = min(max( round(((round(telemetry.temperature * 10) / 10.0) + 30.0)/100.0*1023.0), 0), 0x3ff);
  int _pressure = min(max( round(telemetry.pressure), 0), 0x3ff);

  LOG(telemetry.temperature);
  LOG(telemetry.pressure);

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
  data[11] = min(max(( (telemetry.battery - 2.7) / 1.6 ) * 255.0, 0.0), 255.0);

  data[12] = (_maxaltitude >> 8) & 0xFF;
  data[13] = _maxaltitude & 0xFF;

  data[14] = (_temperature >> 2) & 0xFF;
  data[15] = (_pressure >> 2) & 0xFF;
  data[16] = ((_temperature & 3) << 6) + ((_pressure & 3) << 4);

  dataLen = 17;
  
  if (burst) port = 4;
  else positionValid = false; // Don't retransmit the same position

  #ifdef DEBUG
  if (burst) {LOG2("Burst @ ", telemetry.altitude);}
  else LOG("Beaconing");
  #endif

  lora.sendData(data, dataLen, lora.frameCounter, port);
  lora.frameCounter++;

  lastPacket = millis();
  randomDelay = random(0, 1000);
}