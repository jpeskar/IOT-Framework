#ifndef NodeDHCP_h
#define NodeDHCP_h

#include <RFM69.h>


class NodeDHCP:public RFM69 {
public:

	NodeDHCP(uint8_t slaveSelectPin=RF69_SPI_CS, uint8_t interruptPin=RF69_IRQ_PIN, bool isRFM69HW=false, uint8_t interruptNum=RF69_IRQ_NUM) :
		RFM69(slaveSelectPin, interruptPin, isRFM69HW, interruptNum) {
	}
	
	uint8_t FindGateway (uint8_t nodeID,  uint8_t networkID, uint8_t ackTime);
	
	void GetNodeIdentity(uint8_t programNetworkID, uint8_t programGatewayID, uint8_t networkID, uint8_t nodeID, char *uniqueID, uint8_t nodeType, uint8_t hardwareVer,
						uint8_t softwareVer, char *sendBuffer, uint8_t sendBufferLength, char *inputBuffer, uint8_t &inputBufferLength);
	

private:
};
#endif