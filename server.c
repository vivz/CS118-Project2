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
  int sockfd, newsockfd, portno, pid;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  if (argc < 2) {
     fprintf(stderr,"ERROR, no port provided\n");
     exit(1);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, 0); //create socket
  if (sockfd < 0) 
    error("ERROR opening socket");
  memset((char *) &serv_addr, 0, sizeof(serv_addr));    //reset memory
  //fill in address info
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
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

  while(1) {

    //accept connections
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
       
    if (newsockfd < 0) 
      error("ERROR on accept");
       
    int n;
    char buffer[input_buffer_size];
        
    memset(buffer, 0, input_buffer_size);  //reset memory

    //read client's message
    n = read(newsockfd,buffer,input_buffer_size);
    if (n < 0) error("ERROR reading from socket");

    //Find what file is being requested
    char requestedfile[100];
    sscanf(buffer, "GET /%s HTTP/1.1", requestedfile);
    // printf("%s\n", requestedfile);

    FILE *filep = fopen(requestedfile, "r");

    if (filep == NULL) {
      n = write(newsockfd, "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<html>This is my 404 page: 404 Not Found</html>", 95);
    } 

    else {
      // printf("file found\n");
      // printf("Here is the message: %s\n",buffer);
      n = write(newsockfd, "HTTP/1.1 200 OK\n", 16);

      char* type;
      int typelength;
      if(strstr(requestedfile, ".jpg") || strstr(requestedfile, ".jpeg")) {
        type = "Content-Type: image/JPEG";
        typelength = 24;
      } else if (strstr(requestedfile, ".html")) {
        type = "Content-Type: text/HTML";
        typelength = 23;
      } else if (strstr(requestedfile, ".gif")) {
        type = "Content-Type: image/GIF";
        typelength = 23;
      } else {
        type = "Content-Type: text/plain";
        typelength = 24;
      }
      // "HTTP/1.1 200 OK\n" //16
      long written_size = 0;
      fseek (filep , 0 , SEEK_END);
      file_size = ftell (filep);
      rewind (filep);

      n = write(newsockfd, type, typelength);
      n = write(newsockfd, "\n\n", 2);
      
      // if (file_size < output_buffer_size) {
      //   fread(output_buffer, 1, file_size, filep);
      //   n = write(newsockfd, output_buffer, file_size);
      // }

      // else {
        while (written_size < file_size){
          //reply to client
          memset(output_buffer, 0, output_buffer_size);
          fread(output_buffer, 1, output_buffer_size, filep);

          n = write(newsockfd,output_buffer, output_buffer_size);
          if (n < 0) error("ERROR writing to socket");      
          written_size += output_buffer_size;      
        }
      // }
      close(newsockfd);//close connection 

    }
  }
  close(sockfd);
     
  return 0; 
}

