
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
    char *filename, *hostname;
    double p_loss, p_corrupt;
    struct sockaddr_in sender_addr;
    socklen_t senderlen = sizeof(sender_addr);
    FILE* fp;
    // contains tons of information, including the server's IP address
    struct hostent *server; 
    char buffer[256];
    struct packet filename_pkt, received_pkt;
    int baselength = sizeof(filename_pkt.type) * 3 + 1;
    
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
    
    printf("Requested file %s\n", filename);

    char* new_filename = strcat(filename, "_copy");
    fp = fopen(new_filename, "wb");


    while(1){
      receive_length = recvfrom(socketfd, &received_pkt, sizeof(received_pkt), 0, (struct sockaddr *)&sender_addr, &senderlen);
      if (receive_length < 0)
      {
        printf("Error receiving a packet\n");
        break;
      }   
      else
      {
        printf("data received: %s\n", received_pkt.data);
      }
      n = fwrite(received_pkt.data,received_pkt.data_size,1,fp);
      if(n < 0)
      {
        printf("Error writing to the file\n");
        break;
      }

    }

    fclose(fp);
    //close socket
    close(socketfd); 
    
    return 0;
}
