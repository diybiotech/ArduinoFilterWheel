///////////////////////////////////////////////////////////////////////////////
// FILE:          FilterWheel.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   Implementation of Filter Wheel controlled by Arduino.
//                
// AUTHOR:        Shirish Goyal, shirish.goyal@gmail.com 09/14/2016
//
// COPYRIGHT:     Shirish Goyal, BioCurious, Sunnyvale, 2016
//
// LICENSE:       This file is distributed under the BSD license.
//                License text is included with the source distribution.
//
//                This file is distributed in the hope that it will be useful,
//                but WITHOUT ANY WARRANTY; without even the implied warranty
//                of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//                IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//                CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES.


#ifndef _ARDUINOFILTERWHEEL_H_
#define _ARDUINOFILTERWHEEL_H_

#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"
#include <cstdlib>
#include <string>
#include <map>
#include <math.h>
#include <sstream>

//////////////////////////////////////////////////////////////////////////////
// Error codes
//
#define ERR_UNKNOWN_MODE         102
#define ERR_UNKNOWN_POSITION     103


//////////////////////////////////////////////////////////////////////////////
// ArduinoFilterWheel class
// Simulation of the filter changer (state device)
//////////////////////////////////////////////////////////////////////////////
template <class T>
bool from_string(T& t, 
                 const std::string& s, 
                 std::ios_base& (*f)(std::ios_base&))
{
  std::istringstream iss(s);
  return !(iss >> f >> t).fail();
}


class ArduinoFilterWheel : public CStateDeviceBase<ArduinoFilterWheel>
{
public:
   ArduinoFilterWheel();
   ~ArduinoFilterWheel();

   int Initialize();
   int Shutdown();
   void GetName(char* pszName) const;
   bool Busy();
   unsigned long GetNumberOfPositions()const {return numPos_;}

   int OnState(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnCOMPort(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   unsigned long numPos_;
   bool initialized_;
   MM::MMTime changedTime_;
   long position_;
   std::string port_;

   void InitializeFilterWheel();
};


#endif //_ARDUINOFILTERWHEEL_H_
