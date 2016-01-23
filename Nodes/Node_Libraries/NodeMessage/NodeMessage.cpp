#include <NodeMessage.h>
#include <arduino.h>
#include <stdlib.h>

//Appends a char array to the buffer array
void NodeMessage::CreateSendString(char *buffer, uint8_t &bufferLength, const char *tag, char *value) {
	bufferLength += sprintf(&buffer[bufferLength],"%s",tag);
	bufferLength += sprintf(&buffer[bufferLength],"%s",value);
}

//Converts an int to a char and appends to the buffer array
void NodeMessage::CreateSendString(char *buffer, uint8_t &bufferLength, const char *tag, uint8_t value) {
	bufferLength += sprintf(&buffer[bufferLength],"%s",tag);
	bufferLength += sprintf(&buffer[bufferLength],"%d",value);
}

//Converts a Hex to a char array and appends to buffer array
void NodeMessage::CreateSendString(char *buffer, uint8_t &bufferLength, uint8_t hexValue) {
	bufferLength += sprintf(&buffer[bufferLength],"%02X",hexValue);
}

//Converts a float value into a char array and appends to buffer array
void NodeMessage::CreateSendString(char *buffer, uint8_t &bufferLength, const char *tag, float floatValue, uint8_t width, uint8_t precision) {
	char tempChar[10];
	
	 dtostrf(floatValue, width, precision, tempChar);
	 bufferLength += sprintf(&buffer[bufferLength],"%s",tag);
	 bufferLength += sprintf(&buffer[bufferLength],"%s",tempChar);
}
void NodeMessage::CreateSendString(char *buffer, uint8_t &sendBufferLength, const __FlashStringHelper* message, int8_t value) {
	sendBufferLength += sprintf_P(&buffer[sendBufferLength], PSTR("%S") , message);
	sendBufferLength += sprintf(&buffer[sendBufferLength],"%d",value);
}
void NodeMessage::CreateSendString(char *buffer, uint8_t &sendBufferLength, const __FlashStringHelper* message, char *value) {
	sendBufferLength += sprintf_P(&buffer[sendBufferLength], PSTR("%S") , message);
	sendBufferLength += sprintf(&buffer[sendBufferLength],"%s",value);
}
void NodeMessage::CreateHarwareIdentity(char *sendBuffer, uint8_t &sendBufferLength, uint8_t nodeType, uint8_t hwVer, uint8_t swVer, uint8_t *mac){
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

void NodeMessage::ClearBuffer (char *buffer){
	uint8_t buffLength = strlen(buffer);

	for (int i = 0; i <  buffLength; i++) {
		buffer[i] = 0;
	}
}

void NodeMessage::SerialStreamCapture(char *buffer, uint8_t &bufferLength) {
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
void NodeMessage::SerialSendwRadioEcho (char *payload, unsigned int gatewayID, bool verbose, bool radioEcho)
{
	uint8_t payloadLength;

	payloadLength = strlen(payload);
	if(verbose == true && radioEcho == true){
		Serial.print(F("Sending To: "));
		Serial.println(gatewayID);
		Serial.println(payload);
		RFM69::send(gatewayID, payload, payloadLength);
	}
	else if (verbose == true) {
		for (int i = 0; i < payloadLength; i++) {
			Serial.print(payload[i]);
			delay(10);
		}
		Serial.print("\n");
	}
	else if (radioEcho == true) {
		RFM69::send(gatewayID, payload, payloadLength);
			
	}
	ClearBuffer(payload);
	
}


NodeMessage message;