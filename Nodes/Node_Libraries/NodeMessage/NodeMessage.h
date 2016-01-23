#ifndef NodeMessage_h
#define NodeMessage_h

#include <RFM69.h>


class NodeMessage: public RFM69 {
public:

	NodeMessage(uint8_t slaveSelectPin=RF69_SPI_CS, uint8_t interruptPin=RF69_IRQ_PIN, bool isRFM69HW=false, uint8_t interruptNum=RF69_IRQ_NUM) :
		RFM69(slaveSelectPin, interruptPin, isRFM69HW, interruptNum) {
	}
	
	void CreateSendString(char *buffer, uint8_t &bufferLength, const char *tag, char *value); //char Key and value message
	void CreateSendString(char *buffer, uint8_t &bufferLength, const char *tag, uint8_t vaue); //int Key and value message
	void CreateSendString(char *buffer, uint8_t &bufferLength, uint8_t hexValue); //store hex value
	void CreateSendString(char *buffer, uint8_t &bufferLength, const char *tag, float floatValue, uint8_t width, uint8_t precision);
	void CreateSendString(char *buffer, uint8_t &sendBufferLength, const __FlashStringHelper* message, int8_t value); //flash stored Key and value
	void CreateSendString(char *buffer, uint8_t &sendBufferLength, const __FlashStringHelper* message, char *value); //flash stored key and char value
	void CreateHarwareIdentity(char *sendBuffer, uint8_t &sendBufferLength, uint8_t nodeType, uint8_t hwVer, uint8_t swVer, uint8_t *mac);
	void ClearBuffer (char *buffer);
	void SerialStreamCapture(char *buffer, uint8_t &bufferLength);
	void SerialSendwRadioEcho (char *payload, unsigned int gatewayID, bool verbose, bool radioEcho);
	
	
};
extern NodeMessage message;
#endif