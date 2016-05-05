#include <RFM69.h>
#include <SPI.h>
#include <SPIFlash.h>
#include <Flash.h>
#include <NodeMessage.h>

uint8_t  NodeID = 9;
uint8_t  NetworkID = 3;
char     KEY[17]        = {"GreenGrassPeskar"}; //has to be same 16 characters/bytes on all nodes, not more not less!
uint8_t  AckTime  =30 ;  // # of ms to wait for an ack
uint8_t  retries;
uint8_t  MAC[8];
bool	   PromiscuousMode = false;
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


#define FREQUENCY   RF69_433MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define SERIAL_BAUD 115200

//#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W! <=== MOST useful if you have RFM69HW

#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif
RFM69 radio;
SPIFlash flash(FLASH_SS, 0xEF30); //EF40 for 16mbit windbond chip


void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(10);
  radio.initialize(FREQUENCY,NodeID,NetworkID);
  
  radio.encrypt(KEY);
  radio.promiscuous(promiscuousMode);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL! (is chip present?)");
}


void loop() {
  //Capture and process any serial input
  if(Serial.available()> 0){
    message.SerialStreamCapture(InputBuffer, InputBufferLength);
    ProcessInputString(InputBuffer, InputBufferLength);
  }
  
// Capture and Process RFM69 wireless data
  if (radio.receiveDone()) {
    uint8_t senderID;
    uint8_t targetID;
    uint8_t dataLength;
    int16_t senderRSSI; 
    bool    ackRequest;   

    //save radio data from radio first
    for (i =0; i< radio.DATALEN; i++) {
      InputBuffer[i] = radio.DATA[i];
    }
    senderID = radio.SENDERID;
    targetID = radio.TARGETID;
    dataLength = radio.DATALEN;
    senderRSSI = radio.RSSI;
    ackRequest = radio.ACKRequested();

    //send ACK if Needed
    if(ackRequest && targetID == NodeID){
      char ackData[20];
      uint8_t ackDataLength;
      int ackCode = NodeCheck(senderID)
      if (ackCode >0){
        //Package Ack Data
        radio.sendACK(&ackData,ackDataLength);
        if(Verbose){
          message.CreateSendString(SendBuffer, SendBufferLength,F("Ack with Data Sent"), "");
          SerialSendwRadioEcho(SendBuffer);
        }
      }
      else {
        radio.sendACK();
        if(Verbose){
          message.CreateSendString(SendBuffer, SendBufferLength,F("Ack Sent"), "");
          SerialSendwRadioEcho(SendBuffer);
        }
      }
    }

    //Test for new node respond with sender RSSI
    if (senderID == 250) {
      SendBufferLength += sprintf(SendBuffer,"%s",senderRSSI);
    	delay((NodeID-1)*AckTime);
      if(radio.sendWithRetry(senderID,SendBuffer,SendBufferLength,3,AckTime)
      		Serial.print("Sent");      	
    }
    
    if (radio.ACKRequested()) {
      senderID = radio.SENDERID;
      radio.sendACK();
      Serial.print(" - ACK sent.");
    }
   
  }
}




void GetNodeIdentity(){
  if (Verbose || RadioEcho) {
    message.CreateSendString(SendBuffer, SendBufferLength,F("Getting Parameters"), "");
    SerialSendwRadioEcho(SendBuffer);
  }

  //Read From Internal EEPROM
  SoftwareVer = EEPROM.read(0);
  NodeID      = EEPROM.read(1);
  NetworkID   = EEPROM.read(2);
  ackTime     = EEPROM.read(25);
  retries     = EEPROM.read(26);
  SleepCycles = EEPROM.read(27);
  
  for (byte i = 0; i < 16; i++){
    //KEY[i] = EEPROM.read(i + 4);
  }
  for (byte i = 0; i <8; i++) {
    //MAC[i] = EEPROM.read(i+ 30); 
 }

  PromiscuousMode = false; //set to 'true' to sniff all packets on the same network

  //Read From External EEPROM. These values will not change unless node is 
  //rebuilt with new hardware.
  NodeType  = flash.readByte(32512);
  HardwareVer = flash.readByte(32513);
 
  if(Verbose){
    message.CreateSendString(SendBuffer, SendBufferLength,F( "Parameters Loaded"), "");
    SerialSendwRadioEcho(SendBuffer);
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
