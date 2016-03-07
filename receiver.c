
/*
 A simple client in the internet domain using TCP
 Usage: ./client hostname port (./client 192.168.0.151 10000)
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <strings.h>

#include "common.h"


void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    // Socket descriptor
    int socketfd; 
    int portno, n;
    int receive_length;
    int execution_no = 0;
    int buffer_full = 0, i = 0, end = 0;
    int window_size, packets_per_window;
    char *filename, *hostname;
    double p_loss, p_corrupt;
    struct sockaddr_in sender_addr;
    socklen_t senderlen = sizeof(sender_addr);
    FILE* fp = NULL;
    // contains tons of information, including the server's IP address
    struct hostent *server; 
    char buffer[256];
    struct packet filename_pkt, received_pkt, ack_pkt;
    struct packet *packet_buffer;
    int buffer_base = 0, buffer_tail = 0;
    int baselength = sizeof(filename_pkt.type) * 3 + 1;
    ack_pkt.type = ACK_TYPE;
    
    if (argc < 6) {
       fprintf(stderr,"usage: %s <hostname> <port> <filename> <p(loss)> <p(corruption)>\n", argv[0]);
       exit(1);
    }
    
    hostname = argv[1];
    portno = atoi(argv[2]);
    filename = argv[3];
    p_loss = atof(argv[4]);
    p_corrupt = atof(argv[5]);
    
    // create a new socket
    socketfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (socketfd < 0) 
        error("ERROR opening socket");
    
    // takes a string like "www.yahoo.com", and returns a struct hostent, 
    // which contains information, as IP address, address type, 
    // the length of the addresses...
    server = gethostbyname(hostname); 
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    // bind the socket to any valid IP address and a specific port
    memset((char *) &sender_addr, 0, sizeof(sender_addr));

    // initialize server's address
    sender_addr.sin_family = AF_INET; 
    memcpy((char *)server->h_addr, (char *)&sender_addr.sin_addr.s_addr, server->h_length);
    sender_addr.sin_port = htons(portno);   

    memset((char *) &filename_pkt, 0, sizeof(filename_pkt));
    strcpy(filename_pkt.data, filename);
    filename_pkt.type = FILENAME_TYPE;
    filename_pkt.data_size = strlen(filename); 

    // send the file name
    n = sendto(socketfd, &filename_pkt, sizeof(struct packet), 0, (struct sockaddr *)&sender_addr, senderlen);
    if (n < 0) 
        error("ERROR writing to filename socket");
    else {
        printf("%2d) Sent filename packet\n", execution_no++);
    }
    
    printf("Requested file %s\n", filename);

    while(1){
      //receiving a packet
      receive_length = recvfrom(socketfd, &received_pkt, sizeof(received_pkt), 0, (struct sockaddr *)&sender_addr, &senderlen);
      if (receive_length < 0)
      {
        printf("Error receiving a packet\n");
        continue;
      }

      if(received_pkt.type == END_TYPE) {
        printf("%2d) Received END packet\n", execution_no++);
        //TODO: deal wit the remaining packets in the buffer
        break;
      }

      else if(received_pkt.type == WINDOW_SIZE_TYPE){
        window_size = received_pkt.data_size;
        printf("%2d) Received WINDOW_SIZE packet, Window Size: %d\n", execution_no++, window_size);
        //initializing packet buffer array
        packets_per_window = (window_size * 1024) / sizeof(struct packet);
        packet_buffer = (struct packet*) malloc(packets_per_window * sizeof(struct packet));
        for(i = 0; i < packets_per_window; i++)
        {
            packet_buffer[i].type = PLACE_HOLDER_TYPE;
            packet_buffer[i].sequence = i * PACKET_DATA_SIZE;
        }
        continue;
      }

      else if (received_pkt.type == DATA_TYPE) {

        if (fp == NULL) {
            char* new_filename = strcat(filename, "_copy");
            fp = fopen(new_filename, "wb");
            printf("%2d) Created %s\n", execution_no++, new_filename);
        }

        printf("%2d) Received DATA packet, Size: %d, Sequence: %ld\n", 
            execution_no++, received_pkt.data_size, received_pkt.sequence);
        if (PRINT_DATA)
            printf("Data: \n%s\n", received_pkt.data);
        //printf("ftell is %ld\n", ftell(fp));

        //if the packet is in order
        if(ftell(fp) == received_pkt.sequence){
            //writing incoming packet to the file
            n = fwrite(received_pkt.data,received_pkt.data_size,1,fp);
            printf("%2d) Wrote DATA packet, Sequence: %ld\n", execution_no++, received_pkt.sequence);

            if(n < 0)
            {
                printf("Error writing to the file\n");
                continue;
            }
            end = 0;
            //go through the buffer 
            for(i = 0; i != packets_per_window; i++)
            {
                if(packet_buffer[i].sequence == ftell(fp) && packet_buffer[i].type != PLACE_HOLDER_TYPE)
                {
                    //write in-order buffered packets
                    if(fwrite(packet_buffer[i].data,packet_buffer[i].data_size,1,fp) <0 )
                    {
                        printf("Failed writing buffered packet to the file\n");
                        break;
                    }
                    else {
                        printf("%2d) Wrote DATA packet from the buffer,, Sequence: %ld\n", execution_no++, packet_buffer[i].sequence);
                    }
                    //TODO: wrap around sequence number when it exceeds
                    //fill the next expected spot with a place-holder 
                    packet_buffer[i].type = PLACE_HOLDER_TYPE;
                    if( i!= 0)
                        packet_buffer[i].sequence = packet_buffer[i-1].sequence + PACKET_DATA_SIZE;
                    else 
                        packet_buffer[i].sequence = packet_buffer[packets_per_window - 1].sequence + PACKET_DATA_SIZE;
                    end = 1; 
                }
                //don't check the rest if we finish writing the ones in order
                else if(end == 1)
                {
                    end = 0;
                    break;
                }
            }
        }
        //if it's out of order, put it in the buffer
        else
        {
            printf("received an out of order packet\n");
            for(i = 0; i != packets_per_window; i++)
            {
                if(received_pkt.sequence == packet_buffer[i].sequence)
                {
                    packet_buffer[i] = received_pkt; 
                    goto label;
                }
            }
            //if this packet isn't expected, we just drop it   
        }
        //send acknowledgement packet
        ack_pkt.sequence = received_pkt.sequence;
        if(sendto(socketfd, &ack_pkt, sizeof(struct packet), 0, (struct sockaddr *)&sender_addr, senderlen) < 0)
        {
            printf("ERROR writing to acknowledgement socket");
            continue;
        }
        else {
            printf("%2d) Sent ACK packet, Sequence: %ld\n", execution_no++, ack_pkt.sequence);
        }
        //if the packet isn't expected, don't send ACK
        label: ;
      }
     
    }

    fclose(fp);
    //close socket
    close(socketfd); 
    
    return 0;
}
