#pragma once
#include <Arduino.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
  #define DPRINT(x)   Serial.print(x)
  #define DPRINTLN(x) Serial.println(x)
#else
  #define DPRINT(x)   do {} while(0)
  #define DPRINTLN(x) do {} while(0)
#endif