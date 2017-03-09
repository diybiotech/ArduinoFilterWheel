 //////////////////////////////////////////////////////////////////////////////
// FILE:          ArduinoFilterWheel.cpp
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

#include "ArduinoFilterWheel.h"
#include "../../MMDevice/ModuleInterface.h"
#include <sstream>
#include <cstdio>

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#define snprintf _snprintf
#endif

const char* g_DeviceNameArduinoFilterWheelHub = "ArduinoFilterWheel-Hub";
const char* g_DeviceNameArduinoFilterWheel = "ArduinoFilterWheel-FilterWheel";
const char* g_DeviceDescriptionArduinoFilterWheel="Arduino Filter Wheel Driver";
const char* g_versionProp = "Version";

const int g_Min_MMVersion = 0;
const int g_Max_MMVersion = 1;

const char* g_On = "On";
const char* g_Off = "Off";

// static lock
MMThreadLock CArduinoFilterWheelHub::lock_;

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData() {
    RegisterDevice(g_DeviceNameArduinoFilterWheelHub, MM::HubDevice, "Hub (required)");
    RegisterDevice(g_DeviceNameArduinoFilterWheel, MM::StateDevice, "Filter Wheel");
}

MODULE_API MM::Device *CreateDevice(const char *deviceName) {
    if (deviceName == 0)
        return 0;

    if (strcmp(deviceName, g_DeviceNameArduinoFilterWheelHub) == 0) {
        return new CArduinoFilterWheelHub;
    } else if (strcmp(deviceName, g_DeviceNameArduinoFilterWheel) == 0) {
        return new CArduinoFilterWheel;
    }

    return 0;
}

MODULE_API void DeleteDevice(MM::Device *pDevice) {
    delete pDevice;
}
///////////////////////////////////////////////////////////////////////////////

CArduinoFilterWheelHub::CArduinoFilterWheelHub() :
        initialized_(false),
        filterWheelState_(0) {
    portAvailable_ = false;

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

    CPropertyAction *pAct = new CPropertyAction(this, &CArduinoFilterWheelHub::OnPort);
    CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);
}

CArduinoFilterWheelHub::~CArduinoFilterWheelHub() {
    Shutdown();
}

void CArduinoFilterWheelHub::GetName(char *name) const {
	CDeviceUtils::CopyLimitedString(name, g_DeviceNameArduinoFilterWheelHub);
}

bool CArduinoFilterWheelHub::Busy() {
    return false;
}

int CArduinoFilterWheelHub::GetControllerVersion(int &version) {
    int ret = DEVICE_OK;
    //unsigned char command[1];
	std::string answer;

    //command[0] = 1;
    version = 0;

	if(!portAvailable_){
		return ERR_NO_PORT_SET;
	}

    //ret = SendSerialCommand(port_.c_str(), (const char *) command, 1);
    //if (ret != DEVICE_OK)
    //    return ret;

    ret = SendSerialCommand(port_.c_str(), "V", "\r");
	if (ret != DEVICE_OK)
	  return ret;

	ret = GetSerialAnswer(port_.c_str(), "\r", answer);
	if (ret != DEVICE_OK) {
	  return ret;
	}

	LogMessage("Found Board", false);
	LogMessage(answer, false);

    if (answer != "ArduinoFilterWheel" && answer != "\nArduinoFilterWheel") {
      return ERR_BOARD_NOT_FOUND;
    }
    
	version = 1; 
    return ret;
}

int CArduinoFilterWheelHub::WriteToComPortH(const char* command, size_t len)
    {
		int ret = DEVICE_OK;
	std::string answer;

	ret = SendSerialCommand(port_.c_str(), (const char *)command, "\r");
	return ret;
    }

bool CArduinoFilterWheelHub::SupportsDeviceDetection(void) {
    return true;
}

MM::DeviceDetectionStatus CArduinoFilterWheelHub::DetectDevice(void) {
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
            GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_BaudRate, "9600");
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
            int ret = GetControllerVersion(v);

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
        LogMessage("Exception in Detect Device!", false);
    }

    return result;
}

int CArduinoFilterWheelHub::Initialize() {
    // Name
   int ret = CreateProperty(MM::g_Keyword_Name, g_DeviceNameArduinoFilterWheelHub, MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   LogMessage("Initializing Filter Wheel", false);

   // The first second or so after opening the serial port, the Arduino is waiting for firmwareupgrades.  Simply sleep 1 second.
   CDeviceUtils::SleepMs(2000);

   MMThreadGuard myLock(lock_);

   // Check that we have a controller:
   PurgeComPort(port_.c_str());

   ret = GetControllerVersion(version_);
   if( DEVICE_OK != ret)
      return ret;

   if (version_ < g_Min_MMVersion || version_ > g_Max_MMVersion)
       return ERR_VERSION_MISMATCH;

   CPropertyAction* pAct = new CPropertyAction(this, &CArduinoFilterWheelHub::OnVersion);
   std::ostringstream sversion;
   sversion << version_;
   CreateProperty(g_versionProp, sversion.str().c_str(), MM::Integer, true, pAct);
   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

   // turn on verbose serial debug messages
   GetCoreCallback()->SetDeviceProperty(port_.c_str(), "Verbose", "1");

   initialized_ = true;
   return DEVICE_OK;
}

int CArduinoFilterWheelHub::DetectInstalledDevices()
{
   if (MM::CanCommunicate == DetectDevice()) 
   {
      std::vector<std::string> peripherals; 

      peripherals.clear();
      peripherals.push_back(g_DeviceNameArduinoFilterWheel);

      for (size_t i=0; i < peripherals.size(); i++) 
      {
         MM::Device* pDev = ::CreateDevice(peripherals[i].c_str());
         if (pDev) 
         {
             AddInstalledDevice(pDev);
         }
      }
   }

   return DEVICE_OK;
}

int CArduinoFilterWheelHub::Shutdown() {
	LogMessage("Shutdown", false);
   initialized_ = false;
   return DEVICE_OK;
}

int CArduinoFilterWheelHub::OnPort(MM::PropertyBase *pProp, MM::ActionType eAct) {
	LogMessage("On Port", false);

    if (eAct == MM::BeforeGet) {
        pProp->Set(port_.c_str());
    } else if (eAct == MM::AfterSet) {
        pProp->Get(port_);
		portAvailable_ = true;
    }

    return DEVICE_OK;
}

int CArduinoFilterWheelHub::OnVersion(MM::PropertyBase* pProp, MM::ActionType pAct)
{
	LogMessage("On Version", false);
   if (pAct == MM::BeforeGet)
   {
      pProp->Set((long)version_);
   }
   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// CArduinoFilterWheel implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~

CArduinoFilterWheel::CArduinoFilterWheel() : 
	busy_(false),
	initialized_(false), 
	name_(g_DeviceNameArduinoFilterWheel),
	changedTime_(0.0),
    position_(6),
	numPos_(7)
{
   InitializeDefaultErrorMessages();
   EnableDelay();

   SetErrorText(ERR_NO_PORT_SET, "Hub Device not found.  The ArduinoFilterWheel Hub device is needed to create this device");

   // Name
   int ret = CreateProperty(MM::g_Keyword_Name, g_DeviceNameArduinoFilterWheel, MM::String, true);
   assert(DEVICE_OK == ret);

   // Description
   ret = CreateProperty(MM::g_Keyword_Description, g_DeviceDescriptionArduinoFilterWheel, MM::String, true);
   assert(DEVICE_OK == ret);

   // parent ID display
   CreateHubIDProperty();
}

CArduinoFilterWheel::~CArduinoFilterWheel() {
	LogMessage("Destructor", false);
    Shutdown();
}

void CArduinoFilterWheel::GetName(char *Name) const {
    CDeviceUtils::CopyLimitedString(Name, g_DeviceNameArduinoFilterWheel);
}

bool CArduinoFilterWheel::Busy() {
	LogMessage("Busy", false);
	return busy_;
    //MM::MMTime interval = GetCurrentMMTime() - changedTime_;
    //MM::MMTime delay(GetDelayMs() * 1000.0);
    //return (interval < delay);
}

int CArduinoFilterWheel::Initialize() {
	LogMessage("Initialize FW", false);
   CArduinoFilterWheelHub* hub = static_cast<CArduinoFilterWheelHub*>(GetParentHub());
   if (!hub || !hub->IsPortAvailable()) {
	  LogMessage("Port failed for Filter Wheel", true);
      return ERR_NO_PORT_SET;
   }
   char hubLabel[MM::MaxStrLength];
   hub->GetLabel(hubLabel);
   SetParentID(hubLabel); // for backward comp.

    // set property list
    // -----------------

    // Set timer for the Busy signal, or we'll get a time-out the first time we check the state of the shutter, for good measure, go back 'delay' time into the past
    //changedTime_ = GetCurrentMMTime();
   LogMessage("Set Positions FW", false);
    // create default positions and labels
    const int bufSize = 64;
    char buf[bufSize];

	snprintf(buf, bufSize, "Cy3");
    SetPositionLabel(1, buf);

	snprintf(buf, bufSize, "TxRed");
    SetPositionLabel(2, buf);

	snprintf(buf, bufSize, "Cy5");
    SetPositionLabel(3, buf);

	snprintf(buf, bufSize, "Mirror");
    SetPositionLabel(4, buf);

	snprintf(buf, bufSize, "Empty");
    SetPositionLabel(5, buf);

	snprintf(buf, bufSize, "Fitc");
    SetPositionLabel(6, buf);

	// add Stop Position
    snprintf(buf, bufSize, "Stop");
    SetPositionLabel(0, buf);

    // add Home Position   **&*&*&&*&*&*  TODO: HOME  position value *^^*^*^*^^
    //snprintf(buf, bufSize, "Home");
    //SetPositionLabel(5, buf);

    //for (unsigned long i = 1; i <= numPos_; i++) {
    //    snprintf(buf, bufSize, "Filter %d", i);
    //    SetPositionLabel(i, buf);
    //}

    // State
    // -----
	CPropertyAction *pAct = new CPropertyAction(this, &CArduinoFilterWheel::OnState);
    int ret = CreateProperty(MM::g_Keyword_State, "State", MM::Integer, false, pAct);
    if (ret != DEVICE_OK)
        return ret;
	SetPropertyLimits(MM::g_Keyword_State, 0, numPos_- 1);

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

int CArduinoFilterWheel::Shutdown() {
    if (initialized_) {
        initialized_ = false;
    }

    return DEVICE_OK;
}

int CArduinoFilterWheel::OnState(MM::PropertyBase *pProp, MM::ActionType eAct) {
	const int bufSize = 128;
    char msg[bufSize];
	long pos;

	CArduinoFilterWheelHub* hub = static_cast<CArduinoFilterWheelHub*>(GetParentHub());
    if (!hub || !hub->IsPortAvailable())
      return ERR_NO_PORT_SET;

    if (eAct == MM::BeforeGet) {
        pProp->Set((long)position_);

        // nothing to do, let the caller to use cached property
    } else if (eAct == MM::AfterSet) {

        // Set timer for the Busy signal
        changedTime_ = GetCurrentMMTime();

        pProp->Get(pos);

        //char* deviceName;
        //GetName(deviceName);

        //if (pos >= numPos_ || pos < 0) {
		//	LogMessage("Invalid position - reverting",false);
        //    pProp->Set(pos); // revert
        //    //return ERR_UNKNOWN_POSITION;
        //}

		snprintf(msg, bufSize, "Moving to %d", pos);
        LogMessage(msg,false);

        const int bufSize = 16;
        char buf[bufSize];
        snprintf(buf, bufSize, "%d", pos);

        //SendSerialCommand(port_.c_str(), buf, "\r");
        position_ = pos;

		hub->SetFilterWheelState(pos);
		if (hub->GetFilterWheelState() >= 0){
			snprintf(msg, bufSize, "New %d", position_);
			LogMessage(msg,false);
			return hub->WriteToComPortH((const char*)buf, bufSize);
		}
		
    }

    return DEVICE_OK;
}

