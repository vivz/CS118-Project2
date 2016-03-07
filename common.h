#define PACKET_DATA_SIZE 256
#define FILENAME_TYPE 0
#define ACK_TYPE 1
#define DATA_TYPE 2
#define END_TYPE 3
#define WINDOW_SIZE_TYPE 4


struct packet
{
	int type;
	int data_size;
	int sequence;
	char data[PACKET_DATA_SIZE];
};

void printPacket(struct packet p)
{
    printf("sequence:%d, data_size: %d, data: %s\n", p.sequence, p.data_size, p.data);
}

void printPacketArray(struct packet pa[], int size)
{
    for(int i = 0; i < size; i++)
    {
        printPacket(pa[i]);
    }
}
