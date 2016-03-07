
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

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int socketfd; //Socket descriptor
    int portno, n;
    char *filename, *hostname;
    double p_loss, p_corrupt;
    struct sockaddr_in sender_addr;
    socklen_t senderlen = sizeof(sender_addr);  
    struct hostent *server; //contains tons of information, including the server's IP address
    char buffer[256];
    
    if (argc < 6) {
       fprintf(stderr,"usage: %s <hostname> <port> <filename> <p(loss)> <p(corruption)>\n", argv[0]);
       exit(0);
    }
    
    hostname = argv[1];
    portno = atoi(argv[2]);
    filename = argv[3];
    p_loss = atof(argv[4]);
    p_corrupt = atof(argv[5]);
    
    socketfd = socket(AF_INET, SOCK_DGRAM, 0); //create a new socket
    if (socketfd < 0) 
        error("ERROR opening socket");
    
    server = gethostbyname(hostname); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    /* bind the socket to any valid IP address and a specific port */
    memset((char *) &sender_addr, 0, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&sender_addr.sin_addr.s_addr, server->h_length);
    sender_addr.sin_port = htons(portno);
        
    
    memset(buffer,0,256);
    strcpy(buffer, filename);
    n = sendto(socketfd, buffer, strlen(buffer), 0, (struct sockaddr *)&sender_addr, senderlen);
    if (n < 0) 
         error("ERROR writing to filename socket");
    
    printf("Requested file %s\n", filename);
    
    close(socketfd); //close socket
    
    return 0;
}
