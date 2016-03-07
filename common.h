
#define PACKET_DATA_SIZE 256

struct packet
{
	int type;
	int size;
	int sequence;
	char data[PACKET_DATA_SIZE];
};