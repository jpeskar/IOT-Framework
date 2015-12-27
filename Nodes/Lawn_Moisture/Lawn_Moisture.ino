
#include <RFM69.h>			//Library by Felix Rusu - felix@lowpowerlab.com
//#include <WirelessHEX69.h>  //Library by Felix Rusu - felix@lowpowerlab.com
#include <SPI.h>
#include <SPIFlash.h>       //Library by Felix Rusu - felix@lowpowerlab.com
#include <OneWire.h>
#include <LowPower.h>       //https://github.com/rocketscream/Low-Power
#include <EEPROM.h>
#include <Flash.h>



// Radio defines and Variable
uint8_t	 NodeID;
uint8_t  NetworkID; //This is the same on all radios
uint8_t  ProgramNetworkID;
uint8_t  GatewayID;
uint8_t	 ProgramGatewayID;
char	   KEY[17] = {"GreenGrassPeskar"}; //has to be same 16 characters/bytes on all nodes, not more not less!
uint8_t  ackTime;  // # of ms to wait for an ack
uint8_t  retries;
uint8_t  MAC[8];
uint8_t  SleepCycles; //number of 8 second sleep cycles, 75 cycles = 10 min
bool	   PromiscuousMode;
uint8_t  NodeType;
uint8_t  SoftwareVer;
uint8_t	 HardwareVer;
uint8_t	 Number18S20;
bool 	   RadioEcho = false;
bool	   Verbose = true;
char     InputBuffer[64];
char     SendBuffer[64];
uint8_t  SendBufferLength = 0;
uint8_t	 InputBufferLength = 0;


#define SERIAL_BAUD 115200
#define FREQUENCY   RF69_433MHZ // 433Mhz radio
//Pin Defines
#define LED (9)          //Monetio LED
#define MoisturePWR A3   //Power Pin for moisture sensor
#define Temperature 7    //Digital Onewire for Temperature
#define MoistureADC A0   //Analog ADC Pin for moisture sensor
#define BatteryADC  A1

volatile int sleep_count = 0;

//Class Instances
SPIFlash flash(8, 0xEF30); //EF40 for 16mbit windbond chip
RFM69 radio;
OneWire ds(Temperature);  //One Wire Temp Sensor

void setup(void){
	Serial.begin(SERIAL_BAUD);
	delay(10);
	pinMode (LED, OUTPUT);
	pinMode(MoisturePWR, OUTPUT);
	pinMode(BatteryADC, INPUT);
  if(!flash.initialize()){
    //ToDo: real error handler here
    RadioEcho = true;
    Verbose = true;
    CreateSendString(SendBuffer, SendBufferLength,F("External EEPROM Error!"), "");
    SerialSendwRadioEcho(SendBuffer);
  }
  GetNodeIdentity();
  
	/* *************Temp Assignments************************************* */
	NodeID = 251;    //Temp ID assignment
	SleepCycles = 2;//Temp sleep cycles assignment
	NetworkID=3; //Temp variable assignment
	ProgramNetworkID =10; //Temp variable assignment
	GatewayID = 0;
	ProgramGatewayID =0;
  ackTime = 30;
/* ******************************************************************** */
  
	InitializeRadio(NodeID,NetworkID, true);
}

void loop(void) {
	float tempFloat;
	char  tempChar[10];
	uint32_t Time = millis();
	

	if(Serial.available()> 0){
    SerialStreamCapture();
    ProcessInputString(InputBuffer, InputBufferLength);
	}
		
		Serial.print("NODEID"); Serial.print(NodeID);
	
	if(NodeID == 250 && GatewayID == 0){
	 GatewayID = FindGateway(NodeID, NetworkID, ackTime);
		if(GatewayID !=0)
			EEPROM.write(3, GatewayID);
			if(Verbose || RadioEcho){
          CreateSendString(SendBuffer, SendBufferLength,F("Selected GatewayID:"), GatewayID);
          SerialSendwRadioEcho(SendBuffer); 
        }
		else {
			if (Verbose || RadioEcho){
        CreateSendString(SendBuffer, SendBufferLength,F("ERROR on Finding Gateway"), "");
        SerialSendwRadioEcho(SendBuffer);
      }
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
		}
			
	}	
	else if(NodeID == 250 && ProgramGatewayID == 0){
		ProgramGatewayID= FindGateway(NodeID,ProgramNetworkID,ackTime);
		if (ProgramGatewayID != 0) {
			EEPROM.write(29, ProgramGatewayID);
        if(Verbose || RadioEcho){
          CreateSendString(SendBuffer, SendBufferLength,F("Selected Prog GatewayID:"), ProgramGatewayID);
          SerialSendwRadioEcho(SendBuffer); 
        }
		} 
		else {
      if (Verbose || RadioEcho){
        CreateSendString(SendBuffer, SendBufferLength,F("ERROR on Finding Prog Gateway"), "");
        SerialSendwRadioEcho(SendBuffer);
      }
		LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
		}
	}
	
	else if(NodeID == 250 && GatewayID != 0 && ProgramGatewayID != 0)
		GetNodeParameters();

//Main Loop after Node Initilization
	else if(sleep_count == SleepCycles || SleepCycles == 0 && NodeID != 250) {

		sleep_count = 0;
		//SerialSendwRadioEcho (GatewayID, "Awake");

		StartTempMeasurements (); //Start Temp Conversions
		digitalWrite(A3, HIGH); //Turn on moisture Power
		LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_ON); //sleep to give temp measurement time
														//keep radio on

		//Battery Voltage Content Section
		tempFloat = BatteryMeasurement() / 1000;
		dtostrf(tempFloat, 4, 2, tempChar);
		CreateSendString(SendBuffer, SendBufferLength,"V:", tempChar);
		
		//Volumetric Water Content Section
		tempFloat = MoistureMeasurement();
		dtostrf(tempFloat, 3, 1, tempChar);
		digitalWrite(A3, LOW); //Turn off moisture Power
		CreateSendString(SendBuffer, SendBufferLength,"VWC:", tempChar);
		
		//Soil Temperature in deg c
		tempFloat = TempMeasurement(0);
		dtostrf(tempFloat, 4, 2, tempChar);
		CreateSendString(SendBuffer, SendBufferLength,"LTC:", tempChar);
				
		InitializeRadio(NodeID,NetworkID, true); // Radio must be initialized after sleep?
		SerialSendwRadioEcho (SendBuffer);
		if (radio.sendWithRetry(GatewayID, &SendBuffer, SendBufferLength, retries, ackTime)){
      //TODO:Check if Data in ACK
			if(Verbose || RadioEcho) {
			  CreateSendString(SendBuffer, SendBufferLength,F("Ack Recieved"), "");
        SerialSendwRadioEcho(SendBuffer);
			}
      
		}
		else{
			SendString(F("Nothing"));
      CreateSendString(SendBuffer, SendBufferLength,F("Nothing"), "");
      SerialSendwRadioEcho(SendBuffer);
		}
		
	}
delay(1000);
	radio.sleep();
  if(Verbose || RadioEcho) {
    CreateSendString(SendBuffer, SendBufferLength,F("Sleep Count: "), sleep_count);
    SerialSendwRadioEcho(SendBuffer);
	  Serial.flush();
  }
	LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
	sleep_count ++;
}


/* **************************************************
   Name:       GetNodeIdentity

   Accpets:    None

   Returns:     None 

   Description: This function reads the nodes parameters from both internal
                and external EEPROM and writes the values to global variables
 ************************************************** */
void GetNodeIdentity(){
  if (Verbose || RadioEcho) {
	  CreateSendString(SendBuffer, SendBufferLength,F("Getting Parameters"), "");
    SerialSendwRadioEcho(SendBuffer);
  }

	//Read From Internal EEPROM
	SoftwareVer = EEPROM.read(0);
	NodeID 		= EEPROM.read(1);
	NetworkID 	= EEPROM.read(2);
	GatewayID	= EEPROM.read(3);
	ackTime 	= EEPROM.read(25);
	retries		= EEPROM.read(26);
	SleepCycles = EEPROM.read(27);
	ProgramNetworkID = EEPROM.read(28);
	ProgramGatewayID = EEPROM.read(29);
	

	for (byte i = 0; i < 16; i++){
		//KEY[i] = EEPROM.read(i + 4);
	}

 for (byte i = 0; i <8; i++) {
    MAC[i] = EEPROM.read(i+ 30); 
 }

	PromiscuousMode = false; //set to 'true' to sniff all packets on the same network

	//Read From External EEPROM. These values will not change unless node is 
	//rebuilt with new hardware.
	NodeType	= flash.readByte(32512);
	HardwareVer = flash.readByte(32513);
	Number18S20 = flash.readByte(32514);
  if(Verbose || RadioEcho){
	  CreateSendString(SendBuffer, SendBufferLength,F( "Parameters Loaded"), "");
    SerialSendwRadioEcho(SendBuffer);
 }
}


/* **************************************************
   Name:        NodeDHCP

   Accpets:    Encript , used to set radio encrption on

   Returns:     GatewayID, 

   Parameters:  NodeID, user Settable
                NetworkID, Same across all radios

   Description: This function takes in the NodeID, and NetworkID  and uses 
                them to brodcast for a gateway on a given NetworkID.
                The gateways will send back a message with the sender RSSI. 
                The gateay with the highest RSSI wins! Best Gateway wins and is returned. A 
                0 is returned if no gateway is found.
 ************************************************** */
uint8_t FindGateway(uint8_t nodeID,  uint8_t networkID, uint8_t ackTime) {
uint8_t     tempGatewayID[2] ={0,0};
int8_t      tempRSSI[2] = {0,0};
uint8_t     tempAckTime[2] = {0,0};
uint32_t	  sendTime;
uint8_t     gatewayCount =0;

	sendTime = millis();
	radio.setNetwork(networkID); //Set Nework to Desired NetworkID
  if(Verbose || RadioEcho){
    CreateSendString(SendBuffer, SendBufferLength,F("Searching"), "");
    SerialSendwRadioEcho(SendBuffer);
 }
	radio.send(255,"",1,false);  //broadcast to all Nodes on Network
	while(millis() - sendTime < ackTime*100) {
		if(radio.receiveDone()) {
			tempGatewayID[1] = radio.SENDERID;
			tempRSSI[1] = radio.RSSI;
      tempAckTime[1] = millis()- sendTime;
			if(radio.ACKRequested())
				radio.sendACK();
			SendString(F("Got One!"));
			Serial.print("[RX_RSSI:");delay(10);Serial.print(radio.RSSI);delay(10);Serial.println("]");delay(10);
			Serial.print("GatewayID:");delay(10);Serial.println(tempGatewayID[1]);
			gatewayCount ++;
			//Compare and replace of highest value into the first position of the array
			if(tempRSSI[1] > tempRSSI[0] || tempRSSI[0]==0){
				tempRSSI[0]= tempRSSI[1];
				tempGatewayID[0] = tempGatewayID[1];
        tempAckTime[0] = tempAckTime[1];
			}
		}
	}
if (tempGatewayID[0] != 0) 
	return tempGatewayID[0];

else
	return 0;
}


/* **************************************************
   Name:        GetNodeParameters

   Accepts:    Encrypt , used to set radio encryption on

   Returns:     boolean, true means a successful NodeID capture

   Parameters:  FREQUENCY, Hard programmed into device
                NodeID, user Settable
                NetworkID, Same across all radios
                KEY, Used for Radio System

   Description: This function calls upon the programming gateway which is 
   5Mhz above the normal operating frequency and passes node type, hardware version
   software version and external EEPROM address. The mcu sleeps until a message is
   received back with. A function call to decode the message is called. A boolean 
   value is passed out of the function to indicate a successful capture.
 ************************************************** */
//Change to programming Network
//shift frequency +5Mhz
//send node Identifiers
// recieve nodeID
bool GetNodeParameters(){

  radio.setFrequency(438000000);  	//Change frequency to programing network
  radio.setNetwork(ProgramNetworkID);    			//Change Network to Programming Network
  radio.send(ProgramGatewayID, &SendBuffer, SendBufferLength);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_ON);	//Sleep waiting for	data back
  if(radio.receiveDone()) {
		for (byte i = 0; i < radio.DATALEN; i++)
		{
			InputBuffer[i] = (char)radio.DATA[i];
			InputBufferLength += 1;
		}

		if (radio.ACKRequested())
			radio.sendACK();
		
		ProcessInputString(InputBuffer, InputBufferLength);
   
   if(Verbose){
    CreateSendString(SendBuffer, SendBufferLength, F("Node Identity Sucessfully Obtained"), "");
    SerialSendwRadioEcho(SendBuffer);
   }
		
	 
	NodeID = 65;
	radio.setFrequency(433000000);
	radio.setNetwork(NetworkID);
  if(Verbose || RadioEcho){
    CreateSendString(SendBuffer, SendBufferLength,F("Initilization Complete"), "");
    SerialSendwRadioEcho(SendBuffer);
  }
	
  }
  else{
    //Need to add code if there is no response after x seconds
      }
	if(NodeID != 250)
		return true;
	else
		return false;
}

void StartTempMeasurements ()
{
	byte DS18S20[8];
	for (int i = 0; i < Number18S20; i++) {
		flash.readBytes(32515 + (8 * i), &DS18S20, 8);
		ds.reset();			//reset the one wire bus
		ds.select(DS18S20);	//Select a Sensor
		ds.write(0x44);     // start conversion
	}
}

float TempMeasurement(byte SensorNumber)
{
	byte present = 0;
	byte type_s;
	byte addr[8];
	byte data[12];
	float celsius;


	flash.readBytes(32515 + (8 * SensorNumber), &addr, 8);
	for(int i = 0; i < 8; i++) {
		Serial.print(addr[i], HEX);
	}
	Serial.print("");
	present = ds.reset();
	ds.select(addr);
	ds.write(0xBE);         // Read Scratchpad


	for (byte  i = 0; i < 9; i++) {
		data[i] = ds.read();
	}

	celsius = ((data[1] << 8) + data[0]) * 0.0625;
	Serial.println(celsius);
	return celsius;
}

/* **************************************************
   Name:        InitializeRadio

   Accpets:		Encript , used to set radio encrption on
   				    NodeID
   				    NetworkID
              Key
  
   Returns:   Nothing.
   

   Parameters:  FREQUENCY, Hardprogrammed into device
   			        NodeID, user Settable
   			        NetworkID, Same across all radios that have a common function. IE: outdoor sensors
	  		        KEY, Same across all radios

   Description: This function accepts the nodeID, the networkID, the network key and the encritpion flag
                and initializes the radio class and other member functions.
 ************************************************** */
void InitializeRadio (uint8_t nodeID, uint8_t networkID, bool Encript)
{
	radio.initialize(FREQUENCY, nodeID, networkID); //initialize Radio
	if (Encript == true)
	{
		radio.encrypt(KEY);
	}
	radio.promiscuous(PromiscuousMode);

}

void adjustFrequency (int sensorNumber, int frequency) {
  float AdjustFreq;
  float t;   //Temperature in deg C

  t = TempMeasurement(1);
  AdjustFreq = ((123 * t * t * t) + (123 * t * t) + (123 * t) + 123); //our polynomial function
  radio.setFrequency(frequency - AdjustFreq);
}

/* **************************************************
   Name:        MoistureMeasurement

   Returns:     floating point number as % of water

   Parameters: 	None

   Description: This function calls the ReadVcc function to determine actual 
   				voltage and uses that along with an adc voltage conversion 
   				of the sensor output. The voltage conversion and the ADC are
   				used in a calculation of the volumeritic water content.
 ************************************************** */
float MoistureMeasurement() {
  float ADC;
  float moisture;
  float voltage;
  uint16_t realVcc;
  for (int i = 0; i < 6; i++) {//read 6 ADC values toss the first
    if (i > 0) 
      ADC += analogRead(MoistureADC);
  }
  ADC /= 5;
  realVcc = ReadVcc();
  voltage = (ADC * realVcc) / 1023.0;

  if (voltage < 1.1) {
    moisture = 10 * voltage - 1;
	if (moisture < 0)
	  moisture = 0;
  }
  else if (voltage < 1.3) 
	moisture = 25 * voltage - 17.5;
  else if (voltage < 1.82) 
    moisture = 48.08 * voltage - 47.5;
  else if (voltage < 2.2) 
	moisture = 26.32 * voltage - 7.89;
  else
	moisture = 50;
return moisture;
}

/* **************************************************
   Name:        Blink

   Returns:     Nothing.

   Parameters:  Ontime: Time the LED is on in milliseconds
   			    OffTime: Time the LED is off in milliseconds
   			    PinNumber: Pin the LED is attached to
	  		    BlinkNumber: Number of on/off blinks

   Description: This function turns on and off an LED defined by the parameters above.

 ************************************************** */
void Blink (int onTime, int offTime, byte pinNumber, int blinkNumber)
{
  int i = 0;

  while (i < blinkNumber) {
    digitalWrite(pinNumber, HIGH);
	delay(onTime);
	digitalWrite(pinNumber, LOW);
	delay(offTime);
	i++;
  }
}


void SerialSendwRadioEcho (char *payload)
{
  uint8_t payloadLength;

  payloadLength = strlen(payload);
  if(Verbose == true && RadioEcho == true){
    Serial.print(F("Sending To: "));
	  Serial.println(GatewayID);
	  Serial.println(payload);
	  if (radio.sendWithRetry(GatewayID, payload, payloadLength, retries, ackTime))
	    Serial.print(F(" ok!"));
	  else Serial.print(F(" nothing..."));
  }
  else if (Verbose == true) {
	for (int i = 0; i < payloadLength; i++) {
	  Serial.print(payload[i]);
	  delay(10);
		}
  Serial.println("");
  }
  else if (RadioEcho == true) {
    if(radio.sendWithRetry(GatewayID, payload, payloadLength, retries, ackTime));
	  radio.send(GatewayID, "ACK Recieved", 13);
  }
  ClearBuffer(payload);
  SendBufferLength = 0;
}

void CreateSendString(char *buffer, uint8_t &sendBufferLength, const char *tag, char *value) {

  sendBufferLength += sprintf(&buffer[sendBufferLength],"%s",tag);
  sendBufferLength += sprintf(&buffer[sendBufferLength],"%s",value);
}


void CreateSendString(char *buffer, uint8_t &sendBufferLength, const char *tag, uint8_t value) {
  sendBufferLength += sprintf(&buffer[sendBufferLength],tag);
  sendBufferLength += sprintf(&buffer[sendBufferLength],"%d",value);
}

void CreateSendString(char *buffer, uint8_t &sendBufferLength, byte hexValue) {
  sendBufferLength += sprintf(&buffer[sendBufferLength],"%02X",hexValue);
}

void CreateSendString(char *buffer, uint8_t &sendBufferLength, const __FlashStringHelper* message, int8_t value) {
  sendBufferLength += sprintf_P(&buffer[sendBufferLength], PSTR("%S") , message);
  sendBufferLength += sprintf(&buffer[sendBufferLength],"%d",value);
}
void CreateSendString(char *buffer, uint8_t &sendBufferLength, const __FlashStringHelper* message, char *value) {
  sendBufferLength += sprintf_P(&buffer[sendBufferLength], PSTR("%S") , message);
  sendBufferLength += sprintf(&buffer[sendBufferLength],"%s",value);
}

void SerialStreamCapture() {
	char temp;
	uint8_t i = 0;
		
	while(Serial.available()>0){
		temp= Serial.read();
		if(temp != '\n' || temp !='\r'){ //TODO
			InputBuffer[InputBufferLength] = temp;
			InputBufferLength++;
			delay(20);
		}
	}
	InputBuffer[InputBufferLength] = '\0'; //Null Terminate the string.

  if(Verbose || RadioEcho){
	  CreateSendString(SendBuffer,SendBufferLength, F("InputBufferLength:"),InputBufferLength);
	  CreateSendString(SendBuffer,SendBufferLength,F("InputBuffer:"),InputBuffer);
    SerialSendwRadioEcho(SendBuffer);
  }
	
}


/* **************************************************
   Name:        ProcessInputString

   Accpets:		

   Returns:     Nothing

   Parameters:

   Description: This function takes in the Input buffer by refrence and 
   				parses the data out into parameters and values. The values are stored
   				into the Global variables and the internal EEPROM as well. 
************************************************** */
void ProcessInputString(char *inputBuffer, byte inputBufferLength)
{
	char parameter[5];
	uint8_t parameterLength =0;
	char value[10];
	uint8_t valueLength =0;
	bool inputFlag;
	
	for (int i = 0; i < inputBufferLength; i++)
	{
		if (inputBuffer[i] != ';' && inputBuffer[i] != '\n' && inputBuffer[i] != '\0' )
		{
			if(inputBuffer[i] != '\n' && inputBuffer[i] != '\0'){	
		  		if(inputBuffer[i] != ':'){
		  			if(inputFlag == true){
		  				parameter[parameterLength] = inputBuffer[i];
		  				parameterLength ++;
		  			}
		  			else{
		  				value[valueLength] = inputBuffer[i];
		  				valueLength ++;
		  			}
		  		}
			  	else {
			  		inputFlag = false;
			  	}
			}
		}
		else{
		  inputFlag = true;
		  parameter[parameterLength] = '\0';
		  value[valueLength]= '\0';
		  Serial.println(parameter);
		  Serial.println(value);
		  
		  if(strcmp(parameter, "NI") == 0) {
		  	NodeID = atoi(value);
		  	EEPROM.write(1, NodeID);
		  }
		 else if(strcmp(parameter,"RT") == 0){
		 	retries = atoi(value);
		 	EEPROM.write(26, retries);
		 }
		else if(strcmp(parameter, "ACK") == 0){
			ackTime = atoi(value);
			EEPROM.write(25, ackTime);
		}
		else if(strcmp(parameter, "SC") == 0){
			SleepCycles = atoi(value);
			EEPROM.write(27, SleepCycles);
			
		}
		else if(strcmp(parameter,"VM") == 0){
			if (value == "1")
				Verbose = true;
			else 
				Verbose = false;
				
		}
	else if(strcmp(parameter, "RE")  == 0){
			if (value == "1")
				RadioEcho = true;
			else 
				RadioEcho = false;
			 
		}
	else
		SendString(F("Invalid Parse"));
		
		  valueLength =0;  //reset length
		  parameterLength =0;
		  ClearBuffer(parameter); //clear buffers
		  ClearBuffer(value);
		}
	} // end of Message excape to main
}

/* **************************************************
   Name:        BatteryMeasurement

   Returns:     floating point voltage as mV

   Parameters:   None

   Description: This function call the function to determine the actual voltage.
          An ADC is run on a voltage divider and is used with the actual
          chip voltage to conversion is used calculate the voltage of the battery.
 ************************************************** */
float BatteryMeasurement(){
  float    voltage;
  uint16_t ADC;
  uint16_t realVcc;

  realVcc = ReadVcc();

  for (int i = 0; i < 6; i++){ //read 6 ADC values toss the first
  if (i > 0) 
    ADC += analogRead(BatteryADC);
  }

  ADC /= 5;
  voltage = ((float)ADC * realVcc * 3.128) / 1023.0;
return voltage;
}

/* **************************************************
   Name:        ReadVcc

   Accpets:		Nothing

   Returns:     returns an unsigned integer of milivolts

   Parameters:

   Description: This function uses the internal 1.1 voltage refrence and compares it to
   				the vcc of the chip. It uses the difference to calculate the real Vcc. Very
   				useful in determining accurate ADC's. Not my function... found it on the web.
************************************************** */
uint16_t ReadVcc() {
	uint16_t result;
	uint8_t  saveADMUX;

	saveADMUX = ADMUX;

	// Read 1.1V reference against AVcc
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Convert
	while (bit_is_set(ADCSRA, ADSC));
	result = ADCL;
	result |= ADCH << 8;
	result = 1126400L / result; // Back-calculate AVcc in mV

	ADMUX = saveADMUX;  // restore it on exit...
	return result;
}

void ClearBuffer (char *buffer){
  uint8_t buffLength = strlen(buffer);

  for (int i = 0; i <  buffLength; i++) {
    buffer[i] = 0;
  }
}

void SendString(const __FlashStringHelper* message)
{
  if(Verbose){
	  sprintf_P(SendBuffer, PSTR("%S") , message);
	  Serial.println(SendBuffer);
  }
  
}

void CreateHarwareIdentity(char *sendBuffer, uint8_t &sendBufferLength, uint8_t nodeType, uint8_t hwVer, uint8_t swVer, uint8_t *mac){
  flash.readUniqueId();  
  for (int j = 0; j < 8; j++) {
    CreateSendString(sendBuffer, sendBufferLength, mac[j]);
    if (j <7){
      sendBuffer[sendBufferLength]= ':';    
      sendBufferLength ++;
     }
   }
  CreateSendString(sendBuffer, sendBufferLength,"HT:", nodeType);
  CreateSendString(sendBuffer, sendBufferLength,"HV:", hwVer);
  CreateSendString(sendBuffer, sendBufferLength,"SV:", swVer); 
}
int freeRam ()
{
	extern int __heap_start, *__brkval;
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

;
