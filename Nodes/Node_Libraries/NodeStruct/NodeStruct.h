#ifndef NodeStruct_h
#define NodeStruct_h


#define MESSAGE		1
#define ERRORCODE	2
#define COMMAND		3
#define NEWSEND		4
#define NEWRETURN   5
#define LAWNPACKET	6

struct LawnPacket {
    uint8_t dataLength;
    uint8_t dataType;
    float battVoltage;
    float lawnMoisture;
    float lawnTemp;    
};

struct NewSend {
	uint8_t dataLength;
	uint8_t dataType; //#3
	uint8_t mac[8];
	uint8_t nodeType;
	uint8_t hardwareVer;
	uint8_t softwareVer;
};

struct NewReturn {
	uint8_t		dataLength;
	uint8_t		dataType; //#4
	uint8_t		nodeID;
	uint8_t		retries;
	uint8_t		ackTime;
	uint16_t	sleepTime;
	char		key[17];
};

struct Message {
	uint8_t dataLength;
	uint8_t dataType; // #1 or #2
	char    charMessage[50];
};

#endif