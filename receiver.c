
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
    int sockfd; //Socket descriptor
    int portno, n;
    double p_loss, p_corrupt;
    struct sockaddr_in sender_addr;
    struct hostent *server; //contains tons of information, including the server's IP address

    char buffer[256];
    if (argc < 6) {
       fprintf(stderr,"usage: %s <hostname> <port> <filename> <p(loss)> <p(corruption)>\n", argv[0]);
       exit(0);
    }
    
    portno = atoi(argv[2]);
    p_loss = atof(argv[4]);
    p_corrupt = atof(argv[5]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0); //create a new socket
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    server = gethostbyname(argv[1]); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    memset((char *) &sender_addr, 0, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&sender_addr.sin_addr.s_addr, server->h_length);
    sender_addr.sin_port = htons(portno);
    
    if (connect(sockfd,(struct sockaddr *)&sender_addr,sizeof(sender_addr)) < 0) //establish a connection to the server
        error("ERROR connecting");
    
    printf("Please enter the message: ");
    memset(buffer,0, 256);
    fgets(buffer,255,stdin);    //read message
    
    n = write(sockfd,buffer,strlen(buffer)); //write to the socket
    if (n < 0) 
         error("ERROR writing to socket");
    
    memset(buffer,0,256);
    n = read(sockfd,buffer,255); //read from the socket
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",buffer);  //print server's response
    
    close(sockfd); //close socket
    
    return 0;
}
