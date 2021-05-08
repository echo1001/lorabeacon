let lat = 38.073411
let lon = -97.910161

let alt = 400;
let course = 359.80;
let speed = 5.2;
let satellites = 13
let battery = 3.3
let pressure = 18;
let temp = -20.24;
let maxAltitude = 30000

let valid = true

bytes = []
lat += 90;
lat *= 3600 * 25;
lon += 180;
lon *= 3600 * 12.5;

alt = Math.min(Math.max(0, alt), 0xFFFF);
maxAltitude = Math.min(Math.max(0, maxAltitude), 0xFFFF);

course = Math.round(course / (360 / 0x7FF));
course = Math.min(Math.max(0, course), 0x7FF);

speed = Math.round(speed / (50 / 0xFF));
speed = Math.min(Math.max(0, speed), 0xff);

satellites = Math.min(Math.max(0, satellites), 0xf);

temp = Math.round(temp * 10) / 10;
temp = Math.min( Math.max(0, Math.round((temp + 30) / 100 * 1023.0)), 1023.0)
pressure = Math.min( Math.max(0, Math.round(pressure)), 1023.0)

bytes.push(((lat >> 16) & 0xFF))
bytes.push((lat >> 8) & 0xFF)
bytes.push(lat & 0xFF)

bytes.push((lon >> 16) & 0xFF)
bytes.push((lon >> 8) & 0xFF)
bytes.push(lon & 0xFF)

bytes.push((alt >> 8) & 0xFF)
bytes.push(alt & 0xFF)

bytes.push(((course >> 8) & 0xFF) + (satellites << 4) + (valid ? 8 : 0 ) )
bytes.push(course & 0xFF )

//bytes.push((speed >> 8) & 0xFF )
bytes.push(speed & 0xFF )
// min(max(( (telemetry.battery - 2.7) / 1.6 ) * 255.0, 0.0), 255.0)
bytes.push(Math.min( Math.max(0, ((battery - 2.7) / 1.6 ) * 255.0), 255))
bytes.push((maxAltitude >> 8) & 0xFF)
bytes.push(maxAltitude & 0xFF)
console.log(temp, (Math.round(temp / 1.023) / 10 ) - 30)
console.log(pressure)

bytes.push((temp >> 2) & 0xFF)
bytes.push((pressure >> 2) & 0xFF)

bytes.push(((temp & 3) << 6) + ((pressure & 3) << 4))



function buf2hex(buffer) { // buffer is an ArrayBuffer
  return Array.prototype.map.call(new Uint8Array(buffer), x => ('00' + x.toString(16)).slice(-2)).join('');
}

//console.log(bytes)
//console.log(bytes.length)

const buffer = new Uint8Array(bytes).buffer;
console.log(buf2hex(buffer)); // = 04080c10