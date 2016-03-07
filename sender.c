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

#include "common.h"

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
  int socketfd, newsocketfd, portno, pid, window_size;
  int receive_length;
  int send_base = 0, send_tail = 0;
  int i = 0, n = 0;
  double p_loss, p_corrupt;
  struct sockaddr_in sender_addr, receiver_addr;
  struct packet received_pkt;
  socklen_t receiver_addr_len = sizeof(receiver_addr);

  FILE *file_p;

  if (argc < 5) {
     fprintf(stderr,"Error: Not enough arguments\n");
     fprintf(stderr,"Usage: %s <port number> <window size> <P(loss)> <P(corruption)>\n", argv[0]);
     exit(1);
  }

  portno = atoi(argv[1]);
  window_size = atoi(argv[2]);
  p_loss = atof(argv[3]);
  p_corrupt = atof(argv[4]);


  //create socket
  socketfd = socket(AF_INET, SOCK_DGRAM, 0); 
  if (socketfd < 0) 
    error("Error: opening socket failed");
  
  //reset memory
  memset((char *) &sender_addr, 0, sizeof(sender_addr));    

  //fill in address info
  sender_addr.sin_family = AF_INET;
  sender_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  sender_addr.sin_port = htons(portno);

  if (bind(socketfd, (struct sockaddr *) &sender_addr, sizeof(sender_addr)) < 0) 
    error("Error: bind failed");

  printf("waiting on port %d\n", portno);

  //constructing the packets
  int packets_per_window = (window_size * 1024) / sizeof(struct packet);
  struct packet packet_array[packets_per_window];

  while (1) {
    receive_length = recvfrom(socketfd, &received_pkt, 
      sizeof(received_pkt), 
      0, (struct sockaddr *)&receiver_addr, 
      &receiver_addr_len);

    if (received_pkt.type == FILENAME_TYPE) {
      char* filename = received_pkt.data;
      printf("received filename packet for: %s\n", filename);
      file_p = fopen(filename, "r");

      if (file_p == NULL) {
        printf("Error: file not found\n");
        continue;
      }
      for (i=0; i<packets_per_window; i++)
      {
          packet_array[i].type = DATA_TYPE;
          packet_array[i].sequence =  ftell(file_p);
          packet_array[i].data_size = fread(packet_array[i].data, 1, PACKET_DATA_SIZE, file_p);

          n = sendto(socketfd, &packet_array[i], sizeof(struct packet), 0, (struct sockaddr *)&receiver_addr, receiver_addr_len);
          if (n < 0) {
            printf("Error writing to socket\n");
            break;
          }
          else {
            printf("sent packet:\n");
            printPacket(packet_array[i]);
          }

          send_tail++;
          if(feof(file_p))
            break;
      }
      //printPacketArray(packet_array, send_tail);


      // not sure if this goes here
      // fclose(file_p);
    }
    else {
      printf("received a packet\n");
    }

  }

  return 0; 
}

