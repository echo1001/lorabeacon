#include <Arduino.h>
#include "beacon.h"

#include "config.h"

Beacon beacon;

void setup()
{
  randomSeed(analogRead(0));
  beacon.Setup();
}

void loop()
{
  beacon.Update();
}