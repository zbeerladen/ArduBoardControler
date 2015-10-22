/*
 * ArduBoardControler_Rx.ino
 *
 * Created: 9/5/2015 10:10:26 PM
 * Author: AC
 */ 


#include <SPI.h>

#include "Config.h"

//Using the nRF34 library from https://github.com/TMRh20/RF24
#include <nRF24L01.h>
#include "RF24.h"
#include "printf.h"

//Library for VESC UART
#include "VescUart.h"
#include "datatypes.h"

RF24 radio(CEPIN,CSPIN);

//Define variables for remote
remotePackage remPack;
bool recOK = true;
uint32_t lastTimeReceived = 0;

struct bldcMeasure VescMeasuredValues;


int8_t persXJoy = 0;

void setup()
{
	
	#ifdef DEBUG
	  DEBUGSERIAL.begin(115200);
	#endif
	  //Initial for Radio
	  SERIALIO.begin(115200);
	  delay(1000);
	  Serial.println("Nrf24L01 Receiver Starting");
	  radio.begin();
	  radio.enableAckPayload();
	  radio.enableDynamicPayloads();
	  radio.openReadingPipe(1,pipe);

	  radio.startListening();
	  //ToDo: leads to restart!?
	//#ifdef DEBUG
	//
	//radio.printDetails();
	//
	//#endif

	// For initial start 
	
		remPack.valXJoy			= 512; //middle Position
		remPack.valYJoy			= 512;
		remPack.valLowerButton	= 0;
		remPack.valLowerButton	= 0;
}

void loop()
{
	//Getting Values from Vesc over UART

	if (VescUartGetValue(VescMeasuredValues)) {
	//	SerialPrint(VescMeasuredValues);
	}
	else
	{
		Serial.println("Failed to get data from UART!");
	}

	//writing package to TX in AckPayload
	//data will be send with next acknowledgement to TX

	radio.writeAckPayload(pipe, &VescMeasuredValues, sizeof(VescMeasuredValues));
	
	//Get data from TX	
	while (radio.available())
	{
		radio.read(&remPack, sizeof(remPack));
		recOK = true;
	}
	

	uint32_t now = millis();

	if (recOK == true)
	{
		lastTimeReceived = millis();
		
#ifdef DEBUG
		DEBUGSERIAL.println("Received TX successfully!");
		DEBUGSERIAL.println("Received package: ");
		DEBUGSERIAL.print("valXJoy = "); DEBUGSERIAL.print(remPack.valXJoy); DEBUGSERIAL.print(" valYJoy = "); DEBUGSERIAL.println(remPack.valYJoy);
		DEBUGSERIAL.print("LowerButton = "); DEBUGSERIAL.print(remPack.valLowerButton); DEBUGSERIAL.print(" UpperButton = "); DEBUGSERIAL.println(remPack.valUpperButton);
		DEBUGSERIAL.print("Calcx: "); DEBUGSERIAL.print(((float)persXJoy / 100) * 40.0);
		recOK = false;
#endif
	}
	//Check if package were received within timeout
	else if ((now - lastTimeReceived) > TIMEOUTMAX)
		{
			remPack.valXJoy = 128; //middle Position
			remPack.valYJoy = 128;
			remPack.valLowerButton = 0;
			remPack.valLowerButton = 0;
			DEBUGSERIAL.println("TX-signal lost!!");
		}
// Nunchuck mode	
	VescUartSetNunchukValues(remPack);


#ifdef SET_CURRENT_CONTROL
	//Read the remote controls and control Vesc
//Read the x-joystick and controls motor current and break

//transform values read to int

	persXJoy = map(remPack.valXJoy, 0, 255, -100, 100);

	if (persXJoy > DEADBAND_POS)
	{
		VescUartSetCurrent(((float)persXJoy / 100) * 40.0);
		Serial.println("Throttle");
	}
	else if (persXJoy < DEADBAND_NEG)
	{
		VescUartSetCurrentBrake(((float)persXJoy / 100) * -3.0);
		Serial.println("Break");
	}

	else
	{
		VescUartSetCurrent(0.0);
		VescUartSetCurrentBrake(0.0);
	}
#endif // SET_CURRENT_CONTROL


					

	
	//delay(1000);
	
}
