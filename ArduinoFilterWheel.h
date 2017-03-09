 //////////////////////////////////////////////////////////////////////////////
// FILE:          ArduinoFilterWheel.h
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

#ifndef _CArduinoFilterWheelHub_H_
#define _CArduinoFilterWheelHub_H_

#include "MMDevice.h"
#include "DeviceBase.h"
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

class CArduinoFilterWheelHub: public HubBase<CArduinoFilterWheelHub> {
public:
    CArduinoFilterWheelHub();
    ~CArduinoFilterWheelHub();

    int Initialize();
	int Shutdown();
    bool Busy();
    void GetName(char *name) const;
	//void GetPort(char *port) const;

    bool SupportsDeviceDetection(void);
    MM::DeviceDetectionStatus DetectDevice(void);
    int DetectInstalledDevices();

    // property handlers
    int OnPort(MM::PropertyBase* pPropt, MM::ActionType eAct);
    int OnVersion(MM::PropertyBase* pPropt, MM::ActionType eAct);

    // custom interface for child devices
    bool IsPortAvailable() {return portAvailable_;}

    int PurgeComPortH() {return PurgeComPort(port_.c_str());}
    int WriteToComPortH(const char* command, size_t len);
    //{
    //   return WriteToComPort(port_.c_str(), command, static_cast<unsigned>(len));
    //}
    int ReadFromComPortH(unsigned char* answer, unsigned maxLen, unsigned long& bytesRead)
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
    unsigned int filterWheelState_;
};

class CArduinoFilterWheel : public CStateDeviceBase<CArduinoFilterWheel>
{
public:
   CArduinoFilterWheel();
   ~CArduinoFilterWheel();

   // MMDevice API
   // ------------
   int Initialize();
   int Shutdown();

   void GetName(char* pszName) const;
   bool Busy();

   unsigned long GetNumberOfPositions()const {return numPos_;}

   int OnState(MM::PropertyBase* pProp, MM::ActionType eAct);
   //int OnCOMPort(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   int WriteToPort(unsigned long lnValue);
   unsigned long numPos_;
   bool initialized_;
   bool busy_;
   MM::MMTime changedTime_;
   long position_;
   std::string port_;
   std::string name_;
};


#endif //ARDUINOFILTERWHEEL_CArduinoFilterWheelHub_H