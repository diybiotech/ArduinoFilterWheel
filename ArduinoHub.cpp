//
// Created by Shirish Goyal on 9/14/16.
//

#include "ArduinoHub.h"
#include "../../MMDevice/ModuleInterface.h"
#include "FilterWheel.h"
#include <sstream>
#include <cstdio>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define snprintf _snprintf
#endif

const char *g_DeviceNameArduinoHub = "Arduino-Hub";
const char *g_DeviceNameArduinoFilterWheel = "Arduino-Filter-Wheel";

const int g_Min_MMVersion = 1;
const int g_Max_MMVersion = 2;
const char *g_versionProp = "Version";

const char* g_On = "On";
const char* g_Off = "Off";

// static lock
MMThreadLock ArduinoHub::lock_;

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData() {
    RegisterDevice(g_DeviceNameArduinoHub, MM::HubDevice, "Arduino Hub (required)");
    RegisterDevice(g_DeviceNameArduinoFilterWheel, MM::StateDevice, "Arduino Filter Wheel");
}

MODULE_API MM::Device *CreateDevice(const char *deviceName) {
    if (deviceName == 0)
        return 0;

    if (strcmp(deviceName, g_DeviceNameArduinoHub) == 0) {
        return new ArduinoHub;
    } else if (strcmp(deviceName, g_DeviceNameArduinoFilterWheel) == 0) {
        return new ArduinoFilterWheel;
    }

    return 0;
}

MODULE_API void DeleteDevice(MM::Device *pDevice) {
    delete pDevice;
}
///////////////////////////////////////////////////////////////////////////////

int ArduinoHub::Initialize() {
    return 0;
}

bool ArduinoHub::Busy() {
    return false;
}

int ArduinoHub::Shutdown() {
    return 0;
}

void ArduinoHub::GetName(char *name) const {

}

ArduinoHub::ArduinoHub() :
        initialized_(false),
        filterWheelState_(0) {
    portAvailable_ = false;
//    invertedLogic_ = false;
//    timedOutputActive_ = false;

    InitializeDefaultErrorMessages();

    SetErrorText(ERR_PORT_OPEN_FAILED, "Failed opening Arduino USB device");
    SetErrorText(ERR_BOARD_NOT_FOUND,
                 "Did not find an Arduino board with the correct firmware.  Is the Arduino board connected to this serial port?");
    SetErrorText(ERR_NO_PORT_SET, "Hub Device not found.  The Arduino Hub device is needed to create this device");

    std::ostringstream errorText;
    errorText
            << "The firmware version on the Arduino is not compatible with this adapter.  Please use firmware version ";
    errorText << g_Min_MMVersion << " to " << g_Max_MMVersion;

    SetErrorText(ERR_VERSION_MISMATCH, errorText.str().c_str());

    CPropertyAction *pAct = new CPropertyAction(this, &ArduinoHub::OnPort);
    CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);

//    pAct = new CPropertyAction(this, &ArduinoHub::OnLogic);
//    CreateProperty("Logic", g_invertedLogicString, MM::String, false, pAct, true);
//
//    AddAllowedValue("Logic", g_invertedLogicString);
//    AddAllowedValue("Logic", g_normalLogicString);
}


ArduinoHub::~ArduinoHub() {
    Shutdown();
}

int ArduinoHub::GetArduinoVersion(int &version) {
    int ret = DEVICE_OK;
    unsigned char command[1];
    command[0] = 1;
    version = 0;

    ret = WriteToComPort(port_.c_str(), (const unsigned char *) command, 1);
    if (ret != DEVICE_OK)
        return ret;

    std::string answer;
    ret = GetSerialAnswer(port_.c_str(), "\r\n", answer);
    if (ret != DEVICE_OK)
        return ret;

    if (answer != "Arduino-FW")
        return ERR_BOARD_NOT_FOUND;

    // Check version number of the Arduino
    command[0] = 2;
    ret = WriteToComPort(port_.c_str(), (const unsigned char *) command, 1);
    if (ret != DEVICE_OK)
        return ret;

    std::string ans;
    ret = GetSerialAnswer(port_.c_str(), "\r\n", ans);
    if (ret != DEVICE_OK) {
        return ret;
    }

    std::istringstream is(ans);
    is >> version;

    return ret;
}

bool ArduinoHub::SupportsDeviceDetection(void) {
    return true;
}

MM::DeviceDetectionStatus ArduinoHub::DetectDevice(void) {
    if (initialized_)
        return MM::CanCommunicate;

    MM::DeviceDetectionStatus result = MM::Misconfigured;
    char answerTO[MM::MaxStrLength];

    try {
        std::string portLowerCase = port_;
        for (std::string::iterator its = portLowerCase.begin(); its != portLowerCase.end(); ++its) {
            *its = (char) tolower(*its);
        }
        if (0 < portLowerCase.length() && 0 != portLowerCase.compare("undefined") &&
            0 != portLowerCase.compare("unknown")) {
            result = MM::CanNotCommunicate;

            // record the default answer time out
            GetCoreCallback()->GetDeviceProperty(port_.c_str(), "AnswerTimeout", answerTO);

            // device specific default communication parameters
            // for Arduino Duemilanova
            GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_Handshaking, g_Off);
            GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_BaudRate, "57600");
            GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_StopBits, "1");
            // Arduino timed out in GetArduinoVersion even if AnswerTimeout  = 300 ms
            GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout", "500.0");
            GetCoreCallback()->SetDeviceProperty(port_.c_str(), "DelayBetweenCharsMs", "0");

            MM::Device *pS = GetCoreCallback()->GetDevice(this, port_.c_str());
            pS->Initialize();

            // The first second or so after opening the serial port, the Arduino is waiting for firmware upgrades.
            // Simply sleep 2 seconds.
            CDeviceUtils::SleepMs(2000);

            MMThreadGuard myLock(lock_);
            PurgeComPort(port_.c_str());

            int v = 0;
            int ret = GetArduinoVersion(v);

            // later, Initialize will explicitly check the version #
            if (DEVICE_OK != ret) {
                LogMessageCode(ret, true);
            } else {
                // to succeed must reach here....
                result = MM::CanCommunicate;
            }
            pS->Shutdown();
            // always restore the AnswerTimeout to the default
            GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout", answerTO);
        }
    }
    catch (...) {
        LogMessage("Exception in DetectDevice!", false);
    }

    return result;
}

int ArduinoHub::DetectInstalledDevices() {
    return HubBase::DetectInstalledDevices();
}

