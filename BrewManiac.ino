/**********************************************************************
 BrewManiac 
 created by Vito Tai
 Copyright (C) 2015 Vito Tai
 
 This soft ware is provided as-is. Use at your own risks.
 You are free to modify and distribute this software without removing 
 this statement.
 BrewManiac by Vito Tai is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
***********************************************************************/

#include <EEPROM.h>
#include <PID_v1.h>

#define TESTBED

#define VERSION "1.0"

// *************************
//*  Configuration
// *************************

// *************************
//*  Hardware Configuration
// *************************

//////////////////////////
// supported board
// UNOTEST is UNO, uses I2C LCD
#define UNOTEST 1
// MRE168 is MEGA, HD44780s LCD
#define MRE168 2
// Setting from default Open ArdBir 
#define Pcb_ArdBir_DanielXan 3
// ESP8266
#define ESP_12 4
//////////////////////////

#define BOARD Pcb_ArdBir_DanielXan
// not usingI2C_LCD by default 
#define I2C_LCD true

// modify LCD hardware connection following

//hardware setup
const byte SensorPin=7;
#define PumpControlPin  6
#define BuzzControlPin 8
#define HeatControlPin  9


#if BOARD == MRE168
#define ButtonUpPin    A3
#define ButtonDownPin   A2
#define ButtonStartPin  A1
#define ButtonEnterPin  A0
#endif

#if BOARD == UNOTEST

// overwrite I2C_LCD setting
#define I2C_LCD 	true
#define ButtonUpPin    A2
#define ButtonDownPin   A3
#define ButtonStartPin  A0
#define ButtonEnterPin  A1
#endif


#if BOARD == Pcb_ArdBir_DanielXan
#define ButtonUpPin    A2
#define ButtonDownPin   A3
#define ButtonStartPin  A0
#define ButtonEnterPin  A1
#endif

#if BOARD == ESP_12
#define ButtonUpPin    D2
#define ButtonDownPin   D3
#define ButtonStartPin  D0
#define ButtonEnterPin  D1
#endif


#define BluetoothSupported false
#define UseSoftwareSerial false


#if UseSoftwareSerial == true
const byte SoftwareSerialRx = 10;
const byte SoftwareSerialTx = 11;
#endif

// *************************
//*  software Configuration
// *************************

//debug
//should be false
#define SerialDebug false
#define CHANG_BAUDRATE true
#if UseSoftwareSerial != true
#define SerialDebug false
#endif
// should be false
#define FakeHeating false
#if FakeHeating == true
#define USE_DS18020 false
#else
// must be true
#define USE_DS18020 true
#endif

#define DEBUG 

#define ElectronicOnly true



#define NoPrint false
#define BT_STRICT true
#define BT_TemperatureReportPeriod 10000

#define BT_AutoBaudRate false
#define BT_MODULE_INITIALIZATION false
#define BT_Menu false

#if BT_MODULE_INITIALIZATION != true
#define BT_Menu false
#endif


#define NoDelayStart true
#define SupportAutoModeRecovery true
#define SupportManualModeCountDown true


#define ButtonPressedDetectMinTime      125 // in ms
#define ButtonLongPressedDetectMinTime  1500 // in ms

#define ButtonContinuousPressedDetectMinTime 1000 // in ms
#define ButtonContinuousPressedTrigerTime     150 // in ms
#define ButtonFatFingerTolerance               50 // in ms

#if USE_DS18020
//#define ONEWIRE_SEARCH 0
//#define ONEWIRE_CRC16 0
#include <OneWire.h>
#endif

#if I2C_LCD == true
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
	LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
#else
#include <LiquidCrystal.h>
	LiquidCrystal lcd(A4, A5, 2, 3, 4, 5);
#endif

#if UseSoftwareSerial == true
#include <SoftwareSerial.h>
#endif


#if NoPrint == true
#include "mystrlib.h"
#endif
//{debug
//overrite definition to save some memory
#if BOARD == UNOTEST
#define FakeHeating true
#define USE_DS18020 false

#define SupportAutoModeRecovery true
#define SupportManualModeCountDown false
// 32k flash is limitted
#if BluetoothSupported == true
#define BT_AutoBaudRate true
#define BT_MODULE_INITIALIZATION true
#define BT_Menu true
#else
#define BT_AutoBaudRate false
#define BT_MODULE_INITIALIZATION false
#define BT_Menu false
#endif
#endif

#define SerialDebug true


// Main process stepa
#define C_MASHIN_STAGE  0
#define C_MAINMASH_STAGE  1
#define C_MASHOUT_STAGE  2
#define C_NUMHOPS_STAGE  3
#define C_BOIL_STAGE  4
#define C_HOPS_STAGE  5


//}debug
// *************************
//*  global variables
// *************************
unsigned long gSystemStartTime; // in mili seconds
unsigned long gCurrentTimeInMS; // in mili seconds
unsigned long gCurrentTimeInSec; // in seconds

double gCurrentTemperature;
double gSettingTemperature;
double gBoilStageTemperature;

byte gBoilHeatOutput;

// the pump/heat on/off is requested by user
//  real pump/heat on/off depends on the temperature
//   and parameter setting

boolean gIsHeatOn;
boolean gIsPumpOn;

// *************************
//*  function declaration
// *************************
typedef	void (*SetupFunc)(void);
typedef	void (*EventHandlerFunc)(byte);


void switchApplication(byte screenId);


void backToMain(void);

void setEventMask(byte);

// main screen
void mainSetup(void);
void mainEventHandler(byte);

// setup menu
void menuSetup(void);
void menuEventHandler(byte);

void settingPidSetup(void);
void settingPidEventHandler(byte);

void settingUnitSetup(void);
void settingUnitEventHandler(byte);

void settingAutoSetup(void);
void settingAutoEventHandler(byte);

// manual mode
void manualModeSetup(void);
void manualModeEventHandler(byte);
//auto mode
void autoModeSetup(void);
void autoModeEventHandler(byte);


#if BT_Menu == true
void btMenuSetup(void);
void btMenuEventHandler(byte);
#endif

#if BluetoothSupported == true

//Stage
#define StageMashIn 	0
// 1 -6 rest, 
// 7 mashout
#define StageBoil 		8
//#define StageCooling 		9
//#define StageWhirlpool  10
#define StageDelayStart 11
#define StageManualMode 100
#define StageIdleScreen 101
#define StageSetting 	102

//Event
// Timeup usually means another stage or notification, ignore it

#define RemoteEventTemperatureReached 1
#define RemoteEventAddMalt 			  2
#define RemoteEventRemoveMalt 		 3
#define RemoteEventIodineTest 		4
#define RemoteEventPause 			5
#define RemoteEventResume 			6
#define RemoteEventAddHop 			7
#define RemoteEventPwmOn 			8
#define RemoteEventPwmOff 			9

#define RemoteEventBrewFinished 	99

void btReportCurrentStage(byte stage);
void btReportEvent(byte event);
void btReportSettingTemperature(void);
void btReportPwm(void);

#endif
// *************************
//*  Screens
// *************************

#define ButtonPressedEventMask  0x1
#define TemperatureEventMask 0x2
#define TimeoutEventMask  0x4
#define PumpRestEventMask  0x8


typedef struct _CScreen{
	SetupFunc setup;
	EventHandlerFunc eventHandler;
}CScreen;


#define MAIN_SCREEN 0
#define SETUP_SCREEN 1
#define PID_SETTING_SCREEN 2
#define UNIT_SETTING_SCREEN 3
#define AUTO_SETTING_SCREEN 4

#define MANUAL_MODE_SCREEN 5
#define AUTO_MODE_SCREEN 6

#if BT_Menu == true
#define BT_MENU_SCREEN 7
#endif

const CScreen allScreens[]  =
{{
	&mainSetup,
	&mainEventHandler,
},
{
	&menuSetup,
	&menuEventHandler,
},
{
	&settingPidSetup,
	&settingPidEventHandler,
},
{
	&settingUnitSetup,
	&settingUnitEventHandler,
},
{
	&settingAutoSetup,
	&settingAutoEventHandler,
},
{	
	&manualModeSetup,
	&manualModeEventHandler,
},
{	
	&autoModeSetup,
	&autoModeEventHandler,
},
#if BT_Menu == true
{	
	&btMenuSetup,
	&btMenuEventHandler,
}
#endif
};


byte _currentEventMask;
//#define setEventMask(a) _currentEventMask=(a)

void setEventMask(byte mask)
{
	_currentEventMask=mask;
}


// *************************
//*  includes, follow Arduino conveniention
// *************************

// *************************
//*  EEPROM map
// *************************
#include "string.h"
#include "ui.h"

#include "ps.h"

// *************************
//*  Bluetooth
// *************************
#if BluetoothSupported  == true
#include "bt.h"
#endif


// *************************
//*  Time related function
// *************************

unsigned long _gTimeout;
unsigned long _gAuxTimeout;
boolean  _isAuxTimeout;

#define IsAuxTimeout _isAuxTimeout
unsigned long getTimeLeft(void)
{
	if(_gTimeout ==0) return 0;
	unsigned long ret=(_gTimeout - gCurrentTimeInMS);
	return ret;
}

unsigned long tmGetRemainingTime(void)
{
	if(_gTimeout ==0) return 0;
	//else
	unsigned long ret=(_gTimeout - gCurrentTimeInMS);
	return ret;
}

unsigned long tmPauseTimer(void)
{

  unsigned long ret = 0;

  #ifdef DEBUG
    Serial.println ( "In tmPauseTimer" );
  #endif
  
	if( _gTimeout !=0 ) {
    ret = (_gTimeout - gCurrentTimeInMS);
	}

  _gTimeout=0;
  _gAuxTimeout=0;

  #ifdef DEBUG
    Serial.println ( "Reset _gTimeout and _gAuxTimeout" );
  #endif  
	return ret;
}

void tmSetAuxTimeoutAfter(unsigned long duration)
{
	_gAuxTimeout = gCurrentTimeInMS + duration;
}

void tmSetTimeoutAfter(unsigned long duration)
{
	_gTimeout = gCurrentTimeInMS + duration;
#ifdef DEBUG
	Serial.print(F("setTimeoutAfter:"));
	Serial.print(duration);
	Serial.print(F(",current="));
	Serial.print(gCurrentTimeInMS);
	Serial.print(F(",expires="));
	Serial.println(_gTimeout);
#endif
}

void tmInitialize(void)
{
	gSystemStartTime = millis();
}

boolean tmTiming(void)
{
	gCurrentTimeInMS=millis();


	gCurrentTimeInSec = (gCurrentTimeInMS - gSystemStartTime) / 1000;
	
	if(_gTimeout
	 &&  _gTimeout <= gCurrentTimeInMS)
	{
    #ifdef DEBUG
      Serial.println ( "gTimeout Timeout" );
    #endif
		_gTimeout = 0;
		_isAuxTimeout=false;
		return true;
	}

	if(_gAuxTimeout
	 &&  _gAuxTimeout <= gCurrentTimeInMS)
	{
    #ifdef DEBUG
      Serial.println ( "gAuxTimeout Timeout" );
    #endif
		_gAuxTimeout = 0;
		_isAuxTimeout=true;
		return true;
	}
	
	return false;
}

// *************************
//*  button related function
// *************************

#define ButtonUpMask    0x01
#define ButtonDownMask  0x02
#define ButtonStartMask 0x04
#define ButtonEnterMask 0x08

unsigned char _testButtonStatus;
unsigned long _buttonChangeTime;
unsigned long _continuousPressedDectedTime;
boolean _continuousPressedDetected;
boolean gLongPressed;
byte gButtonPressed=0;

unsigned long _oneFigerUp;

void btnInitialize(void)
{
  	pinMode (ButtonUpPin,    INPUT_PULLUP);
  	pinMode (ButtonDownPin,  INPUT_PULLUP);
  	pinMode (ButtonStartPin, INPUT_PULLUP);
  	pinMode (ButtonEnterPin, INPUT_PULLUP);
  	gButtonPressed=0;
  	_testButtonStatus=0;
}


#define btnIsUpPressed (gButtonPressed == ButtonUpMask)
#define btnIsDownPressed (gButtonPressed == ButtonDownMask)
#define btnIsEnterPressed (gButtonPressed == ButtonEnterMask)
#define btnIsStartPressed (gButtonPressed == ButtonStartMask)

#define btnIsEnterLongPressed ((gButtonPressed == ButtonEnterMask) && gLongPressed)

#define btnIsUpContinuousPressed (gButtonPressed == (ButtonUpMask<<4))
#define btnIsDownContinuousPressed (gButtonPressed == (ButtonDownMask<<4))

#define isExactButtonsPressed(mask) ((mask) == gButtonPressed)
#define isButtonsPressed(mask) ((mask) & gButtonPressed)
#define PRESSED 0
#if SerialDebug != true
#define BUTTON_DEBUG false
#endif

#define BUTTON_DEBUG false

#if BluetoothSupported == true

boolean _virtualButtonPressed=false;

void virtualButtonPress(byte mask,boolean longPressed)
{
       gButtonPressed = mask;
       gLongPressed = longPressed;
       _virtualButtonPressed=true;
}

#endif



boolean btnReadButtons(void)
{
#if BluetoothSupported == true
      if(_virtualButtonPressed)
      {
      	_virtualButtonPressed = false;
        return true;
       }
#endif

  	unsigned char buttons=0;

  	if (digitalRead(ButtonUpPin) == PRESSED)
  	{
  		buttons |= ButtonUpMask;
  	}
  	if (digitalRead(ButtonDownPin) == PRESSED)
  	{
  		buttons |= ButtonDownMask;
  	}
  	if (digitalRead(ButtonEnterPin) == PRESSED)
  	{
  		buttons |= ButtonEnterMask;
  	}
  	if (digitalRead(ButtonStartPin) == PRESSED)
  	{
  		buttons |= ButtonStartMask;
  	}

	if(buttons==0)
	{
		if(_testButtonStatus == 0) return false;
		
		unsigned long duration=gCurrentTimeInMS - _buttonChangeTime;
 
  		#if BUTTON_DEBUG == true
    	Serial.print(F("pressed:"));
    	Serial.print(_testButtonStatus);
    	Serial.print(F(","));
    	Serial.print(buttons);
    	Serial.print(F("for:"));
    	Serial.println(duration);
   		#endif
		
		if(duration > ButtonPressedDetectMinTime)
		{
			if(duration > ButtonLongPressedDetectMinTime) gLongPressed=true;
			else gLongPressed =false;
			gButtonPressed = _testButtonStatus;
			
			_testButtonStatus = 0;
			_continuousPressedDetected = false;
  		
  		#if BUTTON_DEBUG == true
  		Serial.print(gButtonPressed);
  		if (gLongPressed) Serial.println(F(" -Long Pressed"));
    	else Serial.println(F(" -Pressed"));
   		#endif


			return true; 
		}
		
  		#if BUTTON_DEBUG == true
    	Serial.println(F("Not Pressed"));
   		#endif

		_testButtonStatus =0;
		_continuousPressedDetected = false;
		
		return false; 
	}
	
	// buttons is not ZERO afterward
	if(buttons == _testButtonStatus) // pressing persists
	{
		if(_continuousPressedDetected )
		{
			//if duration exceeds a trigger point
			if( (gCurrentTimeInMS - _continuousPressedDectedTime) > ButtonContinuousPressedTrigerTime)
			{
				_continuousPressedDectedTime=gCurrentTimeInMS;

			  	#if BUTTON_DEBUG == true
			  	Serial.print(gButtonPressed);
    			Serial.print(F(" -Continues 2 pressed:"));
    			Serial.println(gCurrentTimeInMS);
   				#endif

				return true;
			}
		}
		else
		{
			unsigned long duration=gCurrentTimeInMS - _buttonChangeTime;
			if(duration > ButtonContinuousPressedDetectMinTime)
			{
				_continuousPressedDetected=true;
				_continuousPressedDectedTime=gCurrentTimeInMS;
				// fir the first event
				gButtonPressed = buttons << 4; // user upper 4bits for long pressed

			  	#if BUTTON_DEBUG == true
			  	Serial.print(gButtonPressed);
    			Serial.print(F(" -Continues detected pressed:"));
    			Serial.println(gCurrentTimeInMS);
   				#endif

				return true;
			}
		}
	}
	else // if(buttons == _testButtonStatus)
	{
		// for TWO buttons event, it is very hard to press and depress
		// two buttons at exactly the same time.
		// so if new status is contains in OLD status.
		// it might be the short period when two fingers are leaving, but one is detected
		// first before the other
		// the case might be like  01/10 -> 11 -> 01/10
		//  just handle the depressing case: 11-> 01/10
		if((_testButtonStatus & buttons)
			&&  (_testButtonStatus > buttons))
		{
			if(_oneFigerUp ==0)
			{
				_oneFigerUp = gCurrentTimeInMS;
				// skip this time
				return false;
			}
			else
			{
				// one fat finger is dected
				if( (gCurrentTimeInMS -_oneFigerUp) < ButtonFatFingerTolerance) 
				{
					return false;
				}
			}

		#if BUTTON_DEBUG == true
    	Serial.println(F("Failed fatfinger"));
   		#endif

		}
		// first detect, note time to check if presist for a duration.
		_testButtonStatus = buttons;
		_buttonChangeTime = gCurrentTimeInMS;
		_oneFigerUp = 0;
		
		#if BUTTON_DEBUG == true
    	Serial.println(F("Attempted"));
   		#endif

	}
	
	return false;
}

// *************************
//*  tempture related function
// ****************************
// temperature event


#if USE_DS18020 == true

OneWire ds(SensorPin);
boolean _isConverting=false;

#define DSCMD_CONVERT_T 0x44
#define DSCMD_READ_SCRATCHPAD 0xBE 
#define DSCMD_WRITE_SCRATCHPAD 0x4E
#define DSCMD_COPY_SCRATCHPAD 0x48
#define DS_DEG_RESOLUTION 0.0625

#define DS_RES_BITS 10

#if DS_RES_BITS == 12
// 12 bit resolution - 0.0625 degC resolution, 750ms conversion time
#define DS_MASK_RESOLUTION 0xFFFC  // 0.25 degC resolution
#define DS_RESOLUTION 0x7F
#endif
#if DS_RES_BITS == 11
// 11 bit resolution - 0.125 degC resolution, 375ms conversion time
#define DS_MASK_RESOLUTION 0xFFFC // 0.25 degC resolution
#define DS_RESOLUTION 0x5F
#endif
#if DS_RES_BITS == 10
// 10 bit resolution - 0.25 degC resolution, 187.5ms conversion time
#define DS_MASK_RESOLUTION 0xFFFC // 0.25 degC resolution
#define DS_RESOLUTION 0x3F
#endif

byte _sensorData[9];

void dsInizialize(void) 
{
  ds.reset();
  ds.skip();
}
#endif

void tpInitialize(void)
{
#if USE_DS18020
	// Change DS18B20 resolution
  dsInizialize();
  ds.write(DSCMD_WRITE_SCRATCHPAD);   // Write scratchpad
  ds.write(0);            // TL
  ds.write(0);            // TH
  ds.write(DS_RESOLUTION);   // Configuration Register

	dsInizialize();  	
 	ds.write(DSCMD_COPY_SCRATCHPAD);         // Copy Scratchpad to DS EEPROM
	
#endif

	gCurrentTemperature = 19.99;
	gBoilStageTemperature=readSetting(PS_BoilTemp);
	
}

// the following code basically comes from Open ArdBir

void tpReadTemperature(void)
{
#if USE_DS18020 == true

	dsInizialize();
  	if (_isConverting == false) 
  	{
	  	// start conversion and return
    	ds.write(DSCMD_CONVERT_T, 0);
    	_isConverting = true;
     
    	return;
  	}
  	// else if convert start 
  	//if (_isConverting) 
  	//{ 

  	// check for conversion if it isn't complete return if it is then convert to decimal

    if (ds.read_bit() == 0) 
    	return;

    // reset & "select" again
    dsInizialize();
    
    // request data
    ds.write(DSCMD_READ_SCRATCHPAD);  

    for ( byte i = 0; i < 9; i++) {           // with crc we need 9 bytes
      		_sensorData[i] = ds.read();
    }

    	/* add this routine for crc version */
    if ( OneWire::crc8(_sensorData, 8) != _sensorData[8]) {  
      //if checksum fails start a new conversion right away
      // reissue convert command
    	dsInizialize();      
      ds.write(DSCMD_CONVERT_T, 0);
      _isConverting = true;
    	return;
    }

    // temp data verified, calculate temperature value
    unsigned int raw = (_sensorData[1] << 8) + _sensorData[0];
    gCurrentTemperature = (raw & DS_MASK_RESOLUTION) * DS_DEG_RESOLUTION;

    //apply calibration 
    //gCurrentTemperature += ((float)(readSetting(PS_Offset) - 50) / 10.0);
    _isConverting = false;
  	//} 
#endif
}

void pumpLoadParameters(void);
void heatLoadParameters(void);

void loadBrewParameters(void)
{
	heatLoadParameters();
	pumpLoadParameters();
}
// *************************
//*  heating related function
// *************************

#if ElectronicOnly != true
boolean _isUseGas;
#endif

boolean _physicalHeattingOn;
byte _heatWindowSize;
unsigned long _windowStartTime;

//double pidInput;
//double pidSetpoint;
//pidInput=gCurrentTemperature;
//pidSetpoint=gSettingTemperature;

#define pidInput gCurrentTemperature
#define pidSetpoint gSettingTemperature

double pidOutput;

PID thePID(&pidInput,&pidOutput,&pidSetpoint,100,40,0,DIRECT);

void heatInitialize(void)
{
	thePID.SetMode(AUTOMATIC);
	
	_physicalHeattingOn=false;
	gIsHeatOn=false;
}
// the should be call before REAL action instead of system startup
void heatLoadParameters(void)
{
	thePID.SetTunings((double)readSetting(PS_kP)-100.0, 
					  (double)((readSetting(PS_kI)-100.0) / 250.0),
					  (double)readSetting(PS_kD)-100.0);
					
	_heatWindowSize = readSetting(PS_WindowSize);    
 	thePID.SetSampleTime((int)readSetting(PS_SampleTime) * 250);

 	gBoilStageTemperature=(float)readSetting(PS_BoilTemp);
 	gBoilHeatOutput=readSetting(PS_BoilHeat);

#if 0 // SerialDebug == true
	
	Serial.print("gBoilStageTemperature=");
	Serial.println(gBoilStageTemperature);

#endif

}
#if FakeHeating == true
unsigned long lastTime;
#endif

void heatPhysicalOn(void)
{
	if(!_physicalHeattingOn)
	{
		digitalWrite (HeatControlPin, HIGH);
		_physicalHeattingOn=true;
#if FakeHeating == true
	lastTime = gCurrentTimeInMS;
#endif
	}
}

void heatPhysicalOff(void)
{
	if(_physicalHeattingOn)
	{
		digitalWrite (HeatControlPin, LOW);
		_physicalHeattingOn=false;
	}
}


void heatOff(void)
{
	gIsHeatOn = false;
	uiHeatingStatus(HeatingStatus_Off);
	heatPhysicalOff();
}

boolean _isPIDMode;

void heatOn(void)
{
	gIsHeatOn = true;
	
	_isPIDMode =true;
	
	// should run through heating algorithm first
	// so that the correct symbol can be shown
	_windowStartTime=millis();
//	uiHeatingStatus(HeatingStatus_On_PROGRAM_OFF);
	heatPhysicalOn();
	uiHeatingStatus(HeatingStatus_On);
}

void heatProgramOff(void)
{
	heatOff();
	uiHeatingStatus(HeatingStatus_On_PROGRAM_OFF);
}

float round025(float num)
{
	int intPart=(int)num;
	return (float)intPart + ((int)((num - (float)intPart)*100.0)/25)*0.25;
}

void heatThread(boolean inBoil)
{
	if(! gIsHeatOn) return;
	
#if FakeHeating == true
	if(_physicalHeattingOn){
		gCurrentTemperature += (gCurrentTimeInMS - lastTime) * 0.0005;
		lastTime = gCurrentTimeInMS;
	}
#endif	
	//[TODO:] remove redaundancy?
	//pidInput=gCurrentTemperature;
	//pidSetpoint=gSettingTemperature;
	
	// only when heat is on, 
	//  do the code execute to determine
	//  if physical heat should be ON or OFF
	
	// the following code comes directly from Open ArdBir
  	
  	float DeltaPID;
  	
  	float Rapporto, Delta, IsteresiProporzionale;


    DeltaPID = 3.50;
    if (pidInput >= gBoilStageTemperature - DeltaPID)  {
      DeltaPID = 1;
    }
    IsteresiProporzionale = 0.0;

  	Delta    = pidSetpoint - (pidInput + IsteresiProporzionale);
  	Rapporto = Delta / DeltaPID;
  
  	unsigned long now = millis();
  
  	if (_isPIDMode) //PID mode
  	{
    	if (Rapporto < 1.00)
    	{
        #ifdef DEBUG
          Serial.println ("Calling PID compute");
        #endif
     		thePID.Compute();   // was 6, getting close, start feeding the PID -mdw
    	} 
    	else // if (Rapporto < 1.00)
    	{
        #ifdef DEBUG
          Serial.println ("Bypassing PID compute, full power");
        #endif
        pidOutput = 255;      // was 5, ignore PID and go full speed -mdw  // set the output to full on
    	}  
  	}// end of _isPIDMode

  	// In boiling stage, the output value is reassigned.
  	
	if (inBoil &&
	  pidInput >= pidSetpoint && pidSetpoint >= gBoilStageTemperature) {
		pidOutput = gBoilHeatOutput * 255.0 / 100.0;
    #ifdef DEBUG
      Serial.println ( "Overridden PID in boil for PWM");
    #endif
	}
  
#ifdef DEBUG //  SerialDebug == true
    	Serial.print("PID.Compute: Input=");
      Serial.print(pidInput);
      Serial.print(", setpoint=");
      Serial.print(pidSetpoint);
      Serial.print(",boil stage temp=");
      Serial.print(gBoilStageTemperature);
      Serial.print(",pid output=");
      Serial.println(pidOutput);
#endif

	// PWM
  	if (now - _windowStartTime > (unsigned int) _heatWindowSize * 250) 
  	{
    	_windowStartTime += (unsigned int)_heatWindowSize * 250; 
    	//time to shift the Relay Window
  	}
  	
#if 0 // SerialDebug == true
	Serial.print("pidOutput=");
	Serial.println(pidOutput);
#endif 
  	
  	if ((pidOutput / 255) * ((unsigned int)_heatWindowSize * 250) > now - _windowStartTime) 
  	{
  		if(!_physicalHeattingOn)
  		{
  			uiHeatingStatus(HeatingStatus_On);
   	 		heatPhysicalOn();
   	 	}
  	}
  	else 
  	{
  		// turn off heat 
  		if(_physicalHeattingOn)
  		{
  			uiHeatingStatus(HeatingStatus_On_PROGRAM_OFF);
  			heatPhysicalOff();
  		}
  	}
} // end of heatThread

// *************************
//*  pump related function
// *************************

boolean _pumpPhysicalOn;
unsigned long _pumpLastSwitchOnTime;
float _pumpStopTemp;
byte _sensorType;

unsigned long _pumpRestTime;
unsigned long _pumpCycleTime;
boolean _isStageTempReached;
boolean _isPumpRestChanged;
boolean _pumpRestEnabled;

#define SensorInside 0
#define SensorOutside 1

void pumpPhysicalOn(void)
{
	if(_pumpPhysicalOn) return;
	digitalWrite (PumpControlPin, HIGH);
	_pumpPhysicalOn=true;
	_pumpLastSwitchOnTime = gCurrentTimeInMS;
}

void pumpPhysicalOff(void)
{
	if(!_pumpPhysicalOn) return;
	digitalWrite (PumpControlPin, LOW);
	_pumpPhysicalOn=false;
}

void pumpInitialize(void)
{
	_pumpPhysicalOn=false;
	gIsPumpOn=false;
	_isPumpRestChanged = false;
	_pumpRestEnabled=false;
}

void pumpLoadParameters(void)
{
	_pumpStopTemp = (float) readSetting(PS_TempPumpRest);
	_sensorType = readSetting(PS_SensorType);
	_pumpRestTime =(unsigned long) readSetting(PS_PumpRest) *60 *1000;
	_pumpCycleTime=(unsigned long) readSetting(PS_PumpCycle) *60 *1000;
	_isPumpRestChanged = false;
#if 0	
	Serial.print("pumpLoadParameters,cycletime=");
	Serial.print(_pumpCycleTime);
	Serial.print("resttime=");
	Serial.println(_pumpRestTime);
#endif
}

void pumpOff(void)
{
	if(!gIsPumpOn) return;
	gIsPumpOn=false;
	uiPumpStatus(PumpStatus_Off);
	pumpPhysicalOff();
}

void pumpOn(void)
{
	if(gIsPumpOn) return;
	gIsPumpOn=true;
	uiPumpStatus(PumpStatus_On);
	
	pumpPhysicalOn();
}

void pumpRestSetEnabled(boolean enable)
{
	_pumpRestEnabled=enable;
	
	if(_pumpRestEnabled && _pumpPhysicalOn) // restart counting time
		_pumpLastSwitchOnTime = gCurrentTimeInMS;
		
	if(!_pumpRestEnabled && gIsPumpOn)
	{
        if(!_pumpPhysicalOn)
        {
			pumpPhysicalOn();
			uiPumpStatus(PumpStatus_On);
		}
	}
	
//	if (_pumpRestEnabled) Serial.println(F("Enable pump rest"));
//	else Serial.println(F("disable pump rest"));
}

void pumpControl(boolean tempReached)
{
	_isStageTempReached =tempReached;
}

boolean isPumpRest(void)
{
	return ! _pumpPhysicalOn;
}

boolean pumpRestEvent(void)
{
	if(_isPumpRestChanged)
	{
		_isPumpRestChanged = false;
		return true;
	}
	return false;
}

void pumpThread(void)
{
	if(!gIsPumpOn) return;
	// temperature control
	
	if(!_pumpRestEnabled) return;
	
  	float deltaTemp;
  
  	if (_sensorType == SensorInside) deltaTemp = (float)_pumpRestTime * gSettingTemperature / 70;
  	else deltaTemp = (float)_pumpRestTime * gSettingTemperature / 35;
	
	if(gCurrentTemperature >= _pumpStopTemp)
	{
    	if (_sensorType == SensorInside) 
    	{ 
      		 // inside
      		 if(_pumpPhysicalOn)
      		 {
				      pumpPhysicalOff();
				    uiPumpStatus(PumpStatus_On_PROGRAM_OFF);
        		_isPumpRestChanged = true;

			    }
			    return;
    	} 
    	else 
    	{
    		// sensor outside
      		if (_isStageTempReached)
      		{
        		if (gCurrentTemperature < (gBoilStageTemperature - (deltaTemp * 2))) 
        		{
        			if(!_pumpPhysicalOn)
        			{
						    pumpPhysicalOn();
						    uiPumpStatus(PumpStatus_On);
        				_isPumpRestChanged = true;
					    }
        		} 
        		else 
        		{ 
        			if(_pumpPhysicalOn)
        			{
					    	pumpPhysicalOff();
					    	uiPumpStatus(PumpStatus_On_PROGRAM_OFF);
        				_isPumpRestChanged = true;
					    }
        		}
      		} 
      		else // not tempreached
      		{
      			if(!_pumpPhysicalOn)
      			{
					pumpPhysicalOn();
					uiPumpStatus(PumpStatus_On);
        			_isPumpRestChanged = true;
				}
      		}
    	}
  	} 
  	else // of if(gCurrentTemperature >= _pumpStopTemp)
  	{
  		// if under pump stop temperature
      	if((gCurrentTimeInMS - _pumpLastSwitchOnTime) < (unsigned long)_pumpCycleTime) 
      	{
      		if(!_pumpPhysicalOn)
      		{
      			pumpPhysicalOn();
      			uiPumpStatus(PumpStatus_On);
        		_isPumpRestChanged = true;
        	}
      	} 
      	else 
      	{
      		// pump rest state, heat will be off!
      		if(_pumpPhysicalOn)
      		{
      			pumpPhysicalOff();
      			uiPumpStatus(PumpStatus_On_PROGRAM_OFF);
        		_isPumpRestChanged = true;
        	}
        	
        	if ((gCurrentTimeInMS - _pumpLastSwitchOnTime) >= 
        			(_pumpCycleTime + _pumpRestTime) 
        		|| ((gSettingTemperature - gCurrentTemperature) > deltaTemp))
        	{ 
        		_pumpLastSwitchOnTime = gCurrentTimeInMS;
        		//Serial.println("on time start");
        	}
      	}	 
    } // end of else //if if(gCurrentTemperature >= _pumpStopTemp)
	
}//pumpThread()

// *************************
//*  buzzer related function
// *************************

// move to buzz.h
#include "buzz.h"

// *************************
//*  Event handling function
// *************************

// *************************
//*  Applications
//*  application should have two
//*  functions, setup & eventHandler
// *************************

//**************************************************************
//* For setting editing
//**************************************************************
// I hope I could do it in C++ objective way.
// maybe someday

//typedef void (*FuncValueDisplay)(int);

int _editingValue;
int  _maxValue;
int  _minValue;
void (*_displayFunc)(int);

int editItemValue(void)
{	
	return _editingValue;
}
void editItem(str_t label, int value, int max, int min,void (*displayFunc)(int))
{
	_editingValue=value;
	_maxValue=max;
	_minValue=min;
	_displayFunc=displayFunc;
	
	// uiClearSettingRow();
	// uiSettingTitle will know the length of
	// label, and fill spaces to clear the line
	uiSettingTitle(label);
	(*_displayFunc)(_editingValue);
}

void editItemTitleAppendNumber(byte num) // specially for HOP#
{
	uiSettingTitleAppendNumber(num);
}

void editItemChange(int change)
{
	int potential=_editingValue+change;

	if(potential > _maxValue) potential= _maxValue;
	if(potential < _minValue) potential= _minValue;
	if(potential!=_editingValue)
	{
		_editingValue =potential;	
		uiSettingFieldClear();	
		(*_displayFunc)(_editingValue);
	}	
}


// *************************
//*  PID & PWD settings
// *************************

#if ElectronicOnly != true	
void displayHeatingSource(int data)
{
  	if(data) uiSettingDisplayText(STR(Gas));
  	else uiSettingDisplayText(STR(Electric));
}
#endif

void displayOffset100(int data)
{
	float fvalue=(float)data -100.0;
	uiSettingDisplayNumber(fvalue,0);
}

void displayMultiply250(int data)
{
	float fvalue=(float)data *250.0;
	uiSettingDisplayNumber(fvalue,0);
}

void displaySimpleInteger(int data)
{
	uiSettingDisplayNumber((float)data,0);
}

void displayTempShift50Divide10(int data)
{
	float fvalue=((float)data -50.0) /10.0;
	
	uiSettingShowTemperature(fvalue,1);
}

#if ElectronicOnly != true	
void displayDivide10(int data)
{
	float fvalue=(float)data /10.0;
	uiSettingDisplayNumber(fvalue,1);
}
#endif

void displayPercentage(int data)
{
	uiDisplaySettingPercentage(data);
}

//**************************************************************
//* PID PWM setting screen
//**************************************************************
byte _currentPidSetting;

// table implementation takes up a lot of memory.
// change to hard-coded.
#define PID_SETTING_NUM 8
void settingPidEditSetting(void)
{	
	int value=(int)readSetting(PS_AddrOfPidSetting(_currentPidSetting));
	
	//editItem(str_t label, byte value, byte max, byte min,CDisplayFunc displayFunc)
	if(_currentPidSetting==1)
		editItem(STR(kP),value,200,0,& displayOffset100);
	else if(_currentPidSetting==2)
		editItem(STR(kI),value, 255,0,& displayOffset100);
	else if(_currentPidSetting==3)
		editItem(STR(kD),value,200,0,& displayOffset100);
	else if(_currentPidSetting==4)
		editItem(STR(SampleTime),value,3500/250,1500/250,& displayMultiply250);
	else if(_currentPidSetting==5)
		editItem(STR(WindowSet_ms),value,7500/250,4000/250,& displayMultiply250);
	else if(_currentPidSetting==6)
		editItem(STR(Heat_in_Boil),value,100,0,& displayPercentage);
//	else if(_currentPidSetting==7)
//		editItem(STR(Calibration),value,100,0,&displayTempShift50Divide10);

}

void settingPidSetup(void)
{
	uiMenu(STR(Up_Down_x_Ok));
#if ElectronicOnly != true		
	_currentPidSetting=0;
#else
	_currentPidSetting=1;
#endif

	settingPidEditSetting();
}

void settingPidEventHandler(byte)
{
	if(btnIsEnterPressed)
	{
		byte value=(byte)editItemValue();
		updateSetting(PS_AddrOfPidSetting(_currentPidSetting),value);
		
		//go to next setting
		_currentPidSetting ++;		

		{
			// use electric, // unavailable setting: last one
			if(_currentPidSetting == (PID_SETTING_NUM -1)) 
			{
				// last item is unavailable for electric setting
				uiClearSettingRow();
				switchApplication(SETUP_SCREEN);
				return;
			}
		}
		// not last item, (if it is last item, it won't run through here)
		settingPidEditSetting();
	}
	else if(btnIsUpPressed)
	{
		editItemChange(+1);
	}
	else if(btnIsDownPressed)
	{
		editItemChange(-1);
	}
	else if(btnIsUpContinuousPressed)
	{
		editItemChange(+4);
	}
	else if(btnIsDownContinuousPressed)
	{
		editItemChange(-4);
	}

}

// *************************
//*  Unit Parameters settings
// *************************
byte _currentUnitSetting;

void displayDegreeSymbol(int value)
{
	uiSettingDegreeSymbol(value);
	//if (value==0) uiSettingDisplayText(STR(Celius));
	//else uiSettingDisplayText(STR(Fahrenheit));
}

void displayInsideOutside(int value)
{
	if (value==0) uiSettingDisplayText(STR(Inside));
	else uiSettingDisplayText(STR(Outside));
}

void displayOnOff(int value)
{
	if (value==0) uiSettingDisplayText(STR(Off));
	else uiSettingDisplayText(STR(On));
}

void displayYesNo(int value)
{
	if (value==0) uiSettingDisplayText(STR(No));
	else uiSettingDisplayText(STR(Yes));
}

void displayTemperature(int value)
{
	uiSettingDisplayNumber((float)value,0);
}

void displayActivePassive(int value)
{
	if (value==0) uiSettingDisplayText(STR(Passive));
	else uiSettingDisplayText(STR(Active));
}

void displayTime(int value)
{
	uiSettingTimeInMinutes((byte)value);
}

void displayTimeOff(int value)
{
	if(value==0)
	{
		uiSettingDisplayText(STR(Off));
	} 
	else
	{
		displayTime(value);
	}
}

void displaySimpleTemperature(int value)
{
	uiSettingShowTemperature((float)value,0);
}


void settingUnitDisplayItem(void)
{
	int value=(int)readSetting(PS_AddrOfUnitSetting(_currentUnitSetting));
	
	if(_currentUnitSetting==PS_SettingIndex(PS_SensorType))
		editItem(STR(Sensor),value,1,0,&displayInsideOutside);

	else if(_currentUnitSetting==PS_SettingIndex(PS_BoilTemp))
#ifdef TESTBED  
    editItem(STR(Temp_Boil),value,30,20,&displaySimpleTemperature);    
#else    
		editItem(STR(Temp_Boil),value,105,90,&displaySimpleTemperature);
#endif  

	else if(_currentUnitSetting==PS_SettingIndex(PS_PumpCycle))
		editItem(STR(Pump_Cycle),value,15,5,&displayTime);

	else if(_currentUnitSetting==PS_SettingIndex(PS_PumpRest))
		editItem(STR(Pump_Rest),value,5,0,&displayTime);

	else if(_currentUnitSetting==PS_SettingIndex(PS_PumpPreMash))
		editItem(STR(Pump_PreMash),value,1,0,&displayOnOff);

	else if(_currentUnitSetting==PS_SettingIndex(PS_PumpOnMash))
		editItem(STR(Pump_On_Mash),value,1,0,&displayOnOff);

	else if(_currentUnitSetting==PS_SettingIndex(PS_PumpOnMashOut))
		editItem(STR(Pump_Mashout),value,1,0,&displayOnOff);

	else if(_currentUnitSetting==PS_SettingIndex(PS_PumpOnBoil))
		editItem(STR(Pump_On_Boil),value,1,0,&displayOnOff);

	else if(_currentUnitSetting==PS_SettingIndex(PS_TempPumpRest))
		editItem(STR(Pump_Stop),value,105,80,&displaySimpleTemperature);

	else if(_currentUnitSetting==PS_SettingIndex(PS_PidPipe))
		editItem(STR(PID_Pipe),value,1,0,&displayActivePassive);

	else if(_currentUnitSetting==PS_SettingIndex(PS_SkipAddMalt))
		editItem(STR(Skip_Add),value,1,0,&displayYesNo);

	else if(_currentUnitSetting==PS_SettingIndex(PS_SkipRemoveMalt))
		editItem(STR(Skip_Remove),value,1,0,&displayYesNo);

}

// Initialization of the screen
void settingUnitSetup(void)
{
	uiMenu(STR(Up_Down_x_Ok));
	_currentUnitSetting=PS_SettingIndex(PS_SensorType);
	settingUnitDisplayItem();
}

void settingUnitEventHandler(byte)
{
	if(btnIsEnterPressed)
	{
		byte value=(byte)editItemValue();
		updateSetting(PS_AddrOfUnitSetting(_currentUnitSetting),value);
		
		//goto next item
		_currentUnitSetting++;
		
		if(_currentUnitSetting > PS_SettingIndex(PS_SkipRemoveMalt) )
		{
      // All unit params entered, return to main menu
			uiClearSettingRow();
			switchApplication(SETUP_SCREEN);
			return;
		}
		settingUnitDisplayItem();
	}
	else if(btnIsUpPressed)
	{
		editItemChange(+1);
	}
	else if(btnIsDownPressed)
	{
		editItemChange(-1);
	}
	else if(btnIsUpContinuousPressed)
	{
		editItemChange(+4);
	}
	else if(btnIsDownContinuousPressed)
	{
		editItemChange(-4);
	}
}

// *************************
//*  Automation settings
// *************************


void displayStageTemperature(int value)
{
	
	float temperature=TempFromStorage(value);
	uiSettingShowTemperature(temperature,2);
}

byte _editingStage;
// useing stage to input number of hops & boild time
byte _editingStageAux;
// for Mash stage
//		_editingStageAux ==0 means temp
// 		_editingStageAux == 1 means time
// for hop time
//    it is number of hop
//    

int _maxHopTime; // to make sure hop time is in order
				  // will be set at BOIL time setting
				  // and every hoptime
byte _hopNumber;
#define MAX_STAGE_TIME 140
#define MIN_STAGE_TIME 1

void settingAutomationDisplayItem(void)
{
	int value;
	
	if(_editingStage <= C_MASHOUT_STAGE ) // from MashIn,Main Mash,MashOut
	{
		if(_editingStageAux==0)
			value = readSettingWord(PS_StageTemperatureAddr(_editingStage));
		else
		{
			value =readSetting(PS_StageTimeAddr(_editingStage));
			if (value==0) value=1;
		}
	}
	// else read value later
		//8. number of hops
		//9. boil time
		//10. time hop number #
	
	if( _editingStageAux == 0 )  // temperature editing
		uiMenu(STR(Up_Down_x_Ok));
			
	if(_editingStage == C_MASHIN_STAGE)
	{
		// Mash In:temp only
		editItem(STR(Mash_In),value,ToTempInStorage(75),ToTempInStorage(20),&displayStageTemperature);
	}
	else if(_editingStage == C_MAINMASH_STAGE )
	{
		if (_editingStageAux == 0)
#ifdef TESTBED    
      editItem(STR(Mash),value,ToTempInStorage(50),ToTempInStorage(25),&displayStageTemperature);     
#else      
			editItem(STR(Mash),value,ToTempInStorage(70),ToTempInStorage(60),&displayStageTemperature);
#endif
		else
#ifdef TESTBED       
      editItem(STR(Mash),value,50,MIN_STAGE_TIME,&displayTime);
#else   
      editItem(STR(Mash),value,MAX_STAGE_TIME,MIN_STAGE_TIME,&displayTime);   
#endif
	}
/* 
	else if(_editingStage ==2)
	{
		if (_editingStageAux == 0)
			editItem(STR(Glucanase),value,ToTempInStorage(50),ToTempInStorage(35),&displayStageTemperature);
		else
			editItem(STR(Glucanase),value,MAX_STAGE_TIME,MIN_STAGE_TIME,&displayTime);	
	}
	else if(_editingStage ==3)
	{
		if (_editingStageAux == 0)
			editItem(STR(Protease),value,ToTempInStorage(60),ToTempInStorage(45),&displayStageTemperature);
		else
			editItem(STR(Protease),value,MAX_STAGE_TIME,MIN_STAGE_TIME,&displayTime);	
	}
	else if(_editingStage ==4)
	{
		if (_editingStageAux == 0)
			editItem(STR(bAmylase),value,ToTempInStorage(70),ToTempInStorage(50),&displayStageTemperature);
		else
			editItem(STR(bAmylase),value,MAX_STAGE_TIME,MIN_STAGE_TIME,&displayTime);	
	}
	else if(_editingStage ==5)
	{
		if (_editingStageAux == 0)
			editItem(STR(aAmylase1),value,ToTempInStorage(76),ToTempInStorage(60),&displayStageTemperature);
		else
			editItem(STR(aAmylase1),value,MAX_STAGE_TIME,MIN_STAGE_TIME,&displayTime);	
	}
	else if(_editingStage ==6)
	{
		if (_editingStageAux == 0)
			editItem(STR(aAmylase2),value,ToTempInStorage(76),ToTempInStorage(60),&displayStageTemperature);
		else
			editItem(STR(aAmylase2),value,MAX_STAGE_TIME,MIN_STAGE_TIME,&displayTime);	
	}
 */
	else if(_editingStage == C_MASHOUT_STAGE )
	{
		// MashOut
		if (_editingStageAux == 0)
#ifdef TESTBED       
      editItem(STR(Mash_out),value,ToTempInStorage(30),ToTempInStorage(22),&displayStageTemperature);     
#else      
      editItem(STR(Mash_out),value,ToTempInStorage(80),ToTempInStorage(75),&displayStageTemperature);
#endif      
		else
#ifdef TESTBED       
    editItem(STR(Mash_out),value,10,1,&displayTime);       
#else
      editItem(STR(Mash_out),value,MAX_STAGE_TIME,MIN_STAGE_TIME,&displayTime); 
#endif    
	}
	else if(_editingStage == C_NUMHOPS_STAGE )
	{
		// 8. number of hops
		value =readSetting(PS_NumberOfHops);
		// boiling, need to input 
		editItem(STR(Number_Of_Hops),value,10,0,&displaySimpleInteger);
	}
	else if(_editingStage == C_BOIL_STAGE )
	{
		// 9. boil time
		value =readSetting(PS_BoilTime);
		// boiling, need to input 
#ifdef TESTBED       
      editItem(STR(Boil),value,10,1,&displayTime);
#else
    editItem(STR(Boil),value,MAX_STAGE_TIME,MIN_STAGE_TIME,&displayTime);
#endif         
	}
	else //if(_editingStage ==10)
	{
		//10. hops
		value=readSetting(PS_TimeOfHop(_editingStageAux));
		
		if(value>_maxHopTime) value=_maxHopTime;
		//create a number
		// hop number starts from 1
		
		editItem(STR(Hops_Number_x),value,_maxHopTime,0,&displayTime);
		editItemTitleAppendNumber(_editingStageAux+1); // 
	}
}

void settingAutoSetup(void)
{
	_editingStage= C_MASHIN_STAGE;
	_editingStageAux=0;
	settingAutomationDisplayItem();
}

void settingAutoEventHandler(byte)
{
	if(btnIsEnterPressed)
	{
		int value=editItemValue();
		
		if(_editingStage == C_MASHIN_STAGE )
		{			
			
			_editingStageAux=0;
		}
		if(_editingStage <= C_MASHOUT_STAGE)
		{
			if(_editingStageAux ==0)
			{
				updateSettingWord(PS_StageTemperatureAddr(_editingStage),value);
			
				if(_editingStage== C_MASHIN_STAGE ) // no time needed for Mash In
					_editingStage++;
				else
					_editingStageAux=1;
			}
			else
			{
				updateSetting(PS_StageTimeAddr(_editingStage),(byte)value);
				//next stage	
				_editingStageAux=0;
				_editingStage++;
			}
		}
		else if(_editingStage == C_NUMHOPS_STAGE )
		{
			updateSetting(PS_NumberOfHops,(byte)value);
			//number of hops
			_editingStage++;
		}
		else if(_editingStage == C_BOIL_STAGE )
		{
			updateSetting(PS_BoilTime,(byte)value);
			
			// set the maxHopTime for the first hop
			_maxHopTime=value;
			// boiling time;
			
			_editingStageAux=0;
			_editingStage++;
		}
		else // if(_editingStage == 10)
		{
			updateSetting(PS_TimeOfHop(_editingStageAux),(byte)value);
			
			// update masxHoptime for next hop
			_maxHopTime = value-1;
			
			int hopsNum=readSetting(PS_NumberOfHops);
			
			if(_editingStageAux == (hopsNum-1))
			{
				//finish
				uiClearSettingRow();
				switchApplication(SETUP_SCREEN);
				return;
			}
			else
			{
				_editingStageAux++;

			}
		}
		//next item
		settingAutomationDisplayItem();
	}
/* 
	else if(btnIsStartPressed)
	{
		// only handle in stage 1 to 5
		if((_editingStage >= C_MAINMASH_STAGE && _editingStage < C_MASHOUT_STAGE)
			&& _editingStageAux == 0 )
		{
			// skip the stage, set the time to zero, and move to 
			// next stage.
			updateSetting(PS_StageTimeAddr(_editingStage),(byte)0);

			_editingStage++;
			settingAutomationDisplayItem();
		}
	}
 */
	else if(btnIsUpPressed)
	{
		if(_editingStage < C_NUMHOPS_STAGE && _editingStageAux == 0)
			editItemChange(+4);
		else
			editItemChange(+1);
	}
	else if(btnIsDownPressed)
	{
		if(_editingStage < C_NUMHOPS_STAGE && _editingStageAux == 0)
			editItemChange(-4);
		else
			editItemChange(-1);
	}
	else if(btnIsUpContinuousPressed)
	{
		if(_editingStage < C_NUMHOPS_STAGE && _editingStageAux == 0)
			editItemChange(+12);
		else
			editItemChange(+4);
	}
	else if(btnIsDownContinuousPressed)
	{
		if(_editingStage < C_NUMHOPS_STAGE && _editingStageAux == 0)
			editItemChange(-12);
		else
			editItemChange(-4);
	}
}// end of void settingAutoEventHandler(byte)

// *************************
//* Bluetooth Menu/Settings
// *************************
#if BT_Menu == true
// reuse _currentUnitSetting as indexing variable
#define _currentBtMenuItem _currentUnitSetting 

char _pinBuf[7];
short _editingIndex;

void btDisplayItem(void)
{
	if(_currentBtMenuItem ==0)
	{
		// network name
		uiMenu(STR(x_x_x_Ok));
		uiSettingTitle(STR(BT_NetworkName));
		uiSettingDisplayTextDynamic(btNetworkName);
	}
	else if(_currentBtMenuItem ==1)
	{
		// use PIN?
		if(gIsConnected)
			uiMenu(STR(x_x_x_Ok));
		else
			uiMenu(STR(Up_Down_x_Ok));

		uiSettingTitle(STR(BT_UsePin));
		// btSecurityType == 0 : no security

		editItem(STR(BT_UsePin),btSecurityType != 0,1,0,&displayYesNo);
		
	}
	else if(_currentBtMenuItem ==2)
	{
		// PIN CODE
		if(gIsConnected)
			uiMenu(STR(x_x_x_Ok));
		else
			uiMenu(STR(x_x_Edit_Ok));
			
		uiSettingTitle(STR(BT_PIN));
		uiSettingDisplayTextDynamic(btPinCode);		
	}
}

void btMenuSetup(void)
{
	_editingIndex = -1;
	_currentBtMenuItem=0;
	btDisplayItem();
}
void btMenuEventHandler(byte event)
{
	if(btnIsEnterPressed)
	{
		bool uplevel=false;
		if(_currentBtMenuItem ==0)
		{
			// do nothing for now.
		}
		else if(_currentBtMenuItem ==1)
		{
			int value=editItemValue();
			if(value && btSecurityType==0)
			{
				// change to use PIN
				btSecurityType=btSetUsePinCode(true);
			}
			else if(!value && btSecurityType!=0)
			{
				// change to disable PIN
				btSecurityType=btSetUsePinCode(false);
			}
			
			//if(btSecurityType ==0) uplevel=true;
		}
		else // 2
		{
			if(_editingIndex >=0) return; // if editing return
			
			if(strcmp(_pinBuf,btPinCode)!=0)
				btSetPinCode(_pinBuf);
			uplevel=true;
		}
		
		if(uplevel)
		{
			//finish
			uiClearSettingRow();
			switchApplication(SETUP_SCREEN);
			return;
		}
		else
		{
			_currentBtMenuItem++;
			btDisplayItem();
		}
	}
	
	// handle other button only if not connected
	if(gIsConnected) return;
	
	if(_currentBtMenuItem ==1)
	{	
		 if(btnIsUpPressed)
		{
			editItemChange(+1);
		}
		else if(btnIsDownPressed)
		{
			editItemChange(-1);
		}
	}
	else if(_currentBtMenuItem ==2)
	{
		if(btnIsStartPressed)
		{
			if(_editingIndex <0)
			{
				strcpy(_pinBuf,btPinCode);
				// enter edit mode
				uiMenu(STR(Up_Down_Next_x));
				uiEditTextStart(_pinBuf);
				_editingIndex=0;
			}
			else
			{
				if(_editingIndex == (strlen(_pinBuf)-1))
				{
					// finish editing
					_editingIndex=-1;

					uiMenu(STR(x_x_Edit_Ok));
					uiEditTextEnd();
				}
				else
				{
					uiEditTextNext();
					_editingIndex++;
				}
			}
		}
		else if(btnIsUpPressed)
		{
			if(_editingIndex <0) return;
			if((_pinBuf[_editingIndex] - '0') > 1)
			{
				_pinBuf[_editingIndex] --;
				uiEditTextSetCharAtCursor(_pinBuf[_editingIndex]);
			}
		}
		else if(btnIsDownPressed)
		{
			if(_editingIndex <0) return;
			if((_pinBuf[_editingIndex] - '0') < 9)
			{
				_pinBuf[_editingIndex] ++;
				uiEditTextSetCharAtCursor(_pinBuf[_editingIndex]);
			}

		}
	}
}
#endif

// *************************
//*  Level 1 Menu (settings)
// *************************

str_t const level1Menu[]={STR(PID_PWM),STR(Unit_Parameters),STR(Set_Automation)
#if BT_Menu == true
,STR(BT_Setup)
#endif
};
/*const byte level1Screens[]={PID_SETTING_SCREEN,UNIT_SETTING_SCREEN,AUTO_SETTING_SCREEN
#if BT_Menu == true
,BT_MENU_SCREEN
#endif
};
*/
void menuDisplayList(byte index)
{
	byte menuNO=(sizeof(level1Menu) / sizeof(char const*)) -1;

		#if BT_Menu == true
			if(! gIsBtModulePresent)
				menuNO --;
		#endif

	uiSubTitle(level1Menu[index]);
	if(index ==0)
		uiMenu(STR(x_Down_Quit_Ok));
	else if(index == menuNO)
		uiMenu(STR(Up_x_Quit_Ok));
	else 
		uiMenu(STR(Up_Down_Quit_Ok));
}

byte _currentLevelOne=0;
void menuSetup(void)
{
	uiTitle(STR(Setup));	
//	_currentLevelOne=0; "remember" last menu position
	menuDisplayList(_currentLevelOne);
	
	#if BluetoothSupported == true
	btReportCurrentStage(StageSetting);
	#endif
}

void menuEventHandler(byte event)
{
	if(btnIsEnterPressed)
	{
		//go to level 2
		//switchApplication(level1Screens[_currentLevelOne]);
		if(_currentLevelOne ==0)
			switchApplication(PID_SETTING_SCREEN);
		else if(_currentLevelOne ==1)
			switchApplication(UNIT_SETTING_SCREEN);
		else if(_currentLevelOne ==2) 
			switchApplication(AUTO_SETTING_SCREEN);
#if BT_Menu == true
		else if(_currentLevelOne ==3) 
			switchApplication(BT_MENU_SCREEN);
#endif
	}
	else if(btnIsStartPressed)
	{
		// got to main
		backToMain();	
	}
	else if(btnIsUpPressed)
	{
		if(_currentLevelOne>0)
		{
			_currentLevelOne--;
			menuDisplayList(_currentLevelOne);
		}
	}
	else if(btnIsDownPressed)
	{
		
		byte menuNO=(sizeof(level1Menu) / sizeof(char const*)) -1;
		
		#if BT_Menu == true
			if(! gIsBtModulePresent)
				menuNO --;
		#endif
		
		if(_currentLevelOne < menuNO)
		{
			_currentLevelOne++;
			menuDisplayList(_currentLevelOne);
		}
	}
}

// ***************************************************************************
//*  Common function to handle Setting temperature
//*
// ***************************************************************************
boolean _isEnterPwm;

float _maxAdjustTemp;
float _minAdjustTemp;

void togglePwmInput(void)
{

    #ifdef DEBUG
      Serial.print ( "gCurrentTemperature: " );
      Serial.print ( gCurrentTemperature );
      Serial.print ( " gSettingTemperature: " );
      Serial.print ( gSettingTemperature );  
      Serial.print ( " gBoilStageTemperature: " );
      Serial.println ( gBoilStageTemperature );   
    #endif        
			//turn on/off PWM
			if(gCurrentTemperature >= gSettingTemperature
				&& gSettingTemperature >= gBoilStageTemperature)
			{
				if(!_isEnterPwm)
				{
					uiShowPwmLabel();
					uiShowPwmValue(gBoilHeatOutput);
					_isEnterPwm=true;
					#if BluetoothSupported == true
					btReportPwm();
					btReportEvent(RemoteEventPwmOn);
					#endif
				}
			}
			else
			{
				if(_isEnterPwm)		
				{
					// turn off
					uiClearPwmDisplay();
					_isEnterPwm = false;
					#if BluetoothSupported == true
					btReportEvent(RemoteEventPwmOff);
					#endif

				}
			}
}

void setAdjustTemperature(float max, float min)
{
	_maxAdjustTemp=max;
	_minAdjustTemp=min;
}

void adjustSp(float adjust)
{
	gSettingTemperature += adjust;
	
	if(gSettingTemperature > _maxAdjustTemp) gSettingTemperature=_maxAdjustTemp;
	if(gSettingTemperature < _minAdjustTemp) gSettingTemperature=_minAdjustTemp;
	
	uiDisplaySettingTemperature(gSettingTemperature);
	// if adjust above delta
		
	#if BluetoothSupported == true
	btReportSettingTemperature();
	#endif

}
void adjustPwm(int adjust)
{
	// gBoilHeatOutput is byte, which is, uhhh,, unsigned
	
	// excludes the case adjust <0 and gBoilheatOutput ==0
	if(adjust > 0 || gBoilHeatOutput !=0)
		gBoilHeatOutput += adjust;
	
	if(gBoilHeatOutput > 100) gBoilHeatOutput=100;
	
	uiShowPwmValue(gBoilHeatOutput);
	
	#if BluetoothSupported == true
	btReportPwm();
	#endif
}

boolean processAdjustButtons(void)
{
	if(btnIsUpPressed)
	{
		if(_isEnterPwm)
			adjustPwm(+1);
		else
		{
			adjustSp(+0.25);
		}
	}
	else if(btnIsDownPressed)
	{
		if(_isEnterPwm)
			adjustPwm(-1);
		else
		{
			adjustSp(-0.25);
		}
	}
	else if(btnIsUpContinuousPressed)
	{
		if(_isEnterPwm)
			adjustPwm(+2);
		else
		{
			adjustSp(+0.75);
		}
	}
	else if(btnIsDownContinuousPressed)
	{
		if(_isEnterPwm)
			adjustPwm(-2);
		else
		{
			adjustSp(-0.75);
		}
	}
	else
	{
		return false;
	}
	return true; // handled
}

// ***************************************************************************
//*  Manual Mode Screen
//*
// ***************************************************************************

#define DEFAULT_MANUL_MODE_TEMPERATURE 35.0
//states variables

#define MSAskWater 0
// display "Water Added?"

#define MSWaitTemperature  1
#define MSTemperateReached 2

byte _state;

#if SupportManualModeCountDown == true
unsigned long manualModeChangeCountDownTime; 
bool isCountDownTimeBlinking;
bool isManualModeCountDownMode;
#endif
//
void manualModeSetup(void)
{
	uiTitle(STR(Manual_Mode));
	uiSubTitle(STR(Water_Added));
	uiMenu(STR(Continue_Yes_No));
	_state = MSAskWater;
	_isEnterPwm=false;
	
	gSettingTemperature = DEFAULT_MANUL_MODE_TEMPERATURE;
}

#define TEMPERATURE_ADJUST_THRESHOLD 2.0

//function after confirmation of adding water 
void manualModeEnterManualMode(void)
{
	uiClearSubTitleRow();
	uiMenu(STR(Up_Down_Heat_Pmp));
	
	// Setpoint temperature
	uiDisplaySettingTemperature(gSettingTemperature);
	// displace current temperature
	uiTempDisplaySetPosition(TemperatureManualModePosition);
	// display counting time
	uiRunningTimeSetPosition(RunningTimeNormalPosition);
	uiRunningTimeShowInitial(0);	
	
	if(gCurrentTemperature >=gSettingTemperature)
	{
		//temp reached
		_state=MSTemperateReached;
		uiRunningTimeStart();
	}
	else
	{
		_state=MSWaitTemperature;
		// wait to reach temperature setting
	}
	
	#if SupportManualModeCountDown == true
	setEventMask(TemperatureEventMask | ButtonPressedEventMask | TimeoutEventMask);
	#endif
	
	setAdjustTemperature(110.0,20.0);
	
	#if BluetoothSupported == true
	btReportCurrentStage(StageManualMode);
	btReportSettingTemperature();
	#endif
}


void manualModeEventHandler(byte event)
{

	if(_state == MSAskWater)
	{
		//only button event will come before
		// we change it
		//if(event != ButtonPressedEventMask) return;
		
		// cares only button start & enter
		if(btnIsStartPressed)
		{
			// yes.
			//load heating parameters
			loadBrewParameters();
			manualModeEnterManualMode();
			// setup to be called for Timer & temperature event
			
			setEventMask(ButtonPressedEventMask|TemperatureEventMask|TimeoutEventMask);
		}
		else if(btnIsEnterPressed)
		{
			// NO. back to main
			switchApplication(MAIN_SCREEN);
		}
	}
	else  // NOT Wait for Water State
	{
		// states other than MSAskWater, handle Heat & Pump button
		if(event == ButtonPressedEventMask)
		{
			if(btnIsStartPressed)
			{			
				//turn heating on/off
				if(gIsHeatOn) heatOff();
				else heatOn();
			}
			else if(btnIsEnterPressed)
			{
				// turn pump on/off
				if(gIsPumpOn) pumpOff();
				else pumpOn();
			}
			#if SupportManualModeCountDown == true
			else if(isExactButtonsPressed(ButtonEnterMask | ButtonStartMask))
			{
				isCountDownTimeBlinking = ! isCountDownTimeBlinking;
				if(isCountDownTimeBlinking)
				{
					uiRunningTimeShowInitial(0);
					manualModeChangeCountDownTime=0;
					tmPauseTimer();
					_state = MSTemperateReached; // force to ignore temperature 
				}
				else
				{
					// end of input. if zero, make it normal mode
					// else make it count down mode
					isManualModeCountDownMode=(manualModeChangeCountDownTime > 0);
					_state = MSWaitTemperature;
				}
				uiRunningTimeBlink(isCountDownTimeBlinking);

			}
			else if(isCountDownTimeBlinking)
			{
				if(btnIsUpPressed)
				{
					if(manualModeChangeCountDownTime< 140)
					{
						manualModeChangeCountDownTime ++;
						uiRunningTimeShowInitial(manualModeChangeCountDownTime * 60);
					}
				}
				else if(btnIsDownPressed)
				{
					if(manualModeChangeCountDownTime> 0)
					{
					 	manualModeChangeCountDownTime --;
					 	uiRunningTimeShowInitial(manualModeChangeCountDownTime * 60);
					}
				}
			}
			#endif
			else
			{
				if(processAdjustButtons())
				{
					if( MSTemperateReached == _state
							&&	gSettingTemperature >= (TEMPERATURE_ADJUST_THRESHOLD + gCurrentTemperature))
					{
						#if SupportManualModeCountDown == true

						// pause counting, reset it

						if(isManualModeCountDownMode)
						{
							uiRunningTimeShowInitial(manualModeChangeCountDownTime * 60);
						}
						else
						#endif
							uiRunningTimeShowInitial(0);
						_state = MSWaitTemperature;
					}
				}
			}
		} 
		else if(event == TemperatureEventMask)
		{
			// Handle temperature change or other states 
			//
			if (_state == MSWaitTemperature)
			{
				if(gCurrentTemperature >= gSettingTemperature)
				{	
					// beep & start counting time
					
					buzzPlaySound(SoundIdTemperatureReached);
					
					_state=MSTemperateReached;
					
					#if SupportManualModeCountDown == true
					if(isManualModeCountDownMode)
					{
						uiRunningTimeStartCountDown(manualModeChangeCountDownTime*60);
						tmSetTimeoutAfter(manualModeChangeCountDownTime*60 * 1000);
					}
					else
					#endif
					uiRunningTimeStart();
					
					#if BluetoothSupported == true
					btReportEvent(RemoteEventTemperatureReached);
					#endif
				}
			}
			// MSTemperateReached state				
			
			togglePwmInput();

		} // end of temperature handling
		#if SupportManualModeCountDown == true
		else if(event == TimeoutEventMask)
		{
			buzzPlaySound(SoundIdCountDown);
			isManualModeCountDownMode=false;
			uiRunningTimeStart();
		}
		#endif
	} // else of if(_state == MSAskWater)
}//void manualModeEventHandler(byte event)

// *************************
//*  Auto Mode Screen
// *************************

// delay_start_q
// (resume ?)
// water_added_q
// pmmp Prime
// input delay

// use the same variable  with maunal mode 
//byte _state;

#define AS_AskDelayStart   0
#define AS_AskResume      1
#define AS_AskWaterAdded  2
#define AS_PumpPrime    3
#define AS_DelayTimeInput   4
#define AS_DelayTimeConfirm 5
#define AS_DelayWaiting     6

#define AS_MashIn     7
#define AS_MashInAskContinue   8
#define AS_AskAddMalt       9

#define AS_Mashing          10
//#define AS_Mashout , the same as Mashing procedure
#define AS_AskMaltRemove   11
#define AS_Boiling       12

//#define AS_Whirlpool       13
//#define AS_IodineTest      14
#define AS_Pause       15
//#define AS_Cooling       16
#define AS_Finished       17


#define HOP_ALTERTING_TIME 10
#define ADVANCE_BEEP_TIME 5

#define AutoStateIs(s) (_state==(s))

byte _primePumpCount;

//**************************
// Delay start

#if NoDelayStart == false
unsigned long _delayTime;
bool _delayRequested;
#endif

void autoModeSetup(void)
{
#if SupportAutoModeRecovery == true
	if(readSetting(PS_AutomodeStarted))
	{
		_state=AS_AskResume;
		uiSubTitle(STR(Resume_Process));
		uiMenu(STR(Continue_Yes_No));
		return;
	}
	//else
#endif

#if NoDelayStart == false
	_state=AS_AskDelayStart;
	_delayTime=0;
	// output Delay State
	uiTitle(STR(AutomaticMode));
	uiSubTitle(STR(Delay_Start));
	uiMenu(STR(No_Yes));
#else
	_state = AS_AskWaterAdded;
	uiTitle(STR(AutomaticMode));
	uiSubTitle(STR(Water_Added));
	uiMenu(STR(Continue_Yes_No));

#endif
}

void autoModeEnterMashIn(void)
{
	// load saving settings here, the latest timing for it.
	loadBrewParameters();
	
	_state = AS_MashIn;
	// setup temperature event mask request after this.
	setEventMask(TemperatureEventMask /*| ButtonPressedEventMask */);
	
	//load temperature value	
	gSettingTemperature = TempFromStorage(readSettingWord(PS_StageTemperatureAddr(0)));

	// setup screen
	uiClearTitle();
	uiAutoModeTitle();
	uiClearSubTitleRow();
	uiAutoModeStage(0); // 0 is Mash-In
	
	// displace temperature
	uiDisplaySettingTemperature(gSettingTemperature);
	uiTempDisplaySetPosition(TemperatureAutoModePosition);

	uiMenu(STR(Up_Down_Pause_x));

	// start pump, if request,
	
	if(readSetting(PS_PumpPreMash)) pumpOn();
	else pumpOff();
	
	// start heat
	heatOn();	

	setAdjustTemperature(75.0,25.0);
	_isEnterPwm=false;
	
	#if BluetoothSupported == true
	btReportCurrentStage(StageMashIn);
	#endif
}

//************************************
// for recovery
//

#if SupportAutoModeRecovery == true	

#define LEVEL_FACTOR 5

bool autoModeRecoveryTimeTracking;
byte autoModeLastRecoveryTime;
byte autoModeRecoveryIndex;

void autoModeRecoveryTimeReset(void)
{
	for(byte i=0; i < LEVEL_FACTOR; i++)
		updateSetting(PS_StageTimeLeft + i, 0xFF);
}

void autoModeRecoveryInit(void)
{
	updateSetting(PS_AutomodeStarted,1);
	//	autoModeLastRecoveryTime=0;
	//PS_StageTimeLeft
}

void autoModeUpdateRecoveryTime(byte time)
{
	//PS_StageTimeLeft
	autoModeLastRecoveryTime=time;
	
	updateSetting(PS_StageTimeLeft + autoModeRecoveryIndex, time);
	autoModeRecoveryIndex++;
	if(autoModeRecoveryIndex >= LEVEL_FACTOR) autoModeRecoveryIndex=0;
}

void autoModeTrackRecoveryTime(void)
{
	byte remain=(tmGetRemainingTime() + 30000)/60000;
	
	if(remain != autoModeLastRecoveryTime)
	{
		autoModeUpdateRecoveryTime(remain);
	}
}

byte autoModeGetRecoveryTime(void)
{
	autoModeLastRecoveryTime = 0xFF;
	// get smallest value
	for(byte i=0; i < LEVEL_FACTOR; i++)
	{
		byte value=readSetting(PS_StageTimeLeft + i);
		if( autoModeLastRecoveryTime > value)
		{
			#if SerialDebug == true
			Serial.print(F("RecoveryTime: "));
			Serial.println(value);
			#endif
			autoModeLastRecoveryTime = value;
			autoModeRecoveryIndex=i;
		}
	}
	return autoModeLastRecoveryTime;
}

#endif


//************************************
// Mashing state
//

byte _mashingStep;
boolean _mashingTemperatureReached;

boolean _askingSkipMashingStage;


void autoModeNextMashingStep(void)
{
	//[TODO:] the algorithm here assumes
	//  1. step 6 (alphaAmylase2) and step 7 (MashOut) can not be skipped.!!
	// once they don't. we should check it here before go to next stage
	//	
	
	_mashingStep++;

	byte time;
	while((time = readSetting(PS_StageTimeAddr(_mashingStep))) == 0)
	{
		_mashingStep++;
	}
	// 	if(_mashingStep > 7), mashout time will always more than 1
	
	uiAutoModeStage(_mashingStep);
  uiClearPrompt();
	uiRunningTimeSetPosition(RunningTimeNormalPosition);
	uiRunningTimeShowInitial(time * 60);

#if SupportAutoModeRecovery == true
	autoModeRecoveryTimeReset();
	autoModeRecoveryInit();
	updateSetting(PS_StageResume,_mashingStep);
	autoModeUpdateRecoveryTime(time);
#endif
	
	gSettingTemperature = TempFromStorage(readSettingWord(PS_StageTemperatureAddr(_mashingStep)));	
	uiDisplaySettingTemperature(gSettingTemperature);
	
	uiMenu(STR(Up_Down_Pause_STP));
	_mashingTemperatureReached=false;

	if(isPumpRest())
	{
		heatOn();
	}
	pumpRestSetEnabled(false);
	
	if(_mashingStep < C_MASHOUT_STAGE )
	{
		// pump is off at the time AddMalt
		if(readSetting(PS_PumpOnMash)) pumpOn();
		else pumpOff();
	}
	else if(_mashingStep ==  C_MASHOUT_STAGE )
	{
		if(readSetting(PS_PumpOnMashOut)) pumpOn();
		else pumpOff();
	}

	#if BluetoothSupported == true
	btReportCurrentStage(_mashingStep);
	#endif	
}

void autoModeEnterMashing(void)
{
	_state = AS_Mashing;
	setEventMask(TemperatureEventMask | ButtonPressedEventMask | TimeoutEventMask | PumpRestEventMask);

	_askingSkipMashingStage = false;
	_mashingStep = C_MASHIN_STAGE; // 0 is mash in , real mashing starts from 1
	autoModeNextMashingStep();
}




void autoModeEnterAskRemoveMalt(void)
{
	_state = AS_AskMaltRemove;
	pumpOff();
	
	uiRunningTimeStop();
	uiClearPrompt();
	uiPrompt(STR(Remove_Malt));
	uiMenu(STR(Continue_Yes_No));
	// skip event mask, just filter it out in handling code
	
	buzzPlaySoundRepeat(SoundIdWaitUserInteraction);

	if(readSetting(PS_PidPipe) == 1 && readSetting(PS_SensorType) == 0)
		heatProgramOff(); // heat off, programming
	
	#if BluetoothSupported == true
	btReportEvent(RemoteEventRemoveMalt);
	#endif
}

//******************************
// Pause

byte _stateBeforePause;
unsigned long _savedTime;
boolean _savedHeating;
boolean _savedPump;
void autoModePause(unsigned long time)
{	

  #ifdef DEBUG
    Serial.print ( "Pausing, time passed : " );
    Serial.println ( time );
   #endif   
	_stateBeforePause = _state;
	_state = AS_Pause;
	// stop Heating & pump
	_savedHeating = gIsHeatOn;
	_savedPump = gIsPumpOn;
	heatOff();
	pumpOff();
	// just wait for user button
	
	uiPreparePauseScreen(STR(In_Pause));
	uiMenu(STR(x_x_Exit_x));
	
	_savedTime=time;
	
	uiRunningTimeShowInitial(_savedTime/1000);
		
	#if BluetoothSupported == true
	btReportEvent(RemoteEventPause);
	#endif
}

void autoModeExitPause(void)
{

  #ifdef DEBUG
    Serial.print ( "Restoring to state: " );
    Serial.println ( _stateBeforePause );
    Serial.print ( "Saved time: " );
    Serial.println ( _savedTime );
    Serial.print ( "Mashing step: " );
    Serial.println ( _mashingStep );    
   #endif    
   
	// restore state
	_state = _stateBeforePause;
	
	// restore timer, if any
	if(_savedTime > 0)
	{
		tmSetTimeoutAfter(_savedTime);
		if(_savedTime > ADVANCE_BEEP_TIME*1000)
			tmSetAuxTimeoutAfter(_savedTime -ADVANCE_BEEP_TIME*1000);
	}
	// restore screen
	uiClearScreen();
	
	uiAutoModeTitle();
	uiAutoModeStage(_mashingStep);

	// temperateure position
	uiTempDisplaySetPosition(TemperatureAutoModePosition);
	// set temperature point
	uiDisplaySettingTemperature(gSettingTemperature);

	// counting time
	uiRunningTimeSetPosition(RunningTimeNormalPosition);
	
	if(_savedTime==0)
	{
		if(_state != AS_MashIn)
		{
			byte time = readSetting(PS_StageTimeAddr(_mashingStep));
			uiRunningTimeShowInitial(time * 60);
		}
	}
	else //if(_savedTime==0)
	{
		// temperature reached, timer already started
		// this should always NOT be MashIn
		if(_state == AS_Mashing)
		{
			uiRunningTimeShowInitial(_savedTime/1000);
			uiRunningTimeStartCountDown(_savedTime/1000);
		}
	}
		
	// menu is different for mashin & mashing
  #ifdef DEBUG
    Serial.print ( "Before UI set, state : " );
    Serial.println ( _state );
   #endif 
    
	if(_state == AS_MashIn)
		uiMenu(STR(Up_Down_Pause_x));
	else
		uiMenu(STR(Up_Down_Pause_STP));
		
	// restore heating and pump
	if(_savedHeating) heatOn();
	if(_savedPump) pumpOn();
	
	#if BluetoothSupported == true
	btReportEvent(RemoteEventResume);
	#endif
}

void autoModeMashingStageFinished(void)
{
#if SupportAutoModeRecovery == true
	autoModeRecoveryTimeTracking = false;
#endif
  #ifdef DEBUG
    Serial.print ( "Mashing stage finished, current stage: " );
    Serial.println ( _mashingStep );
  #endif
          
	//[TODO:] make sure step 6 , beta 2 is non-skippable.
	if(_mashingStep <  C_MASHOUT_STAGE ) // step 7 = mashout
	{
			autoModeNextMashingStep();
	}
	else
	{
		// change to boiling stage, or malt out waiting state
			if(readSetting(PS_SkipRemoveMalt))
				autoModeEnterBoiling();
			else
				autoModeEnterAskRemoveMalt();
	}
  #ifdef DEBUG
    Serial.print ( "Next stage: " );
    Serial.println ( _mashingStep );
  #endif
}

//*********************************
// boiling stage

boolean _isBoilTempReached;

void autoModeEnterBoiling(void)
{
	_state = AS_Boiling;
	_isBoilTempReached=false;
	gBoilStageTemperature=readSetting(PS_BoilTemp);
	//gSettingTemperature =110;//
	gSettingTemperature = gBoilStageTemperature;
	
	uiDisplaySettingTemperature(gSettingTemperature);
	// display time
	byte boilTime=readSetting(PS_BoilTime);

#if SupportAutoModeRecovery == true	
	autoModeRecoveryTimeReset();
	updateSetting(PS_StageResume,C_BOIL_STAGE);
	autoModeUpdateRecoveryTime(boilTime);
#endif

	uiRunningTimeShowInitial(boilTime * 60);
	
	uiAutoModeStage(BoilingStage);
	uiMenu(STR(Up_Down_x_x));
	
	if(readSetting(PS_PumpOnBoil)) pumpOn();
	else pumpOff();

	setAdjustTemperature(110.0,70.0);
	_isEnterPwm =false;
	heatOn();
	#if BluetoothSupported == true
	btReportCurrentStage(StageBoil);	
	#endif
}



// reuse the variable 
//#define _restoreBoilingTimer _finishedTimer
#define _numHopToBeAdded _primePumpCount

void autoModeShowHopAdding(void)
{
	uiAutoModeShowHopNumber(readSetting(PS_NumberOfHops) - _numHopToBeAdded +1);
}

#define AUX_TIMER_HOP 1

#ifdef AUX_TIMER_HOP // try use Aux timer for HOP

bool recoveryTimer;

void autoModeAddHopNotice(void)
{
			
			// the first hop is added at the time boiling starts
			tmSetAuxTimeoutAfter(HOP_ALTERTING_TIME * 1000);
			recoveryTimer = true;
			
			autoModeShowHopAdding();
			_numHopToBeAdded --;
			buzzPlaySound(SoundIdAddHop);
			
			#if BluetoothSupported == true
			btReportEvent(RemoteEventAddHop);
			#endif			
}


void autoModeStartBoilingTimer(void)
{
	// [IMPORTANT!] cast to (unsigned long) is needed
	// NO hop adding. just start last before 

	byte boilTime=readSetting(PS_BoilTime);
	_numHopToBeAdded =  readSetting(PS_NumberOfHops);

	tmSetTimeoutAfter((unsigned long)boilTime * 60 *1000);
			
	if(_numHopToBeAdded > 0)
	{
		byte nextHopTime=boilTime - readSetting(PS_TimeOfHop(0));
		
		if(nextHopTime == 0)
		{
			// alert directly, start timer to restore
			autoModeAddHopNotice();			
		}
		else
		{
			recoveryTimer = false;
			tmSetAuxTimeoutAfter((unsigned long)nextHopTime * 60 * 1000);
		}
	}
}

void autoModeStartNextHopTimer(void)
{
	// it is done at timer expires :_numHopToBeAdded--;
	
	byte currentHopIdx=readSetting(PS_NumberOfHops) - _numHopToBeAdded -1;
	byte currentHopTime=readSetting(PS_TimeOfHop(currentHopIdx));

	
	if(_numHopToBeAdded > 0) // there are next timer
	{
		byte nextHopTime= readSetting(PS_TimeOfHop(currentHopIdx+1));
		
		tmSetAuxTimeoutAfter(((unsigned long)(currentHopTime - nextHopTime) * 60 
								-HOP_ALTERTING_TIME)* 1000);
		recoveryTimer = false;
	}
}


#else

void autoModeAddHopNotice(void)
{
			
			// the first hop is added at the time boiling starts
			tmSetAuxTimeoutAfter(HOP_ALTERTING_TIME * 1000);
			
			autoModeShowHopAdding();
			_numHopToBeAdded --;
			buzzPlaySound(SoundIdAddHop);
			
			#if BluetoothSupported == true
			btReportEvent(RemoteEventAddHop);
			#endif			
}

// the first timer
void autoModeStartBoilingTimer(void)
{
	_numHopToBeAdded =  readSetting(PS_NumberOfHops);
	
	byte boilTime=readSetting(PS_BoilTime);
		
	if(_numHopToBeAdded > 0)
	{
		byte nextHopTime=boilTime - readSetting(PS_TimeOfHop(0));
		
		if(nextHopTime == 0)
		{
			// alert directly, start timer to restore
			autoModeAddHopNotice();
			
			// we need to get next hop, or timer
			autoModeStartNextHopTimer();
		}
		else
		{
			tmSetTimeoutAfter((unsigned long)nextHopTime * 60 * 1000);
		}
	}
	else
	{
		// [IMPORTANT!] cast to (unsigned long) is needed
		// NO hop adding. just start last before 
		tmSetTimeoutAfter((unsigned long)boilTime * 60 *1000);
	}
}

void autoModeStartNextHopTimer(void)
{
	// it is done at timer expires :_numHopToBeAdded--;
	
	
	byte currentHopIdx=readSetting(PS_NumberOfHops) - _numHopToBeAdded -1;
	byte currentHopTime=readSetting(PS_TimeOfHop(currentHopIdx));

#if 0 // SerialDebug == true
	Serial.print(F("autoModeStartNextHopTimer _numHopToBeAdded:"));
	Serial.print(_numHopToBeAdded);
	Serial.print(F(" ,currentHopIdx="));
	Serial.println(currentHopIdx);
#endif
	
	if(_numHopToBeAdded > 0) // there are next timer
	{
		byte nextHopTime= readSetting(PS_TimeOfHop(currentHopIdx+1));
		
		tmSetTimeoutAfter((unsigned long)(currentHopTime - nextHopTime)* 60 * 1000);
	}
	else
	{
		// NO other hop adding. just start last before 
		tmSetTimeoutAfter((unsigned long)currentHopTime * 60 *1000);
	}
}
#endif
//******************************
#if NoDelayStart == false
// unit is 15 min, use 12 hour, 
#define MAX_DELAY_TIME (12*60/15)

void autoModeEnterDelayTimeInput(void)
{
	_state = AS_DelayTimeInput;
	uiClearSubTitleRow();
	uiSubTitle(STR(Setting_Delay));
	uiMenu(STR(Up_Down_Quit_Ok));
	// use display time counting
	uiRunningTimeSetPosition(RunningTimeDelayInputPosition);
	uiRunningTimeShowInitial(15 *60);
	_delayTime=1; // one unit is 15min
}
#endif
//******************************


bool _stageConfirm;
boolean _pumpRunning;

#define BREW_END_STAY_DURATION 10


void autoModeBrewEnd(void)
{
  #ifdef DEBUG
    Serial.println ( "In autoModeBrewEnd" );
  #endif
    
	pumpOff();
	
	uiTempDisplayHide(); 
	uiRunningTimeStop();
	
	_state = AS_Finished;
  tmPauseTimer();
	
	uiClearScreen();
	
	uiAutoModeFinishScreen();

	buzzPlaySoundRepeat(SoundIdBrewEnd);
	tmSetTimeoutAfter(BREW_END_STAY_DURATION * 1000);
   
	#if BluetoothSupported == true
	btReportEvent(RemoteEventBrewFinished);
	#endif
}


//************************************
// for recovery
//

#if SupportAutoModeRecovery == true	

void autoModeResumeProcess(void)
{
	// get stage
	byte stage=readSetting(PS_StageResume);
	byte time=autoModeGetRecoveryTime();

	setEventMask(TemperatureEventMask | ButtonPressedEventMask | TimeoutEventMask | PumpRestEventMask);
	uiClearScreen();

	uiAutoModeTitle();
	//uiDisplaySettingTemperature(gSettingTemperature); will be set later in each entering procedure
	uiTempDisplaySetPosition(TemperatureAutoModePosition);
	uiRunningTimeSetPosition(RunningTimeNormalPosition);
	// time may be 0xFF, invalid, => not just enter
	// less then stage time. temperature reached
	//
	heatLoadParameters();
	heatOn();

	if (stage == C_BOIL_STAGE) // boiling
	{
		autoModeEnterBoiling();
		// if 0xfFF, assume not 
		if(time != 0xFF)
		{
			byte boilTime=readSetting(PS_BoilTime);

			if(time < boilTime)
			{
				// findout which hop is current 
				autoModeRecoveryTimeTracking = true;
			
				#if BluetoothSupported == true
				btReportEvent(RemoteEventTemperatureReached);
				#endif

				_isBoilTempReached=true;
				unsigned long sec=(unsigned long)time *60;
				uiRunningTimeStartCountDown(sec);
				tmSetTimeoutAfter(sec *1000);
				
				// start hop & boiling out timer
				byte hopnum =  readSetting(PS_NumberOfHops);	
				_numHopToBeAdded=hopnum;
				if(hopnum > 0)
				{
					byte i;
					byte nextHopTime;
					for(i=0;i<hopnum;i++)
					{
						nextHopTime=readSetting(PS_TimeOfHop(i));
						if(nextHopTime > time) _numHopToBeAdded--;
						else break;
					}
					if(_numHopToBeAdded > 0)
					{
						unsigned long hopTimer =time - nextHopTime;
						recoveryTimer = false;
						tmSetAuxTimeoutAfter((unsigned long)hopTimer * 60 * 1000);
					}
				}
				
			}

		}
	}
	else
	{
		// Mash-In is not record.
		// everything else is in MASHING state
		// just enter mashing step ... 
		_state = AS_Mashing;
		_askingSkipMashingStage = false;
		_mashingStep = stage - 1; // next step will increase the step		
		autoModeNextMashingStep();
		// adjust timer if necessary
		if(time != 0xFF)
		{
			byte stagetime = readSetting(PS_StageTimeAddr(_mashingStep));	
			if(time < stagetime)
			{
				_mashingTemperatureReached = true;
				unsigned long seconds=(unsigned long)time * 60;
				
				tmSetTimeoutAfter( seconds *1000);
				tmSetAuxTimeoutAfter((seconds-ADVANCE_BEEP_TIME) *1000);
				uiRunningTimeStartCountDown(seconds);	
				pumpRestSetEnabled(true);
				
				autoModeRecoveryTimeTracking = true;
				
				#if BluetoothSupported == true
				btReportEvent(RemoteEventTemperatureReached);
				#endif


			}
		}

	}
}

#endif

//******************************
// Auto Mode Event Handling
void autoModeEventHandler(byte event)
{
	// switch-case uses more memory, though it looks better
	// 
#if SupportAutoModeRecovery == true
	if(AutoStateIs( AS_AskResume))
	{
		if(btnIsStartPressed)
		{
			// YES
			autoModeResumeProcess();
		}
		else if(btnIsEnterPressed)
		{
			// clear the flag
			updateSetting(PS_AutomodeStarted,0);
			autoModeSetup();
		}
	}
	else
#endif
	
#if NoDelayStart == false	
	if(AutoStateIs( AS_AskDelayStart))
	{
		// initially only Button Event will come
		if(btnIsStartPressed)
		{
			// NO
			_delayRequested=false;
			// next state
			_state = AS_AskWaterAdded;
		}
		else if(btnIsEnterPressed)
		{
			// YES
			_delayRequested=true;
			
			_state = AS_AskWaterAdded;
		}
		
		// if state changed.		
		if (_state == AS_AskWaterAdded)
		{
			// ask resume, just ignore this for now
			uiSubTitle(STR(Water_Added));
			uiMenu(STR(Continue_Yes_No));
		}
	}//if(_state == AS_AskDelayStart)
	else 
#endif	
	
	if(AutoStateIs(AS_AskWaterAdded))
	{
		if(btnIsStartPressed)
		{
			_state = AS_PumpPrime;
			// request on off timer			
			setEventMask(TimeoutEventMask /*| ButtonPressedEventMask */);

			//[TODO:] pause or stop the action?
			uiNoMenu();
			uiSubTitle(STR(Pump_Prime));
			// start pump & timer
			pumpOn();
			_primePumpCount=0;
			tmSetTimeoutAfter(1000); // 1sec
			
		}
		else if(btnIsEnterPressed)
		{
			// NO; before heat & pump are used, it is safe to switch directly
			// instead of "backToMain"
			switchApplication(MAIN_SCREEN);
			return; // good bye
		}
	}  //end of state AS_AskWaterAdded
	else if(AutoStateIs(AS_PumpPrime))
	{
		if(event == TimeoutEventMask)
		{
			if(gIsPumpOn)
			{
				pumpOff();
				tmSetTimeoutAfter(2000);
			}
			else
			{
				_primePumpCount++;

				if(_primePumpCount < 5)
				{
					pumpOn();
					tmSetTimeoutAfter(1000 + _primePumpCount * 250);
				}
				else
				{
					// next stage is setting delay or mash start
#if NoDelayStart == false
					if(_delayRequested)
					{
						autoModeEnterDelayTimeInput();
					}
					else
					{
						//_state = AS_MashIn;
						autoModeEnterMashIn();
					}
#else
						autoModeEnterMashIn();
#endif
				} // else of prime pump < 5

			} // end of else if(gIsPumpOn)
		} // end of handling of TimeoutEventMask
	} // end of state AS_PumpPrime
#if NoDelayStart == false	
	else if(AutoStateIs(AS_DelayTimeInput))
	{
		// input delay timer
		if(event != ButtonPressedEventMask) return;
		
		if(btnIsUpPressed)
		{
			if( (_delayTime +1) < MAX_DELAY_TIME )
			{
				_delayTime ++;
				uiRunningTimeShowInitial(_delayTime * 15 * 60);
			}
		}
		else if(btnIsDownPressed)
		{
			if(_delayTime > 1)
			{
				_delayTime --;
				uiRunningTimeShowInitial(_delayTime * 15 * 60);
			}
		}
		else if(btnIsStartPressed)
		{
			// quite
			backToMain();
		}
		else if(btnIsEnterPressed)
		{
			_state = AS_DelayTimeConfirm;
			uiMenu(STR(Continue_Yes_No));
		}
	} // state AS_DelayTimeInput
	else if(AutoStateIs(AS_DelayTimeConfirm))
	{
		if(event != ButtonPressedEventMask) return;
		
		if(btnIsStartPressed)
		{
			// YES
			_state = AS_DelayWaiting;
			uiClearSubTitleRow();
			uiSubTitle(STR(To_be_started_in));
			uiMenu(STR(x_x_Quit_Go));
			
			tmSetTimeoutAfter(_delayTime * 15 * 60 * 1000);
			uiRunningTimeStartCountDown(_delayTime * 15 * 60);
			setEventMask(TimeoutEventMask | ButtonPressedEventMask );
			
			#if BluetoothSupported == true
			btReportCurrentStage(StageDelayStart);
			#endif
		}
		else if(btnIsEnterPressed)
		{
			//NO
			backToMain();
		}
		
	} //AS_DelayTimeConfirm
	else if(AutoStateIs(AS_DelayWaiting))
	{
		if(event == ButtonPressedEventMask)
		{
			if(btnIsStartPressed)
			{
				// Quit
				uiRunningTimeStop();
				tmPauseTimer();
				backToMain();
			}
			else if(btnIsEnterPressed)
			{
				//GO
				// cancel timer
				uiRunningTimeStop();
				tmPauseTimer();
				uiClearSettingRow();
				//_state = AS_MashIn;
				autoModeEnterMashIn();				
			}
		}
		else if(event == TimeoutEventMask)
		{
			buzzPlaySound(SoundIdDelayTimeout);
			uiRunningTimeStop();
			uiClearSettingRow();
			autoModeEnterMashIn();
		}
	}//AS_DelayWaiting
#endif
	else if(AutoStateIs(AS_MashIn))
	{
		if(event == TemperatureEventMask)
		{
			if(gCurrentTemperature >=gSettingTemperature)
			{
				// temp reached. ask continue & malt in
				_state = AS_MashInAskContinue;
				
				uiPrompt(STR(TemperatureReached));
				uiMenu(STR(Continue_Yes_x));
				setEventMask(ButtonPressedEventMask);
				
				buzzPlaySoundRepeat(SoundIdWaitUserInteraction);
				
				#if BluetoothSupported == true
				btReportEvent(RemoteEventTemperatureReached);
				#endif

			}
		}//TemperatureEventMask
		else if(event == ButtonPressedEventMask)
		{
			// up/down/pause
			if(btnIsStartPressed)
			{
				autoModePause(0);
			}
			else
			{
				//for up/down
				processAdjustButtons();
			}
		}//ButtonPressedEventMask
	} // endof state AS_MashIn
	else if(AutoStateIs(AS_Pause))
	{
		if(event == ButtonPressedEventMask
			&& btnIsStartPressed)
		{	
			autoModeExitPause();
		} 
	} //AS_Pause
	else if(AutoStateIs(AS_MashInAskContinue))
	{
		if(btnIsStartPressed)
		{
			buzzMute();

			// goto next stage, Mashing or ask MaltADD
			if(readSetting(PS_SkipAddMalt))
			{
				uiClearPrompt();
				// skip Add Malt , enter mashing state
				autoModeEnterMashing();
			}
			else
			{
				pumpOff();
				// ask Add Malt
				uiClearPrompt();
				uiPrompt(STR(Add_Malt));
				uiMenu(STR(Continue_Yes_No));
				_state = AS_AskAddMalt;
				
				#if BluetoothSupported == true
				btReportEvent(RemoteEventAddMalt);
				#endif

			}
		}
	} /// AS_MashInAskContinue
	else if(AutoStateIs(AS_AskAddMalt))
	{
		if(btnIsStartPressed)
		{
			// YES
			uiClearPrompt();
			autoModeEnterMashing();
		}
		else if(btnIsEnterPressed)
		{
			// NO
			// heater & pump might started, so use back to main
			backToMain();
		}
	} // AS_AskAddMalt
	else if(AutoStateIs(AS_Mashing))
	{
		// handle key event together.
		// the same way reached or not.
		if(event == ButtonPressedEventMask)
		{	
			if(_askingSkipMashingStage)
			{
				if(btnIsStartPressed)
				{
					// YES.
					// undone _askingSkipMashingStage
					uiClearPrompt();
					// not necessary , autoModeMashingStageFinished()
					// will print eht menu againuiMenu(STR(Up_Down_Pause_STP));
					// unwind the change
					uiRunningTimeHide(false);
					_askingSkipMashingStage = false;
					tmPauseTimer(); // cancel timer, if any
					// go to next stage
					autoModeMashingStageFinished();
				}
				else if(btnIsEnterPressed)
				{
					// NO
					uiClearPrompt();
					uiMenu(STR(Up_Down_Pause_STP));
					// unwind the change
					uiRunningTimeHide(false);
					_askingSkipMashingStage = false;					
				}
				return;
			}
			// else
			if(btnIsStartPressed)
			{
				// if in 
				if(_mashingTemperatureReached)
				{
					buzzMute();
					autoModePause(tmPauseTimer());
					
				}
				else
				{
					autoModePause(0);
				}
			}
			else if(btnIsEnterPressed)
			{
				// Skip, go to next stage
				if(btnIsEnterLongPressed)  // long pressed is "cover" in normal pressed
				{
					if(_mashingTemperatureReached)
						buzzMute();
					uiRunningTimeHide(true);
					_askingSkipMashingStage = true;
					uiClearPrompt();
					uiPrompt(STR(Go_to_next_step));
					uiMenu(STR(Continue_Yes_No));
				}
			}
			else
			{
				//up, down etc.
				if(_askingSkipMashingStage) return; // ignore

				processAdjustButtons();
			}

		}
		else if(event == PumpRestEventMask)
		{
      Serial.println ( "Mashing: Pump rest event!" );
			//
			if(isPumpRest())
			{
				// into rest
				uiMenu(STR(_Pump_Rest_));
				// stop heat
        Serial.println ( "Mashing: Stopping heat" );
				heatProgramOff();
				//[TODO:] beep
			}
			else
			{
				// back from rest
				uiMenu(STR(Up_Down_Pause_STP));
       Serial.println ( "Mashing: Starting heat" );
				heatOn();
			}
		}
		else // else of PumpRestEvent & Button,
		{
			if(_mashingTemperatureReached)
			{
				if(event == TimeoutEventMask)
				{
					// counting time
					// except button, we care also two timer
					// one for 10  or 5 seconds before time out
					// the other for end of phase timeout
					if(IsAuxTimeout)
					{
						buzzPlaySound(SoundIdCountDown);
					}
					else
					{
						// next stage
           #ifdef DEBUG
            Serial.println ( "Moving to next mashing stage due to timeout" );
           #endif
						autoModeMashingStageFinished();						
					}
				}
			}
			else // of if(_mashingTemperatureReached)
			{
				if(event == TemperatureEventMask)
				{
					// rising temperature
					if(gCurrentTemperature >= gSettingTemperature)
					{
						#if SupportAutoModeRecovery == true
						autoModeRecoveryTimeTracking = true;
						#endif
						
						_mashingTemperatureReached = true;
						unsigned long seconds=(unsigned long)readSetting(PS_StageTimeAddr(_mashingStep)) * 60;
				
						tmSetTimeoutAfter( seconds *1000);
						tmSetAuxTimeoutAfter((seconds-ADVANCE_BEEP_TIME) *1000);
						
						uiRunningTimeStartCountDown(seconds);
					
						buzzPlaySound(SoundIdTemperatureReached);
					
						pumpRestSetEnabled(true);
						
						#if BluetoothSupported == true
						btReportEvent(RemoteEventTemperatureReached);
						#endif

					}
				}
			} 	// end of else if(_mashingTemperatureReached)
		}		// end of temperature and timeout handling
	}//AS_Mashing
	else if(AutoStateIs(AS_AskMaltRemove))
	{
		if(event ==ButtonPressedEventMask)
		{
			if(btnIsStartPressed)
			{
				buzzMute();

				// yes
				uiClearPrompt();
				autoModeEnterBoiling();
			}
			else if(btnIsEnterPressed)
			{
				buzzMute();

				// back to main
				backToMain();
			}
		}
	}//AS_AskMaltRemove
	else if(AutoStateIs(AS_Boiling))
	{
		if(event ==ButtonPressedEventMask)
		{
			processAdjustButtons();
		}
		else if(event ==TimeoutEventMask)
		{
#ifdef AUX_TIMER_HOP
			if(IsAuxTimeout)
			{
				// start next timer to end notice of hop adding	
				if(recoveryTimer)
				{
					uiAutoModeStage(BoilingStage);
					autoModeStartNextHopTimer();
				}
				else
				{
					// start next timer
					autoModeAddHopNotice();
				}
			}
			else
			{
					#if SupportAutoModeRecovery == true
					autoModeRecoveryTimeTracking = false;
					// end of recovery
					updateSetting(PS_AutomodeStarted,0);
					#endif

					// next stage
					heatOff(); // heat OFF
					pumpOff();

          // All done!
					autoModeBrewEnd();
			}

#else
			if(IsAuxTimeout)
			{
				// start next timer to end notice of hop adding	
				uiAutoModeStage(BoilingStage);
			}
			else
			{
				// timeout of hop or boil timer
				if(_numHopToBeAdded == 0)
				{
					// next stage
					heatOff(); // heat OFF
					pumpOff();

          // All done!
          autoModeBrewEnd();

				}
				else
				{
				    // show hop, and start restoreboilingtimer
					autoModeAddHopNotice();
					autoModeStartNextHopTimer();
				}	
			}
#endif
		}
		else // if(event ==TemperatureMask)
		{
			togglePwmInput();

			if(gCurrentTemperature >= gBoilStageTemperature)
			{
				if(_isBoilTempReached == false)
				{
				
					#if SupportAutoModeRecovery == true
					autoModeRecoveryTimeTracking = true;
					#endif
				
					#if BluetoothSupported == true
					btReportEvent(RemoteEventTemperatureReached);
					#endif

					_isBoilTempReached=true;

					//buzz temperature reach first
					// because later "add hop" buzz may interrupt
					// it
					buzzPlaySound(SoundIdBoil);
					// start counting down
					byte boilTime=readSetting(PS_BoilTime);
					uiRunningTimeStartCountDown((unsigned long)boilTime *60);
					// start hop & boiling out timer
					autoModeStartBoilingTimer();				
				}
			}
		}
	} //AS_Boiling
	else if(AutoStateIs(AS_Finished))
	{
		if(event == TimeoutEventMask)
		{
      #ifdef DEBUG
        Serial.println ( "Returning to main" );
      #endif
			buzzMute();
			backToMain();
		}
	}//AS_Finished
} // end of autoModeEventHandler


// *************************
//*  Main Screen
// *************************

void mainSetup(void)
{
  uiReset();
	uiClearScreen();

	uiTitle(STR(welcome));
	uiMenu(STR(Manual_Auto_Setup));
  uiTempDisplaySetPosition(TemperatureMainScreenPosition);
    
    #if BluetoothSupported == true
    btReportCurrentStage(StageIdleScreen);
    #endif
}

// main screen
//  -down button-> manual
//  -start button->auto
//  -ener button ->setup 

void mainEventHandler(byte event)
{
	if(btnIsEnterPressed)
	{
		switchApplication(SETUP_SCREEN);

	}
  	else if(btnIsDownPressed)
  	{
      	switchApplication(MANUAL_MODE_SCREEN);
  	}
  	else if(btnIsStartPressed)
  	{
      	switchApplication(AUTO_MODE_SCREEN);
  	}
  
}

// *************************
//*  Screen switch
// *************************


const CScreen *currentScreen;
void switchApplication(byte screenId)
{
	currentScreen=allScreens+screenId;
	//turn off temperature update by default, let those who want turn it on
//	uiClearSettingRow();
	uiRunningTimeStop();
	uiTempDisplayHide();
	
	//default event is keyboard
	setEventMask(ButtonPressedEventMask);

	(* currentScreen->setup)();

}


void backToMain(void)
{
	// turn pump, heat and buzzer off
	heatOff();
	pumpOff();
  buzzMute();
	//
	switchApplication(MAIN_SCREEN);
}
// *************************
//*  Main procedure
// *************************

void setup() {
  // put your setup code here, to run once:
#if SerialDebug == true
Serial.begin(115200);

#endif	
	//[TODO:] move them to individual initilization code?
	pinMode (HeatControlPin, OUTPUT);
	pinMode (PumpControlPin, OUTPUT);
	pinMode (BuzzControlPin, OUTPUT);
//	gIsUseFahrenheit = readSetting(PS_TempUnit);

	tmInitialize();

	btnInitialize();

	tpInitialize();

	uiInitialize();

	heatInitialize();

	pumpInitialize();

	switchApplication(MAIN_SCREEN);

#if BluetoothSupported == true
	btInitialize();
#endif

}

#if SerialDebug == true
//#include "serialdebug.h"
#endif

//*********************************************************************
//*********************************************************************




void loop() {

	// Process Events
	//    button, temperature, time
	//  Event: BUTTON,  TEMP Reach, TimeOut.

    
  
  #ifdef DEBUG_TIMING
    static double lastmillis = 0;
    static double elapsed = 0;
    static double minelapsed = 100000;
    static double maxelapsed = 0;
    static double totelapsed = 0;      
    static double aveelapsed = 0;  
    static double count = 0;    
    if ( lastmillis > 0 ) {
      elapsed = millis() - lastmillis;
      if ( elapsed < minelapsed ) {
        minelapsed = elapsed;
      }

      if ( elapsed > maxelapsed ) {
        maxelapsed = elapsed;
      }

      totelapsed += elapsed;
      aveelapsed = totelapsed / ++count;

      Serial.print ( "Elapsed times, last : " );
      Serial.print ( elapsed );
      Serial.print ( " Min: " );      
      Serial.print ( minelapsed );
      Serial.print ( " Max: " );
      Serial.print ( maxelapsed );    
      Serial.print ( " Ave: " );
      Serial.println ( aveelapsed );         
    }
    lastmillis = millis();
  #endif
	tpReadTemperature();
	
	// let the handler compare the temperatures themselves if they request it.
	if(_currentEventMask & TemperatureEventMask)
		(*currentScreen->eventHandler)(TemperatureEventMask);

	if(tmTiming())
	{
		if(_currentEventMask & TimeoutEventMask) {
      #ifdef DEBUG
        Serial.println ( "Main loop timeout !" );
      #endif
			(*currentScreen->eventHandler)(TimeoutEventMask);	
    }      
	}
	
	if(btnReadButtons()) 
	{
		if(isExactButtonsPressed(ButtonUpMask | ButtonDownMask))
			backToMain();
		else
		{
			// if(_currentEventMask & ButtonPressedEventMask) button event is always handled in all 
			// screen!
			(*currentScreen->eventHandler)(ButtonPressedEventMask);
		}
	}
	
	if(pumpRestEvent())
	{
		if(_currentEventMask & PumpRestEventMask)
			(*currentScreen->eventHandler)(PumpRestEventMask);
	}
#if SupportAutoModeRecovery == true	
	if(autoModeRecoveryTimeTracking)
		autoModeTrackRecoveryTime();
#endif	
	//update Time & temperature 
	uiDisplayTemperatureAndRunningTime();
	
	//	
	// threads
	heatThread( AutoStateIs(AS_Boiling) );
	pumpThread();
	buzzThread();

#if BluetoothSupported == true	
	btThread();
#endif	
	// handler state machine

#if SerialDebug == true
//	rmReceive();
#endif	
}// end of loop();

