#define PACKET_DATA_SIZE 512
#define FILENAME_TYPE 7
#define ACK_TYPE 1
#define DATA_TYPE 2
#define END_TYPE 3
#define WINDOW_SIZE_TYPE 4
#define PLACE_HOLDER_TYPE 5
#define RETRANSMISSION_TYPE 6

#define PRINT_DATA 0
#define PRINT_BUFFER_STATUS 0
#define PRINT_SEND_WINDOW_STATUS 0

#define MAX_SEQUENCE_NUMBER 30720

#define TIMEOUT 0.5

struct packet
{
	int type;
	int data_size;
	long sequence;
	char data[PACKET_DATA_SIZE];
};

void printPacket(struct packet p)
{
    char* packet_type;

    if (p.type == FILENAME_TYPE) 
        packet_type = "FILENAME";
    else if (p.type == ACK_TYPE)
        packet_type = "ACK";
    else if (p.type == DATA_TYPE)
        packet_type = "DATA";
    else if (p.type == END_TYPE)
        packet_type = "END";
    else if (p.type == WINDOW_SIZE_TYPE)
        packet_type = "WINDOW_SIZE";
    else if (p.type == PLACE_HOLDER_TYPE)
        packet_type = "PLACE_HOLDER";
    else
        packet_type = "UNKNOWN";
    printf("Packet type: %s, Sequence:%ld, Data Size: %d\n", packet_type, p.sequence, p.data_size);
    if (PRINT_DATA)
        printf("Data: \n%s\n", p.data);
}

void printPacketArray(struct packet pa[], int size)
{
    int i;
    for(i = 0; i < size; i++)
    {
        printPacket(pa[i]);
    }
}
