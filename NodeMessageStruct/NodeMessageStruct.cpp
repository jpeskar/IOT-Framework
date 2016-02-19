#include <NodeMessageStruct.h>


void NodeMessageStruct::SendMessage(uint8_t gateway, const __FlashStringHelper* message, uint8_t type, bool verbose, bool echo) {
	Message myMessage;
	myMessage.dataType = type;
	sprintf_P(myMessage.charMessage, PSTR("%S") , message);
	myMessage.dataLength = sizeof(Message);
	
	 NodeMessageStruct::SerialSendwRadioEcho (myMessage, gateway, verbose, echo);
}

void NodeMessageStruct::SendMessage(uint8_t gateway, const __FlashStringHelper* message, uint8_t value, uint8_t type, bool verbose, bool echo) {
	Message myMessage;
	myMessage.dataType = type;
	sprintf_P(myMessage.charMessage, PSTR("%S") , message);
	sprintf(myMessage.charMessage, "%S:%d", myMessage.charMessage, value);
	myMessage.dataLength = sizeof(Message);
	
	NodeMessageStruct::SerialSendwRadioEcho (myMessage, gateway, verbose, echo);
}

void NodeMessageStruct::ClearBuffer (char *buffer){
	uint8_t buffLength = strlen(buffer);

	for (int i = 0; i <  buffLength; i++) {
		buffer[i] = 0;
	}
}



/* **************************************************
   Name:        SerialStreamCapture

   Returns:     buffer			- As a pointer
				bufferLength	- As Reference
				

   Accepts:		buffer			- As a pointer
				bufferLength	- As a reference

   Description: This function captures the data stream from the UART port and puts it into an input buffer which.
 ************************************************** */
void NodeMessageStruct::SerialStreamCapture(char *buffer, uint8_t &bufferLength) {
	char temp;
	uint8_t i = 0;
	
	
	while(Serial.available()>0){
		temp= Serial.read();
		if(temp != '\n' || temp !='\r'){ //TODO handle "\n\r"
			buffer[bufferLength] = temp;
			bufferLength++;
			delay(20);
		}
	}
	buffer[bufferLength] = '\0'; //Null Terminate the string.
	
	Serial.print("InputBufferLength:"); Serial.println(bufferLength);
	Serial.print("InputBuffer:"); Serial.println(buffer);
}



void NodeMessageStruct::SerialSendwRadioEcho (Message myMessage, uint8_t gatewayID, bool verbose, bool echo) {
	if(verbose == true && echo == true){
		Serial.print(F("Sending To: "));
		Serial.println(gatewayID);
		Serial.print("Message Type: ");
		Serial.println(myMessage.dataType);
		Serial.print("Message: ");
		Serial.println(myMessage.charMessage);
		
		RFM69::send(gatewayID,(const void*)(&myMessage), sizeof(myMessage));
	}
	else if (verbose == true) {
		Serial.print(myMessage.charMessage);
		Serial.print("\n");
	}
	else if (echo == true) {
		RFM69::send(gatewayID,(const void*)(&myMessage), sizeof(myMessage));
			
	}
}


NodeMessageStruct message;