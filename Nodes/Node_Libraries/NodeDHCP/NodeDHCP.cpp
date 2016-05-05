#include <NodeDHCP.h>
#include <RFM69.h> 
#include <LowPower.h>
#include <NodeMessage.h>
#include <Arduino.h>


/* **************************************************
   Name:        NodeDHCP
   Accepts:		Encrypt , used to set radio encryption on
   Returns:     GatewayID, 
   Parameters:  NodeID, user Settable
   	   	        NetworkID, Same across all radios
   Description: This function takes in the NodeID, and NetworkID  and uses 
   them to broadcast for a gateway on a given NetworkID.
   The gateways will send back a message with the sender RSSI. 
   The gateway with the highest RSSI wins! Best Gateway wins and is returned. A 
   0 is returned if no gateway is found.
 ************************************************** */
uint8_t NodeDHCP::FindGatway (uint8_t nodeID,  uint8_t networkID ,uint8_t ackTime) {
uint8_t     tempGatewayID[2] ={0,0};
int8_t      tempRSSI[2] = {0,0};
uint8_t     tempAckTime = 0;
uint32_t	sendTime;
uint8_t     gatewayCount =0;

	sendTime = millis();
	RFM69::setNetwork(networkID); //Set network to Desired NetworkID
	//delay(10);
	Serial.println("Searching");
	RFM69::send(255,"",1,false);  //broadcast to all Nodes on Network
	while(millis() - sendTime < ackTime*100) {
		if(RFM69::receiveDone()) {
			tempGatewayID[1] = RFM69::SENDERID;
			tempRSSI[1] = RFM69::RSSI;
			if(RFM69::ACKRequested())
				RFM69::sendACK();
			//SendString(F("Got One!"));
			Serial.print("[RX_RSSI:");delay(10);Serial.print(RFM69::RSSI);delay(10);Serial.println("]");delay(10);
			Serial.print("GatewayID:");delay(10);Serial.println(tempGatewayID[1]);
			gatewayCount ++;
			//simple compare and replace of highest value into the first position of the array
			if(tempRSSI[1] > tempRSSI[0] || tempRSSI[0]==0){
				tempRSSI[0]= tempRSSI[1];
				tempGatewayID[0] = tempGatewayID[1];
			}
		}
	}
Serial.print("Gateway Count:");delay(10);Serial.println(gatewayCount);	
if (tempGatewayID[0] != 0) 
	return tempGatewayID[0];

else
	return 0;
}


/* **************************************************
   Name:        GetNodeParameters

   Accepts:		Encrypt , used to set radio encryption on

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

void NodeDHCP::GetNodeIdentity(uint8_t programNetworkID, uint8_t programGatewayID, char *sendBuffer, uint8_t sendBufferLength, char *inputBuffer, uint8_t &inputBufferLength){

  for (int j = 0; j < 8; j++) {
    message.CreateSendString(sendBuffer, uniqueID[j], sendBufferLength);
	  if (j <7){
		sendBuffer[sendBufferLength]= ':';	  
	    sendBufferLength ++;
	   }
   }
  message.CreateSendString(sendBuffer,"HT:", nodeType, sendBufferLength);
  message.CreateSendString(sendBuffer,"HV:", hardwareVer, sendBufferLength);
  message.CreateSendString(sendBuffer,"SV:", softwareVer, sendBufferLength); 
  RFM69::setFrequency(438000000);  	//Change frequency to programing network
  RFM69::setNetwork(programNetworkID);    			//Change Network to Programming Network
  RFM69::send(programGatewayID, &sendBuffer, sendBufferLength);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_ON);	//Sleep waiting for	data back
  if(RFM69::receiveDone()) {
		for (uint8_t i = 0; i < RFM69::DATALEN; i++){
			inputBuffer[i] = (char)RFM69::DATA[i];
			inputBufferLength += 1;
		}

		if (RFM69::ACKRequested())
			RFM69::sendACK();
		
			
	RFM69::setFrequency(433000000);
	RFM69::setNetwork(networkID);
	
  }
  	
}