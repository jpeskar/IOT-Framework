#include <RFM69.h>
#include <SPI.h>
#include <SPIFlash.h>
#include <EEPROM.h>
#include <NodeStruct.h>
#include <SocketIOClient.h>
#include <ArduinoJson.h>
//#include <Ethernet.h>
//#include <utility/w5100.h> 

uint8_t  NodeID = 9;
uint8_t  NetworkID = 3;
char     KEY[17]   = {"GreenGrassPeskar"}; //has to be same 16 characters/bytes on all nodes, not more not less!
uint8_t  AckTime   = 30 ;  // # of ms to wait for an ack
uint8_t  retries;
uint8_t  MAC[8];
bool	   PromiscuousMode = false;
uint8_t  NodeType;
uint8_t  SoftwareVer;
uint8_t	 HardwareVer;
uint8_t	 Number18S20;
bool	   Verbose = true;
char     InputBuffer[64];
char     SendBuffer[64];
uint8_t  SendBufferLength = 0;
uint8_t	 InputBufferLength = 0;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char hostname[] = "10.10.0.5";
int port = 1234;
StaticJsonBuffer<200> jsonBuffer;


#define FREQUENCY   RF69_433MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define SERIAL_BAUD 115200
//#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W! <=== MOST useful if you have RFM69HW

#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
  #define ETHERNET_SS   10 // Ethernet W5100 SS on D10
  #define RFM69_SS      4  // Radio SS on D4
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
  #define RFM69_SS      10// Radio SS on D10
#endif
RFM69 radio;

SPIFlash flash(FLASH_SS, 0xEF30); //EF40 for 16mbit windbond chip
SocketIOClient socket; 

void setup() {
  //explicit enable each of the Slave Select pins 
  pinMode(ETHERNET_SS,OUTPUT);  //enable SPI slave select pin for ethernet
  pinMode(RFM69_SS, OUTPUT);    //enable SPI slave select pin for radio
  pinMode(FLASH_SS, OUTPUT);    //enable SPI slave select pin for flash memory
  Serial.begin(SERIAL_BAUD);
  delay(10);
  
  //Enable Ethernet first... Allows for Warings back to server
  ssEnable(ETHERNET_SS);
  Ethernet.begin(mac);

//Enable External Flash
  ssEnable(FLASH_SS);
  if (flash.initialize())
    if (Verbose) {Serial.println("SPI Flash Init OK!");}
    GetNodeIdentity();
  else
    //TODO: Error Handler - Send ERROR Message to Server
    if (Verbose) {Serial.println("SPI Flash Init FAIL! (is chip present?)");}

  //Enable Radio
  ssEnable(RFM69_SS);
  radio.initialize(FREQUENCY,NodeID,NetworkID);
  radio.encrypt(KEY);
  radio.promiscuous(PromiscuousMode);

}
  
void loop() {

  if(NodeID == 250) {
    //Add New Gateway Code
  }

  
  //Capture and process any serial input
  if(Serial.available()> 0){
    //message.SerialStreamCapture(InputBuffer, InputBufferLength);
    //ProcessInputString(InputBuffer, InputBufferLength);
  }
   ssEnable(ETHERNET_SS);
  //Capture Socket.io Data
  //if(socket.monitor()){
    //
    //parse JSON Data
  //}
  
// Capture and Process RFM69 wireless data
  ssEnable(RFM69_SS);
  if (radio.receiveDone()) {
    uint8_t senderID;
    uint8_t targetID;
    uint8_t dataLength;
    int16_t senderRSSI; 
    bool    ackRequest;   

    //save radio data from radio first
    for (uint8_t i =0; i< radio.DATALEN; i++) {
      InputBuffer[i] = radio.DATA[i];
    }
    senderID = radio.SENDERID;
    targetID = radio.TARGETID;
    dataLength = radio.DATALEN;
    senderRSSI = radio.RSSI;
    ackRequest = radio.ACKRequested();

    //send ACK if Needed
    if(ackRequest && targetID == NodeID){ //if an ACK is requested and the message was for me
      uint8_t  ackData;
      uint8_t ackDataLength;
      ackData = NodeCheck(senderID);
      if (ackData>0){ //check to see if there is additional info for this node
        radio.sendACK(&ackData, 1); //send Ack Data
        if(Verbose){Serial.print(F("Ack with Data Sent"));}
      }
      else {
        radio.sendACK();
        if(Verbose){Serial.print(F("Ack Sent"));}
      }
    } 
    //Test for new node respond with sender RSSI
    if (senderID == 250) {
      delay((NodeID-1)*AckTime);
      if(radio.sendWithRetry(senderID,(const void *)(&senderRSSI),sizeof(senderRSSI),3,AckTime));
      		if (Verbose) {Serial.print("Sent");}
      }
    
    SendDataToServer(senderID, senderRSSI, dataLength); //package data to JSON for Node server
   
  } //end of radio recieved
} //end of main loop

void GetNodeIdentity(){
  if (Verbose) {Serial.print(F("Getting Parameters"));}

  //Read From Internal EEPROM
  SoftwareVer = EEPROM.read(0);
  NodeID      = EEPROM.read(1);
  NetworkID   = EEPROM.read(2);
  AckTime     = EEPROM.read(25);
  retries     = EEPROM.read(26);
  Verbose     = EEPROM.read(40); //need to adjust this value

  for (byte i = 0; i < 16; i++){
    //KEY[i] = EEPROM.read(i + 4);
  }
  for (byte i = 0; i <8; i++) {
    //MAC[i] = EEPROM.read(i+ 30); 
 }

  PromiscuousMode = false; //set to 'true' to sniff all packets on the same network

  //Read From External EEPROM. These values will not change unless node is 
  //rebuilt with new hardware.
  ssEnable(FLASH_SS);
  NodeType  = flash.readByte(32512);
  HardwareVer = flash.readByte(32513);
  ssEnable(RFM69_SS);
  if(Verbose){Serial.print(F( "Parameters Loaded")); }
}
//look up node update codes.
//once sent remove from table
//Values:
//0 - No Command
//1 - Send Values Now
//2 - Program Reset
//4 - Reset to New
//5 - Shutdown
//6 - Check into program gateway
//7 - Reprogram
uint8_t NodeCheck(uint8_t senderID) {
  int memLocation;
  memLocation = senderID+99 //memory starts at 100 as there is no nodeID 0
  return EEPROM.read(memLocation);
}

void  NodeUpdateWrite(uint8_t nodeID, uint8_t updateNumber){
  int memLocation;
  memLocation = senderID+99 //memory starts at 100 as there is no nodeID 0
  EEPROM.write(memLocation, updateNumber);
} 


void SendDataToServer(uint8_t senderID, int16_t senderRSSI, uint8_t dataLength) {
  if (dataLength == InputBuffer[0]) {

    JsonObject& object = jsonBuffer.createObject();
    char event[15];
    object["GatewayID"] = NodeID;
    object["NetworkID"] = NetworkID;
    object["NodeID"]  = senderID;
    object["RSSI"] = senderRSSI;
     
    switch (InputBuffer[1]) {
      case LAWNPACKET: {
         LawnPacket* temp = (LawnPacket *)&InputBuffer;
         object["BattV"] = temp -> battVoltage;
         object["VWC"]  = temp -> lawnMoisture;
         object["LTC"]  = temp -> lawnTemp;
         sprintf(event, "%s", "nodeData");
         break;
      }
      case NEWSEND {
        NewSend* temp =(Message *)&InputBuffer;
        Object["MAC"] = temp -> mac;
        Object["NTYPE"] = temp -> nodeType;
        
        }
      case MESSAGE: {
        Message* temp = (Message *)&InputBuffer;
        object["MSG"] = temp -> charMessage;
        sprintf(event, "%s", "nodeMessage");
        break;
      }
      case ERRORCODE: {
        Message* temp = (Message *)&InputBuffer;
        object["ERROR"] = temp -> charMessage;
       sprintf(event, "%s", "nodeError");
        break;
      }
      case COMMAND :{
        Message* temp = (Message *)&InputBuffer;
        object["CMD"] = temp -> charMessage;
        sprintf(event, "%s", "nodeCommand");
        break;
      }
      //Packaging and sending data over Socket.io
      uint16_t outputLen = object.measureLength(); //determine length of JSON object
      char outputBuffer[outputLen];              //Make buffer size of Object
      object.printTo(outputBuffer, outputLen);    //print object to buffer
      pinMode(ETHERNET_SS,OUTPUT);
      //socket.send(event , outputBuffer, outputLen); //send data over socket.io connector
      pinMode(RFM69_SS,OUTPUT);
    }
  }
}

void ssEnable(uint8_t pinEnabled) {
  switch (pinEnabled) {
    case ETHERNET_SS:
      digitalWrite(RFM69_SS, HIGH); //disable radio SPI
      digitalWrite(FLASH_SS,HIGH);   //disable flash SPI
      digitalWrite(ETHERNET_SS, LOW); //enable ethenet

    case RFM69_SS:
      digitalWrite(FLASH_SS,HIGH);   //disable flash SPI
      digitalWrite(ETHERNET_SS, HIGH); //disable ethenet
      digitalWrite(RFM69_SS, LOW); //enable radio SPI

    case FLASH_SS:
      digitalWrite(ETHERNET_SS, HIGH); //disable ethenet
      digitalWrite(RFM69_SS, HIGH); //disable radio SPI
      digitalWrite(FLASH_SS, LOW);   //enable flash SPI
   }
   
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}
/*
if (Serial.available() > 0)
  {
    char input = Serial.read();
    if (input == 'r') //d=dump all register values
      radio.readAllRegs();
    if (input == 'E') //E=enable encryption
      radio.encrypt(KEY);
    if (input == 'e') //e=disable encryption
      radio.encrypt(null);
    if (input == 'p')
    {
      promiscuousMode = !promiscuousMode;
      radio.promiscuous(promiscuousMode);
      Serial.print("Promiscuous mode ");Serial.println(promiscuousMode ? "on" : "off");
    }
    
    if (input == 'd') //d=dump flash area
    {
      Serial.println("Flash content:");
      int counter = 0;

      while(counter<=256){
        Serial.print(flash.readByte(counter++), HEX);
        Serial.print('.');
      }
      while(flash.busy());
      Serial.println();
    }
    if (input == 'D')
    {
      Serial.print("Deleting Flash chip content... ");
      flash.chipErase();
      while(flash.busy());
      Serial.println("DONE");
    }
    if (input == 'i')
    {
      Serial.print("DeviceID: ");
      word jedecid = flash.readDeviceId();
      Serial.println(jedecid, HEX);
    }
  }
*/
