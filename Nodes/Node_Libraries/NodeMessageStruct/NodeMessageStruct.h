#ifndef NodeMessageStruct_h
#define NodeMessageStruct_h

#include <RFM69.h>
#include <NodeStruct.h>


class NodeMessageStruct: public RFM69 {
public:

	NodeMessageStruct(uint8_t slaveSelectPin=RF69_SPI_CS, uint8_t interruptPin=RF69_IRQ_PIN, bool isRFM69HW=false, uint8_t interruptNum=RF69_IRQ_NUM) :
		RFM69(slaveSelectPin, interruptPin, isRFM69HW, interruptNum) {
	}
	
	
	void SendMessage(uint8_t gateway, const __FlashStringHelper* message, uint8_t type, bool verbose, bool echo);
	void SendMessage(uint8_t gateway, const __FlashStringHelper* message, uint8_t value, uint8_t type, bool verbose, bool echo);
	void ClearBuffer (char *buffer);
	void SerialStreamCapture(char *buffer, uint8_t &bufferLength);
	void SerialSendwRadioEcho (Message myMessage, uint8_t gatewayID, bool verbose, bool echo);
	
	
};
extern NodeMessageStruct message;
#endif