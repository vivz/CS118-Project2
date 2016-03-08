
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
#include <time.h>
#include <math.h>

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
    int portno, error_flag;
    int receive_length;
    int execution_no = 0;
    int buffer_full = 0, i = 0, end = 0;
    int packets_per_window;
    int window_size;
    char *filename, *hostname;
    double p_loss, p_corrupt;
    struct sockaddr_in sender_addr;
    socklen_t senderlen = sizeof(sender_addr);
    FILE* fp = NULL;
    // contains tons of information, including the server's IP address
    struct hostent *server; 
    struct packet filename_pkt, received_pkt, ack_pkt;
    struct packet *packet_buffer;
    int buffer_base = 0, buffer_tail = 0;
    int baselength = sizeof(filename_pkt.type) * 3 + 1;
    ack_pkt.type = ACK_TYPE;
    

    long expected_data_size, written_data_size = 0;
    long last_written_sequence;

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
    error_flag = sendto(socketfd, &filename_pkt, sizeof(struct packet), 0, (struct sockaddr *)&sender_addr, senderlen);
    if (error_flag < 0) 
        error("ERROR writing to filename socket");
    else {
        printf("%2d) Sent FILENAME packet, Requested: %s\n", execution_no++, filename);
    }
    
    while(1)
    {
      //receiving a packet
      receive_length = recvfrom(socketfd, &received_pkt, sizeof(received_pkt), 0, (struct sockaddr *)&sender_addr, &senderlen);
      if (receive_length < 0)
      {
        printf("Error receiving a packet\n");
        continue;
      }

      if(received_pkt.type == END_TYPE) {
        printf("%2d) Received END packet\n", execution_no++);
        //TODO: deal with the rest of the packets in the buffer.
        //TODO: wait untill timeout????
        if(fp!=NULL)
            fclose(fp);
        break;
      }

      else if(received_pkt.type == WINDOW_SIZE_TYPE)
      {
        window_size = received_pkt.data_size;
        //initializing packet buffer array
        packets_per_window = (window_size * 1024) / sizeof(struct packet);
        // printf("packets per window: %d\n", packets_per_window);
        packet_buffer = (struct packet*) malloc(packets_per_window * sizeof(struct packet));
        for(i = 0; i < packets_per_window; i++)
        {
            packet_buffer[i].type = PLACE_HOLDER_TYPE;
            packet_buffer[i].sequence = (i * PACKET_DATA_SIZE) % MAX_SEQUENCE_NUMBER;
        }

        // we put the data size in the sequence field, janky I know
        expected_data_size = received_pkt.sequence;
        printf("%2d) Received WINDOW_SIZE packet, Window Size: %d, Filesize: %ld, Packets Per Window: %d\n", 
            execution_no++, window_size, expected_data_size, packets_per_window);
        continue;
      }

      else if (received_pkt.type == DATA_TYPE) {
        
        // Corruption
        float random_prob = (rand() % 10000) / 10000.0;
        if (random_prob < p_corrupt) {
            goto send_retransmission;
        }
        



        if (fp == NULL) 
        {
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
        if (packet_buffer[buffer_base].sequence == received_pkt.sequence)
        {
            //writing incoming packet to the file
            error_flag = fwrite(received_pkt.data,received_pkt.data_size,1,fp);
            if(error_flag < 0)
            {
                printf("Error writing to the file\n");
                continue;
            }
            else 
            // writing was successful
            {
                printf("%2d) Wrote DATA packet, Sequence: %ld\n", execution_no++, received_pkt.sequence);
                written_data_size += received_pkt.data_size;
                last_written_sequence = received_pkt.sequence;
                // printf("written_data_size: %ld\n", written_data_size);
                if (written_data_size == expected_data_size){
                    printf("%2d) Full filesize received: %ld\n", execution_no++, written_data_size);
                    // printf("received packet sequence before send ack: %ld\n", received_pkt.sequence);
                    goto send_ack;
                }
                // after writing from buffer, update location written from to be an implied buffer tail 
                // with placeholder and move buffer base to next 

                packet_buffer[buffer_base].type = PLACE_HOLDER_TYPE;

                // calculating the index right before the buffer_base, accounting for edge case of base being 0 
                buffer_tail = buffer_base == 0 ? packets_per_window - 1 : buffer_base - 1;
                packet_buffer[buffer_base].sequence = (packet_buffer[buffer_tail].sequence + PACKET_DATA_SIZE) % MAX_SEQUENCE_NUMBER;
                buffer_base = (buffer_base + 1) % packets_per_window;
                if (PRINT_BUFFER_STATUS)
                    printPacketArray(packet_buffer, packets_per_window);

            }

            // after successful in order write, we want to write out any
            // data packets waiting in the buffer

            while (packet_buffer[buffer_base].sequence == (last_written_sequence + PACKET_DATA_SIZE) % MAX_SEQUENCE_NUMBER && 
                packet_buffer[buffer_base].type != PLACE_HOLDER_TYPE)
            {
                error_flag = fwrite(packet_buffer[buffer_base].data,packet_buffer[buffer_base].data_size,1,fp);
                if (error_flag < 0) 
                {
                    printf("Failed writing buffered packet to the file\n");
                    break;
                } 
                else
                {
                    printf("%2d) Wrote DATA packet from the buffer, Sequence: %ld\n", execution_no++, packet_buffer[buffer_base].sequence);
                    written_data_size += packet_buffer[buffer_base].data_size;
                    last_written_sequence = packet_buffer[buffer_base].sequence;
                    // printf("written_data_size: %ld\n", written_data_size);
                    if (written_data_size == expected_data_size){
                        printf("%2d) Full filesize received: %ld\n", execution_no++, written_data_size);
                        break;
                    }
                    packet_buffer[buffer_base].type = PLACE_HOLDER_TYPE;
                    // calculating the index right before the buffer_base, accounting for edge case of base being 0 
                    buffer_tail = buffer_base == 0 ? packets_per_window - 1 : buffer_base - 1;
                    packet_buffer[buffer_base].sequence = (packet_buffer[buffer_tail].sequence + PACKET_DATA_SIZE) % MAX_SEQUENCE_NUMBER;
                    buffer_base = (buffer_base + 1) % packets_per_window;
                    if (PRINT_BUFFER_STATUS)
                        printPacketArray(packet_buffer, packets_per_window);
                }
            }
        }
        //if it's out of order, put it in the buffer
        else
        {
            for(i = 0; i != packets_per_window; i++)
            {
                if(received_pkt.sequence == packet_buffer[i].sequence)
                {
                    printf("%2d) Received DATA out of order packet (Sequence: %ld) put at buffer location %d\n", 
                        execution_no++, received_pkt.sequence, i);
                    packet_buffer[i] = received_pkt; 
                    if (PRINT_BUFFER_STATUS)
                        printPacketArray(packet_buffer, packets_per_window);
                    goto send_ack;
                }
            }
            //if this packet isn't expected, we just drop it   
            printf("%2d) Received DATA packet (Sequence: %ld) not in window range, dropping\n", 
                execution_no++, received_pkt.sequence);
            goto no_ack;
        }

        //send acknowledgement packet
        send_ack:
            ack_pkt.sequence = received_pkt.sequence;
            ack_pkt.data_size = received_pkt.data_size;
            // printf("in send_ack\n");
            if(sendto(socketfd, &ack_pkt, sizeof(struct packet), 0, (struct sockaddr *)&sender_addr, senderlen) < 0)
            {
                printf("ERROR writing to acknowledgement socket");
                continue;
            }
            else {
                printf("%2d) Sent ACK packet, Sequence: %ld, Data size: %d\n", 
                    execution_no++, ack_pkt.sequence, ack_pkt.data_size);
                if (written_data_size == expected_data_size)
                    break;
            }
        //if the packet isn't expected, don't send ACK
        no_ack:
            continue;

        send_retransmission:
            printf("%2d) Received DATA packet, Size: %d, Sequence: %ld, Corrupted\n", 
                execution_no++, received_pkt.data_size, received_pkt.sequence);
            struct packet retransmission_pkt;
            retransmission_pkt.type = RETRANSMISSION_TYPE;
            retransmission_pkt.sequence = received_pkt.sequence;
            if(sendto(socketfd, &retransmission_pkt, sizeof(struct packet), 0, (struct sockaddr *)&sender_addr, senderlen) < 0)
            {
                printf("ERROR sending retransmission packet");
                continue;
            }
            else {
                printf("%2d) Sent RETRANSMISSION packet, Sequence: %ld\n", 
                    execution_no++, retransmission_pkt.sequence);
                if (written_data_size == expected_data_size)
                    break;
            }
            continue;

      } //end of else if (received_pkt.type == DATA_TYPE)
     
    }//end of while 1

    fclose(fp);
    //close socket
    close(socketfd); 
    
    return 0;
}
