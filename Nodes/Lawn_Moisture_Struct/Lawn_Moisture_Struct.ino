
#include <RFM69.h>			//Library by Felix Rusu - felix@lowpowerlab.com
//#include <WirelessHEX69.h>  //Library by Felix Rusu - felix@lowpowerlab.com
#include <SPI.h>
#include <SPIFlash.h>       //Library by Felix Rusu - felix@lowpowerlab.com
#include <OneWire.h>
#include <LowPower.h>       //https://github.com/rocketscream/Low-Power
#include <EEPROM.h>

#include <NodeMessageStruct.h>
#include <NodeStruct.h>
#include <Battery.h>


// Radio defines and Variable
uint8_t	 NodeID;
uint8_t  NetworkID; //This is the same on all radios
uint8_t  ProgramNetworkID;
uint8_t  GatewayID;
uint8_t	 ProgramGatewayID;
char	   KEY[17] = {"GreenGrassPeskar"}; //has to be same 16 characters/bytes on all nodes, not more not less!
uint8_t  AckTime;  // # of ms to wait for an ack
uint8_t  Retries;
uint8_t  SleepCycles; //number of 8 second sleep cycles, 75 cycles = 10 min
uint16_t SleepTime;
bool	   PromiscuousMode;
uint8_t	 Number18S20;
bool 	   RadioEcho = false;
bool	   Verbose = true;
char     InputBuffer[64];
uint8_t	 InputBufferLength = 0;

#define SERIAL_BAUD 115200
#define FREQUENCY   RF69_433MHZ // 433Mhz radio
//Pin Defines
#define LED (9)          //Monetio LED
#define MoisturePWR A3   //Power Pin for moisture sensor
#define Temperature 7    //Digital Onewire for Temperature
#define MoistureADC 14   //A0 Analog ADC Pin for moisture sensor
#define BatteryADC  15   //A1 Analog ADC Pin for battery
volatile int sleep_count = 0;

//Class Instances
SPIFlash flash(8, 0xEF30); //EF40 for 16mbit windbond chip
RFM69 radio;
OneWire ds(Temperature);  //One Wire Temp Sensor

//Stucture Instances
LawnPacket lawnData;



void setup(void){
	Serial.begin(SERIAL_BAUD);
	delay(10);
	pinMode (LED, OUTPUT);
	pinMode(MoisturePWR, OUTPUT);
	pinMode(BatteryADC, INPUT);
  if(!flash.initialize()){
    //ToDo: real error handler here
    message.SendMessage(GatewayID ,F("External EEPROM Error!"),ERRORCODE, Verbose, true);
  }
  
  GetNodeIdentity();

	/* *************Temp Assignments************************************* */
	NodeID = 251;    //Temp ID assignment
	SleepCycles = 2;//Temp sleep cycles assignment
	NetworkID=3; //Temp variable assignment
	ProgramNetworkID =10; //Temp variable assignment
	GatewayID = 0;
	ProgramGatewayID =0;
  uint8_t NodeType = 3;
  AckTime = 30;
/* ******************************************************************** */
  
	InitializeRadio(NodeID,NetworkID, true);
}

void loop(void) {
	float tempFloat;
	char  tempChar[10];
	uint32_t Time = millis();
	

	if(Serial.available()> 0){
    message.SerialStreamCapture(InputBuffer, InputBufferLength);
    ProcessInputString(InputBuffer, InputBufferLength);
	}
		
	if(NodeID == 250 && GatewayID == 0){
	 GatewayID = FindGateway(NodeID, NetworkID, AckTime);
		if(GatewayID !=0){
			EEPROM.write(3, GatewayID);
			if(Verbose || RadioEcho){message.SendMessage(GatewayID, F("Selected GatewayID: "),GatewayID, MESSAGE , Verbose, RadioEcho);}
	  }
		else {
			message.SendMessage(GatewayID,F("ERROR on Finding Gateway"), ERRORCODE, true, false);
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_ON);
		}
			
	}	
	else if(NodeID == 250 && ProgramGatewayID == 0){
		ProgramGatewayID= FindGateway(NodeID,ProgramNetworkID,AckTime);
		if (ProgramGatewayID != 0) {
			EEPROM.write(29, ProgramGatewayID);
      if(Verbose || RadioEcho){message.SendMessage(GatewayID, F("Selected Prog GatewayID: "),ProgramGatewayID, MESSAGE , Verbose, RadioEcho);}
    } 
		else {
      message.SendMessage(GatewayID,F("ERROR on Finding Prog Gateway"), ERRORCODE, true, false);
		  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_ON);
		}
	}
	
	else if(NodeID == 250 && GatewayID != 0 && ProgramGatewayID != 0){
    
		//if(GetNodeParameters(SendBuffer, SendBufferLength, InputBuffer, InputBufferLength,ProgramGatewayID, ProgramNetworkID))
    //  ProcessReturnData(InputBuffer);
	}

//Main Loop after Node Initilization
	else if(sleep_count == SleepCycles || SleepCycles == 0 && NodeID != 250) {

		sleep_count = 0;
		//SerialSendwRadioEcho (GatewayID, "Awake");

		StartTempMeasurements(); //Start Temp Conversions
		digitalWrite(MoisturePWR, HIGH); //Turn on moisture Power
		LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_ON); //sleep to give temp measurement time
														//keep radio on

		//Battery Voltage Content Section
		lawnData.battVoltage = battery.BatteryMeasurement(BatteryADC) / 1000;
				
		//Volumetric Water Content Section
		lawnData.lawnMoisture = MoistureMeasurement();
				
		//Soil Temperature in deg c
		lawnData.lawnTemp = TempMeasurement(0);

     //Message Type
     lawnData.dataType = LAWNPACKET;

     //Message Length
     lawnData.dataLength = sizeof(LawnPacket);
		 InitializeRadio(NodeID,NetworkID, true); // Radio must be initialized after sleep?
		if(radio.sendWithRetry(GatewayID, (const void*)(&lawnData), sizeof(lawnData))){
		//Check if Data in ACK
			if(Verbose || RadioEcho) { message.SendMessage(GatewayID, F("Ack Recieved"), MESSAGE, Verbose, RadioEcho); }
    }
		else
			if(Verbose || RadioEcho) { message.SendMessage(GatewayID, F("Nothing Recieved"), MESSAGE, true, false); }
		
		
	if(Verbose || RadioEcho){ message.SendMessage(GatewayID, F("Sleep Count :"),sleep_count, MESSAGE , Verbose, RadioEcho); }
  
	}
   //preparing to sleep 
  Serial.flush();
  radio.sleep();
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
  if(Verbose || RadioEcho){ message.SendMessage(GatewayID, F( "Getting Parameters"), MESSAGE , Verbose, RadioEcho); }
 
 	//Read From Internal EEPROM
	//SoftwareVer = EEPROM.read(0);
  //NodeType  =        EEPROM.read(1);
  //HardwareVer =      EEPROM.read(2);
	NodeID 		=        EEPROM.read(3);
	NetworkID =        EEPROM.read(4);
  ProgramNetworkID = EEPROM.read(5);
	GatewayID	=        EEPROM.read(6);
  ProgramGatewayID = EEPROM.read(7);
	AckTime 	=        EEPROM.read(8);
	Retries		=        EEPROM.read(9);
	SleepCycles =      EEPROM.read(10);
  SleepTime  =       EEPROM.read(11);
	
	

  //Read From External EEPROM. These values will not change unless node is 
  //rebuilt with new hardware.
  
  Number18S20 = flash.readByte(32514);

	for (byte i = 12; i < 16; i++){
		//KEY[i] = EEPROM.read(i + 4);
	}

	PromiscuousMode = false; //set to 'true' to sniff all packets on the same network

	//Read From External EEPROM. These values will not change unless node is 
	//rebuilt with new hardware.
	//NodeType	= flash.readByte(32512);
	//HardwareVer = flash.readByte(32513);
	Number18S20 = flash.readByte(32514);
  if(Verbose || RadioEcho){ message.SendMessage(GatewayID,F( "Parameters Loaded"),MESSAGE , Verbose, RadioEcho); }
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
uint8_t FindGateway(uint8_t nodeID,  uint8_t networkID, uint8_t AckTime) {
uint8_t     tempGatewayID[2] ={0,0};
int16_t      tempRSSI[2] = {0,0};
uint32_t	  sendTime;
uint8_t     gatewayCount =0;

  
	sendTime = millis();
	radio.setNetwork(networkID); //Set Nework to Desired NetworkID
  if(Verbose || RadioEcho){message.SendMessage(GatewayID,F( "Searching"),MESSAGE , Verbose, false);}   
  radio.send(255,"",1,false);  //broadcast to all Nodes on Network
	while(millis() - sendTime < AckTime*100) {
		if(radio.receiveDone()) {
			tempGatewayID[1] = radio.SENDERID;
      tempRSSI[1] = (uint16_t)radio.DATA; //save RSSI from data packet
      if(radio.ACKRequested())
				radio.sendACK();
			if(Verbose || RadioEcho){message.SendMessage(GatewayID, F("GatewayID:"),tempGatewayID[1], MESSAGE , Verbose, RadioEcho);}
			gatewayCount ++;
			//Compare and replace of highest value into the first position of the array
			if(tempRSSI[1] > tempRSSI[0] || tempRSSI[0]==0){
				tempRSSI[0]= tempRSSI[1];
				tempGatewayID[0] = tempGatewayID[1];
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

   Accepts:     SendBuffer        -as pointer
                SendBufferLength  -as value
                InputBuffer       -as pointer
                InputBufferLength -as refrence
                ProgramGatewayID  -as value
                ProgramNetworkID  -as value

   Returns:     boolean, true means a successful NodeID capture

   Parameters:  FREQUENCY, Hard programmed into device
                NodeID, user Settable
                NetworkID, Same across all radios
                KEY, Used for Radio System

   Description: This function calls upon the programming gateway which is 
   5Mhz above the normal operating frequency and passes node type, hardware version
   software version and external EEPROM address(contained in sendbuffer). The mcu sleeps until a message is
   received back which should contain the node identity. If the mcu wakes without a sucessful capture it returns a boolean false, 
   else it returns a boolean true and the inputbuffer contains the node identity.
 ************************************************** */
bool GetNodeParameters(uint8_t programGatewayID, uint8_t programNetworkID){
  bool nodeIdentitySet = false;
  NewSend myID;  //struct of type NewSend

  myID.dataLength =  sizeof(NewSend); 
  myID.dataType =    NEWSEND;
  myID.nodeType =    EEPROM.read(1);
  myID.hardwareVer = EEPROM.read(2);
  myID.softwareVer = EEPROM.read(0);

  flash.readUniqueID();
  for (uint8_t i = 0, i < 8, i++){
    myID.mac[i] = flash.UNIQUEID[i]
  }
  
  radio.setFrequency(438000000);  	//Change frequency to programing network
  radio.setNetwork(programNetworkID);    			//Change Network to Programming Network
  radio.sendWithRetry(programGatewayID, (const void*)(&myID), sizeof(myID));
  LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);	//Sleep waiting for	data back
  if(radio.receiveDone() && radio.DATA[0] == radio.DATALEN) {
		if(ParseStructData()) { 
      nodeIdentitySet = true;
    }
		if (radio.ACKRequested())
			radio.sendACK();         //Send ACK ASAP
	}
  radio.setFrequency(433000000);
  radio.setNetwork(NetworkID);
  if (nodeIdentitySet)
    if(Verbose || RadioEcho){message.SendMessage(GatewayID, F("Node Identity Sucessfully Obtained"),MESSAGE , Verbose, RadioEcho);}
  else
    message.SendMessage(GatewayID, F("Node Identity Not Obtained"),ERRORCODE , true, true);
    
  return nodeIdentitySet;
}

void StartTempMeasurements (){
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
void InitializeRadio (uint8_t nodeID, uint8_t networkID, bool Encript) {
	radio.initialize(FREQUENCY, nodeID, networkID); //initialize Radio
	if (Encript == true){
		radio.encrypt(KEY);
	}
	radio.promiscuous(PromiscuousMode);

}

void AdjustFrequency (int sensorNumber, int frequency) {
  float AdjustFreq;
  float t;   //Temperature in deg C

  t = TempMeasurement(sensorNumber);
  AdjustFreq = ((123 * t * t * t) + (123 * t * t) + (123 * t) + 123); //our polynomial function
  radio.setFrequency(frequency - AdjustFreq);
}

/* **************************************************
   Name:        MoistureMeasurement

   Returns:     floating point number as % of water

   Parameters: 	None

   Description: This function calls the ReadVcc function to determine actual voltage and uses that 
                along with an ADC voltage conversion of the sensor output. The voltage conversion 
                and the ADC are used in a calculation of the volumetric water content.
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
  realVcc = battery.ReadVcc();
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
		 	Retries = atoi(value);
		 	EEPROM.write(26, Retries);
		 }
		else if(strcmp(parameter, "ACK") == 0){
			AckTime = atoi(value);
			EEPROM.write(25, AckTime);
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
		if(Verbose || RadioEcho){message.SendMessage(GatewayID,F( "Invald Parse"),MESSAGE , true, true);}
		
		  valueLength =0;  //reset length
		  parameterLength =0;
		  message.ClearBuffer(parameter); //clear buffers
		  message.ClearBuffer(value);
		}
	} // end of Message excape to main
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
void Blink (int onTime, int offTime, byte pinNumber, int blinkNumber) {
  int i = 0;

  while (i < blinkNumber) {
    digitalWrite(pinNumber, HIGH);
  delay(onTime);
  digitalWrite(pinNumber, LOW);
  delay(offTime);
  i++;
  }
}

bool ParseStructData() {
  bool parseData = false;
  
  if (radio.DATALEN == radio.DATA[0]) {
    switch (radio.DATA[1]) {
      case NEWRETURN:{
        NewReturn* temp = (NewReturn *)radio.DATA;
        
        EEPROM.write(1, temp -> nodeID);
        EEPROM.write(26, temp -> retries);
        EEPROM.write(25, temp -> ackTime);
        EEPROM.write(28,temp -> sleepTime);
        strcpy(KEY, temp -> key);
        parseData = true; 
        GetNodeIdentity();
        break;
        }
      case COMMAND:
        {
          Message* temp = (Message *)radio.DATA;
          strcpy(InputBuffer, temp-> charMessage);
          ProcessInputString(InputBuffer, strlen(InputBuffer));
          parseData = true;          
          break;
        }
      case MESSAGE:
        {
          Message* temp = (Message *)radio.DATA;
          strcpy(InputBuffer, temp-> charMessage);
          Serial.print(InputBuffer);

          break;
        }
        
        
    }
  }
}




  

