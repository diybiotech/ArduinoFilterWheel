//
// Created by Shirish Goyal on 9/14/16.
//

#ifndef ARDUINOHUB_H
#define ARDUINOHUB_H

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

class ArduinoHub: public HubBase<ArduinoHub> {
public:
    ArduinoHub();
    ~ArduinoHub();

    virtual int Initialize();
    virtual bool Busy();
    virtual int Shutdown();
    virtual void GetName(char *name) const;

    bool SupportsDeviceDetection(void);
    MM::DeviceDetectionStatus DetectDevice(void);
    int DetectInstalledDevices();

    // property handlers
    int OnPort(MM::PropertyBase* pPropt, MM::ActionType eAct);
    int OnLogic(MM::PropertyBase* pPropt, MM::ActionType eAct);
    int OnVersion(MM::PropertyBase* pPropt, MM::ActionType eAct);

    // custom interface for child devices
    bool IsPortAvailable() {
        return portAvailable_;
    }

    int Purge() {
        return PurgeComPort(port_.c_str());
    }

    int Write(const unsigned char* command, unsigned len) {
        return WriteToComPort(port_.c_str(), command, len);
    }

    int Read(unsigned char* answer, unsigned maxLen, unsigned long& bytesRead)
    {
        return ReadFromComPort(port_.c_str(), answer, maxLen, bytesRead);
    }

    static MMThreadLock& GetLock() {
        return lock_;
    }

    void SetFilterWheelState(unsigned state) {
        filterWheelState_ = state;
    }

    unsigned GetFilterWheelState() {
        return filterWheelState_;
    }

private:
    int GetArduinoVersion(int&);
    std::string port_;
    bool initialized_;
    bool portAvailable_;
    int version_;
    static MMThreadLock lock_;
    unsigned int filterWheelState_;
};


#endif //ARDUINOFILTERWHEEL_ARDUINOHUB_H
