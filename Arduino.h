 //////////////////////////////////////////////////////////////////////////////
// FILE:          Arduino.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   Adapter for Arduino board
//                Needs accompanying firmware to be installed on the board
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
//

#ifndef _Arduino_H_
#define _Arduino_H_

#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"
#include <string>
#include <map>

//////////////////////////////////////////////////////////////////////////////
// Error codes
//
#define ERR_UNKNOWN_POSITION 101
#define ERR_INITIALIZE_FAILED 102
#define ERR_WRITE_FAILED 103
#define ERR_CLOSE_FAILED 104
#define ERR_BOARD_NOT_FOUND 105
#define ERR_PORT_OPEN_FAILED 106
#define ERR_COMMUNICATION 107
#define ERR_NO_PORT_SET 108
#define ERR_VERSION_MISMATCH 109

class ArduinoInputMonitorThread;

class CArduinoHub : public HubBase<CArduinoHub>  
{
public:
   CArduinoHub();
   ~CArduinoHub();

   int Initialize();
   int Shutdown();
   void GetName(char* pszName) const;
   bool Busy();

   bool SupportsDeviceDetection(void);
   MM::DeviceDetectionStatus DetectDevice(void);
   int DetectInstalledDevices();

   // property handlers
   int OnPort(MM::PropertyBase* pPropt, MM::ActionType eAct);
   int OnLogic(MM::PropertyBase* pPropt, MM::ActionType eAct);
   int OnVersion(MM::PropertyBase* pPropt, MM::ActionType eAct);

   // custom interface for child devices
   bool IsPortAvailable() {return portAvailable_;}
   int Purge() {return PurgeComPort(port_.c_str());}
   int Write(const unsigned char* command, unsigned len) {return WriteToComPort(port_.c_str(), command, len);}
   int Read(unsigned char* answer, unsigned maxLen, unsigned long& bytesRead)
   {
      return ReadFromComPort(port_.c_str(), answer, maxLen, bytesRead);
   }

   static MMThreadLock& GetLock() {return lock_;}

   void SetFilterWheelState(unsigned state) {filterWheelState_ = state;}
   unsigned GetFilterWheelState() {return filterWheelState_;}

private:
   int GetControllerVersion(int&);

   std::string port_;
   bool initialized_;
   bool portAvailable_;
   int version_;
   static MMThreadLock lock_;
   unsigned filterWheelState_;
};

class ArduinoInputMonitorThread : public MMDeviceThreadBase
{
   public:
      ArduinoInputMonitorThread(CArduinoInput& aInput);
     ~ArduinoInputMonitorThread();
      int svc();
      int open (void*) { return 0;}
      int close(unsigned long) {return 0;}

      void Start();
      void Stop() {stop_ = true;}
      ArduinoInputMonitorThread & operator=( const ArduinoInputMonitorThread & ) 
      {
         return *this;
      }


   private:
      long state_;
      CArduinoInput& aInput_;
      bool stop_;
};


#endif //_Arduino_H_
