#define PACKET_DATA_SIZE 256
#define FILENAME_TYPE 0
#define ACK_TYPE 1
#define DATA_TYPE 2


struct packet
{
	int type;
	int size;
	int sequence;
	char data[PACKET_DATA_SIZE];
};