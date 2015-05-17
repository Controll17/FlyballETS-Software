// BatterySensor.h

#ifndef _BATTERYSENSOR_h
#define _BATTERYSENSOR_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

class BatterySensorClass
{
protected:

public:
   void init(int iBatterySensorPin);
   void CheckBatteryVoltage();
   uint16_t GetBatteryVoltage();
   uint16_t GetBatteryPercentage();
   
private:
   int _iBatterySensorPin;

   int _iBatteryReadings[10];
   int _iNumberOfBatteryReadings = 0;
   float _fBatteryVoltage = 0;
   
};

extern BatterySensorClass BatterySensor;

#endif

