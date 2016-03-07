/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>   /* for the waitpid() system call */
#include <signal.h> /* signal name macros, and the kill() prototype */


void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno, pid, window_size;
  double p_loss, p_corrupt;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  if (argc < 5) {
     fprintf(stderr,"Error: Not enough arguments\n");
     fprintf(stderr,"Run with arguments: ./sender <port number> <window size> <P(loss)> <P(corruption)>\n");
     exit(1);
  }

  portno = atoi(argv[1]);
  window_size = atoi(argv[2]);
  p_loss = atof(argv[3]);
  p_corrupt = atof(argv[4]);

  sockfd = socket(AF_INET, SOCK_DGRAM, 0); //create socket
  if (sockfd < 0) 
    error("ERROR opening socket");

  memset((char *) &serv_addr, 0, sizeof(serv_addr));    //reset memory
  //fill in address info
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *) &serv_addr,
          sizeof(serv_addr)) < 0) 
          error("ERROR on binding");

  long file_size;

  int output_buffer_size = 100;
  char output_buffer[output_buffer_size];
  int input_buffer_size = 1024;

  char file_name[100];
  char file_extension[10];

  listen(sockfd,5);  //5 simultaneous connection at most

  close(sockfd);
     
  return 0; 
}

