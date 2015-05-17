// 
// 
// 
#include "LightsController.h"
#include "RaceHandler.h"
#include "global.h"

void RaceHandlerClass::init(int iS1Pin, int iS2Pin)
{
   //Start in stopped state
   _ChangeRaceState(STOPPED);
   _iS1Pin = iS1Pin;
   _iS2Pin = iS2Pin;

   ResetRace();

}

void RaceHandlerClass::_ChangeRaceState(RaceStates _byNewRaceState)
{
   //First check if the new state (this function could be called superfluously)
   if (RaceState != _byNewRaceState)
   {
      PreviousRaceState = RaceState;
      RaceState = _byNewRaceState;
   }
}

void RaceHandlerClass::_ChangeDogState(_byDogStates _byNewDogState)
{
   if (_byDogState != _byNewDogState)
   {
      _byDogState = _byNewDogState;
   }
}

void RaceHandlerClass::_ChangeDogNumber(int _iNewDogNumber)
{
   //Check if the dog really changed (this function could be called superfluously)
   if (_iNewDogNumber != iCurrentDog)
   {
      iPreviousDog = iCurrentDog;
      iCurrentDog = _iNewDogNumber;
   }
}

void RaceHandlerClass::Main()
{
   //Don't handle anything if race is stopped
   if (RaceState == STOPPED)
   {
      return;
   }
   if (_lNewS1Time > 0)
   {
      if (bDEBUG) printf(F("S1|T:%lu|St:%i\r\n"), _lNewS1Time, _iS1TriggerState);
      //Check what current state of race is
      if (RaceState != STOPPED
         && (_lNewS1Time - _lPrevS2Time) > 100000 //100 ms debounce to avoid dog coming back immediately triggering the next dog
         && _iS1TriggerState == 1)
      {
         //Potential fault occured
         //Special handling for dog 0 when it's not yet in the lane
         if (iCurrentDog == 0
            && _byDogState == GOINGIN
            && _lNewS1Time < _lPerfectCrossingTime)
         {
            //Dog 0 is too early!
            SetDogFault(iCurrentDog, ON);
            if (bDEBUG) printf("F! D:%i!\r\n", iCurrentDog);
            _lCrossingTimes[iCurrentDog][_iDogRunCounters[iCurrentDog]] = _lNewS1Time - _lPerfectCrossingTime;
            _ChangeDogState(COMINGBACK);
         }
         //Check if this is a next dog which is too early (we are expecting a dog to come back
         else if (_byDogState == COMINGBACK)
         {
            //This dog is too early!
            //We assume previous dog came in at the more or less the same time
            //TODO: Since the previous dog didn't come in yet, we don't know what the perfect crossing time is
            //Therefor we can't display a negative crossing time on the display
            //So we don't set the CrossingTime for this dog (it stays +0.000s)
            _lDogExitTimes[iCurrentDog] = _lNewS1Time;
            _lDogTimes[iCurrentDog][_iDogRunCounters[iCurrentDog]] = _lNewS1Time - _lDogEnterTimes[iCurrentDog];
            _ChangeDogNumber(iCurrentDog + 1);
            _lDogEnterTimes[iCurrentDog] = _lNewS1Time;
            if (bDEBUG) printf("F! D:%i!\r\n", iCurrentDog);
            SetDogFault(iCurrentDog, ON);
         }

         //Normal race handling (no faults)
         if (_byDogState == GOINGIN)
         {
            //Store crossing time only when dogstate was GOINGIN
            _lCrossingTimes[iCurrentDog][_iDogRunCounters[iCurrentDog]] = _lNewS1Time - _lPerfectCrossingTime;
            _ChangeDogState(COMINGBACK);

            //If this dog is doing a rerun we have to turn the error light for this dog off
            if (_bRerunBusy)
            {
               //remove fault for this dog
               SetDogFault(iCurrentDog, OFF);
            }
         }
      }
      _lPrevS1Time = _lNewS1Time;
      _lNewS1Time = 0;
   }

   if (_lNewS2Time > 0)
   {
      if (bDEBUG) printf("S2|T:%lu|St:%i\r\n", _lNewS2Time, _iS2TriggerState);
      //Only check 2nd sensor if we're expecting a dog to come back
      if (_byDogState == COMINGBACK)
      {
         //Check how long current dog is away, we expect it to be away at least 2 seconds
         if (_lNewS2Time - _lPrevS1Time > 2000000
            && _iS2TriggerState == 1)
         {
            _lDogExitTimes[iCurrentDog] = _lNewS2Time;
            _lDogTimes[iCurrentDog][_iDogRunCounters[iCurrentDog]] = _lNewS2Time - _lDogEnterTimes[iCurrentDog];
            //The time the dog came in is also the perfect crossing time
            _lPerfectCrossingTime = _lNewS2Time;
            //Set race back to expect incoming dog
            _ChangeDogState(GOINGIN);
            
            if ((iCurrentDog == 3 && _bFault == false && _bRerunBusy == false) //If this is the 4th dog and there is no fault we have to stop the race
                  || (_bRerunBusy == true && _bFault == false))                //Or if the rerun sequence was started but no faults exist anymore
            {
               _lRaceEndTime = _lNewS2Time;
               _lRaceTime = _lRaceEndTime - _lRaceStartTime;
               _ChangeRaceState(STOPPED);
            }
            else if ((iCurrentDog == 3 && _bFault == true && _bRerunBusy == false)  //If current dog is dog 4 and a fault exists, we have to initiate rerun sequence
                     || _bRerunBusy == true)                                        //Or if rerun is busy (and faults still exist)
            {
               //Dog 3 came in but there is a fault, we have to initiate the rerun sequence
               _bRerunBusy = true;
               for (int i = 0; i < 4; i++)
               {
                  if (_bDogFaults[i])
                  {
                     //Set dognumber for first offending dog
                     _ChangeDogNumber(i);
                     //Reset timers for this dog
                     _lDogEnterTimes[i] = _lNewS2Time;
                     _lDogExitTimes[i] = 0;
                     //Increase run counter for this dog
                     _iDogRunCounters[i]++;
                     break;
                  }
               }
               if (bDEBUG) printf("RR! D:%i\r\n", millis(), iCurrentDog);
            }
            else
            {
               //And increase dog number
               _ChangeDogNumber(iCurrentDog + 1);
               //Store next dog enter time
               _lDogEnterTimes[iCurrentDog] = _lNewS2Time;
            }
         }
      }
      _lPrevS2Time = _lNewS2Time;
      _lNewS2Time = 0;
   }
   //Update racetime
   if (RaceState == RUNNING)
   {
      _lRaceTime = micros() - _lRaceStartTime;
   }

   //Check for faults, loop through array of dogs checking for faults
   _bFault = false;
   for (auto bFault : _bDogFaults)
   {
      if (bFault)
      {
         //At least one dog with fault is found, set general fault value to true
         _bFault = true;
         break;
      }
   }
}

void RaceHandlerClass::StartTimers()
{
   _ChangeRaceState(RUNNING);
   _lRaceStartTime = micros();
   _lPerfectCrossingTime = _lRaceStartTime;
   //Store dog 1 enter time
   _lDogEnterTimes[0] = _lRaceStartTime;
}

void RaceHandlerClass::StartRace()
{
   _ChangeRaceState(STARTING);
   _lRaceStartTime = micros();
   _lPerfectCrossingTime = _lRaceStartTime + 3000000;
}

void RaceHandlerClass::StopRace()
{
   if (RaceState == RUNNING)
   {
      //Race is running, so we have to record the EndTime
      _lRaceEndTime = micros();
      _lRaceTime = _lRaceEndTime - _lRaceStartTime;
   }
   _ChangeRaceState(STOPPED);
}

void RaceHandlerClass::ResetRace()
{
   if (RaceState == STOPPED)
   {
      iCurrentDog = 0;
      iPreviousDog = 0;
      _lRaceStartTime = 0;
      _lRaceEndTime = 0;
      _lRaceTime = 0;
      _lPerfectCrossingTime = 0;
      _byDogState = GOINGIN;
      _ChangeDogNumber(0);
      _bFault = false;
      _bRerunBusy = false;
      
      for (auto& bFault : _bDogFaults)
      {
         bFault = false;
      }
      for (auto& lTime : _lDogEnterTimes)
      {
         lTime = 0;
      }
      for (auto& lTime : _lDogExitTimes)
      {
         lTime = 0;
      }
      for (auto& Dog : _lDogTimes)
      {
         for (auto& lTime : Dog)
         {
            lTime = 0;
         }
      }
      for (auto& Dog : _lCrossingTimes)
      {
         for (auto& lTime : Dog)
         {
            lTime = 0;
         }
      }
      for (auto& iCounter : _iDogRunCounters)
      {
         iCounter = 0;
      }
      for (auto& lTime : _lLastDogTimeReturnTimeStamp)
      {
         lTime = 0;
      }
      for (auto& iCounter : _iLastReturnedRunNumber)
      {
         iCounter = 0;
      }
   }
}
void RaceHandlerClass::SetDogFault(int iDogNumber, DogFaults State)
{
   bool bFault;
   //Check if we have to toggle
   if (State == TOGGLE)
   {
      bFault = !_bDogFaults[iDogNumber];
   }
   else
   {
      bFault = State;
   }

   //Set fault to specified value for relevant dog
   _bDogFaults[iDogNumber] = bFault;

   //If fault is true, set light to on and set general value fault variable to true
   if (bFault)
   {
      LightsController.ToggleFaultLight(iDogNumber, LightsController.ON);
      _bFault = true;
   }
   else
   {
      //If fault is false, turn off fault light for this dog
      LightsController.ToggleFaultLight(iDogNumber, LightsController.OFF);
   }
}

void RaceHandlerClass::TriggerSensor1()
{
   _lNewS1Time = micros();
   _iS1TriggerState = digitalRead(_iS1Pin);
}

void RaceHandlerClass::TriggerSensor2()
{
   _lNewS2Time = micros();
   _iS2TriggerState = digitalRead(_iS2Pin);
}

double RaceHandlerClass::GetRaceTime()
{
   double dRaceTimeSeconds = 0;
   if (RaceState != STARTING)
   {
      dRaceTimeSeconds = _lRaceTime / 1000000.0;
   }

   return dRaceTimeSeconds;
}

double RaceHandlerClass::GetDogTime(int iDogNumber)
{
   double dDogTimeSeconds = 0;

   uint8_t& iRunNumber = _iLastReturnedRunNumber[iDogNumber];
   auto& lLastReturnedTimeStamp = _lLastDogTimeReturnTimeStamp[iDogNumber];
   if (_iDogRunCounters[iDogNumber] > 0)
   {
      //We have multiple crossing times for this dog, so we must cycle through them
      if ((millis() - lLastReturnedTimeStamp) > 2000)
      {
         if (iRunNumber == _iDogRunCounters[iDogNumber])
         {
            iRunNumber = 0;
         }
         else
         {
            iRunNumber++;
         }
         lLastReturnedTimeStamp = millis();
      }
   }

   //First check if we have final time for the requested dog number
   if (_lDogTimes[iDogNumber][iRunNumber] > 0)
   {
      dDogTimeSeconds = _lDogTimes[iDogNumber][iRunNumber] / 1000000.0;
   }
   //Then check if the requested dog is perhaps running (and coming back) so we can return the time so far
   else if (RaceState == RUNNING && iCurrentDog == iDogNumber && _byDogState == COMINGBACK)
   {
      dDogTimeSeconds = (micros() - _lDogEnterTimes[iDogNumber]) / 1000000.0;
   }
   dDogTimeSeconds -= (_lCrossingTimes[iDogNumber][iRunNumber] / 1000000.0);
   return dDogTimeSeconds;
}

String RaceHandlerClass::GetCrossingTime(int iDogNumber)
{
   double dCrossingTime = 0;
   char cCrossingTime[8];
   String strCrossingTime;

   uint8_t& iRunNumber = _iLastReturnedRunNumber[iDogNumber];
   auto& lLastReturnedTimeStamp = _lLastDogTimeReturnTimeStamp[iDogNumber];
   if (_iDogRunCounters[iDogNumber] > 0)
   {
      //We have multiple crossing times for this dog, so we must cycle through them
      if ((millis() - lLastReturnedTimeStamp) > 2000)
      {
         if (iRunNumber == _iDogRunCounters[iDogNumber])
         {
            iRunNumber = 0;
         }
         else
         {
            iRunNumber++;
         }
         lLastReturnedTimeStamp = millis();
      }
   }
   dCrossingTime = _lCrossingTimes[iDogNumber][iRunNumber] / 1000000.0;
   if (dCrossingTime < 0)
   {
      dCrossingTime = fabs(dCrossingTime);
      strCrossingTime = "-";
   }
   else
   {
      strCrossingTime = "+";
   }
   dtostrf(dCrossingTime, 7, 3, cCrossingTime);
   strCrossingTime += cCrossingTime;
   
   return strCrossingTime;
}

String RaceHandlerClass::GetRerunInfo(int iDogNumber)
{
   String strRerunInfo = "  ";
   
   uint8_t iRunNumber = _iLastReturnedRunNumber[iDogNumber];
   if (_iDogRunCounters[iDogNumber] > 0)
   {
      strRerunInfo = "*";
      strRerunInfo += (iRunNumber + 1);
   }
   return strRerunInfo;
}

double RaceHandlerClass::GetTotalCrossingTime()
{
   long lTotalCrossingTime = 0;

   for (auto& Dog : _lCrossingTimes)
   {
      for (auto& lTime : Dog)
      {
         lTotalCrossingTime += lTime;
      }
   }
   double dTotalCrossingTime = lTotalCrossingTime / 1000000.0;
   return dTotalCrossingTime;
}

String RaceHandlerClass::GetRaceStateString()
{
   String strRaceState;
   switch (RaceState)
   {
   case RaceHandlerClass::STOPPED:
      strRaceState = " STOP";
      break;
   case RaceHandlerClass::STARTING:
      strRaceState = " START";
      break;
   case RaceHandlerClass::RUNNING:
      strRaceState = "RUNNING";
      break;
   default:
      break;
   }

   return strRaceState;
}

RaceHandlerClass RaceHandler;

