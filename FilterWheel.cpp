///////////////////////////////////////////////////////////////////////////////
// FILE:          FilterWheel.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   The example implementation of the demo camera.
//                Simulates generic digital camera and associated automated
//                microscope devices and enables testing of the rest of the
//                system without the need to connect to the actual hardware. 
//                
// AUTHOR:        Shirish Goyal <shirish.goyal@gmail.com>, BioCurious, Sunnyvale, CA


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define snprintf _snprintf
#endif

#include "FilterWheel.h"
#include <cstdio>
#include <string>
#include <math.h>
#include "../../MMDevice/ModuleInterface.h"
#include <sstream>

using namespace std;

// External names used by the rest of the system
// to load particular device from DLL

const char *g_DeviceName = "Arduino Filter Wheel";


///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////

MODULE_API void InitializeModuleData() {
    RegisterDevice(g_DeviceName, MM::StateDevice, "Arduino Filter Wheel");
}

MODULE_API MM::Device *CreateDevice(const char *deviceName) {
    if (deviceName == 0)
        return 0;


    if (strcmp(deviceName, g_DeviceName) == 0) {
        return new ArduinoFilterWheel();
    }

    return 0;
}

MODULE_API void DeleteDevice(MM::Device *pDevice) {
    delete pDevice;
}



///////////////////////////////////////////////////////////////////////////////
// Arduino Filter Wheel implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ArduinoFilterWheel::ArduinoFilterWheel() :
        numPos_(6),
        initialized_(false),
        changedTime_(0.0),
        position_(0),
        port_("") {
    InitializeDefaultErrorMessages();

    //Com port
    CPropertyAction *pAct = new CPropertyAction(this, &ArduinoFilterWheel::OnCOMPort);
    CreateProperty(MM::g_Keyword_Port, "", MM::String, false, pAct, true);

    EnableDelay(); // signals that the delay setting will be used
}

ArduinoFilterWheel::~ArduinoFilterWheel() {
    Shutdown();
}

void ArduinoFilterWheel::GetName(char *Name) const {
    CDeviceUtils::CopyLimitedString(Name, g_DeviceName);
}


int ArduinoFilterWheel::Initialize() {
    if (initialized_)
        return DEVICE_OK;

    // set property list
    // -----------------

    // Name
    int ret = CreateProperty(MM::g_Keyword_Name, g_DeviceName, MM::String, true);
    if (DEVICE_OK != ret)
        return ret;

    // Description
    ret = CreateProperty(MM::g_Keyword_Description, "Arduino Filter Wheel Driver", MM::String, true);
    if (DEVICE_OK != ret)
        return ret;

    // Set timer for the Busy signal, or we'll get a time-out the first time we check the state of the shutter, for good measure, go back 'delay' time into the past
    changedTime_ = GetCurrentMMTime();

    // create default positions and labels
    const int bufSize = 64;
    char buf[bufSize];

    // add Stop Position
    snprintf(buf, bufSize, "Stop");
    SetPositionLabel(0, buf);

    // add Home Position   **&*&*&&*&*&*  TODO: HOME  position value *^^*^*^*^^
    snprintf(buf, bufSize, "Home");
    SetPositionLabel(0, buf);

    for (long i = 1; i <= numPos_; i++) {
        snprintf(buf, bufSize, "Filter %ld", i);
        SetPositionLabel(i, buf);
    }

    // State
    // -----
    CPropertyAction *pAct = new CPropertyAction(this, &ArduinoFilterWheel::OnState);
    ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct);
    if (ret != DEVICE_OK)
        return ret;

    // Label
    // -----
    pAct = new CPropertyAction(this, &CStateBase::OnLabel);
    ret = CreateProperty(MM::g_Keyword_Label, "", MM::String, false, pAct);
    if (ret != DEVICE_OK)
        return ret;
    
    ret = UpdateStatus();
    if (ret != DEVICE_OK)
        return ret;

    initialized_ = true;

    return DEVICE_OK;
}

bool ArduinoFilterWheel::Busy() {
    MM::MMTime interval = GetCurrentMMTime() - changedTime_;
    MM::MMTime delay(GetDelayMs() * 1000.0);
    return (interval < delay);
}


int ArduinoFilterWheel::Shutdown() {
    if (initialized_) {
        initialized_ = false;
    }

    return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

int ArduinoFilterWheel::OnState(MM::PropertyBase *pProp, MM::ActionType eAct) {
    if (eAct == MM::BeforeGet) {
        printf("Getting position of Filter Wheel\n");
        pProp->Set(position_);

        // nothing to do, let the caller to use cached property
    } else if (eAct == MM::AfterSet) {

        // Set timer for the Busy signal
        changedTime_ = GetCurrentMMTime();

        long pos;
        pProp->Get(pos);

        //char* deviceName;
        //GetName(deviceName);

        printf("Moving to position %ld\n", position_);

        if (pos >= numPos_ || pos < 0) {
            pProp->Set(position_); // revert
            return ERR_UNKNOWN_POSITION;
        }

        const int bufSize = 20;
        char buf[bufSize];
        snprintf(buf, bufSize, "pos=%ld", pos);

        SendSerialCommand(port_.c_str(), buf, "\r");
        position_ = pos;
    }

    return DEVICE_OK;
}

int ArduinoFilterWheel::OnCOMPort(MM::PropertyBase *pProp, MM::ActionType eAct) {
    if (eAct == MM::BeforeGet) {
        pProp->Set(port_.c_str());
    } else if (eAct == MM::AfterSet) {
        if (initialized_) {
            pProp->Set(port_.c_str());
        }

        pProp->Get(port_);
    }

    return DEVICE_OK;
}

void ArduinoFilterWheel::InitializeFilterWheel() {
    SendSerialCommand(port_.c_str(), "sensors=0", "\r");
    SendSerialCommand(port_.c_str(), "pos?", "\r");

    std::string ans = "";
    GetSerialAnswer(port_.c_str(), "\n", ans);

    int i = 0;
    from_string<int>(i, (ans).c_str(), std::dec);
    position_ = i;

}

