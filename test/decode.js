function DecodePosition(decoded, bytes) {
  //decoded.temperature = Math.floor(((bytes[0] << 8 | bytes[1]) / 327.675 - 100) * 100) / 100;
  decoded.latitude = bytes[0] << 16 | bytes[1] << 8 | bytes[2];
  decoded.latitude /= 3600 * 25;
  decoded.latitude -= 90;
  
  decoded.longitude = bytes[3] << 16 | bytes[4] << 8 | bytes[5];
  decoded.longitude /= 3600 * 12.5;
  decoded.longitude -= 180;
  
  decoded.altitude = ((bytes[6]) << 8) + bytes[7];

  decoded.course = ((bytes[8] & 0x07) << 8) + bytes[9];
  decoded.sats = (bytes[8] & 0xF0) >> 4;
  decoded.course *= 360 / 0x7ff;

  decoded.speed = bytes[10];
  decoded.speed *= 0.25;
  decoded.valid = (bytes[8] & 0x08) > 0;
  return decoded;
}

function DecodeTelemetry(decoded, bytes) {
  decoded.battery = 2.7 + ((bytes[11] / 255.0) * 1.6);
  decoded.maxAltitude = ((bytes[12]) << 8) + bytes[13];

  decoded.temperature = ((bytes[14]) << 2) + ((bytes[16] >> 6) & 3);
  decoded.temperature = (Math.round(decoded.temperature / 1.023) / 10) - 30;
  
  decoded.pressure = ((bytes[15]) << 2) + ((bytes[16] >> 4) & 3);

  return decoded;
}

function decodeUplink(input) {
  var bytes = input.bytes;
  var port = input.fPort;

  var decoded = {
    /* Position */
    latitude    : 0,
    longitude   : 0,
    altitude    : 0,
    course      : 0,
    sats        : 0,
    speed       : 0,

    /* Telemetry */
    battery     : 0,
    maxAltitude : 0,
    temperature : 0,
    pressure    : 0,

    /* Type */
    valid       : false,
    beacon      : false,
    telemetry   : false,
    burst       : false
  };

  if (port === 2) { // Low alt. beacon

    decoded.beacon = true;
    decoded = DecodePosition(decoded, bytes);

  } else if (port === 3) { // High alt. beacon

    decoded.beacon = true;
    decoded.telemetry = true;
    decoded = DecodePosition(decoded, bytes);
    decoded = DecodeTelemetry(decoded, bytes);

  } else if (port === 4) { // Burst

    decoded.burst = true;
    decoded.telemetry = true;
    decoded = DecodePosition(decoded, bytes);
    decoded = DecodeTelemetry(decoded, bytes);

  }

  return {
    data: decoded,
    warnings: [],
    errors: []
  };
}