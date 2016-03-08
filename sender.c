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
  int send_base = 0, send_tail;
  int i = 0, n = 0;
  int execution_no = 0;
  double p_loss, p_corrupt;
  struct sockaddr_in sender_addr, receiver_addr;
  struct packet received_pkt;
  struct packet window_pkt;
  window_pkt.type = WINDOW_SIZE_TYPE;

  long acknowledged_sent_size = 0;

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

  printf("Waiting on port %d\n", portno);

  //constructing the packets
  int packets_per_window = (window_size * 1024) / sizeof(struct packet);
  struct packet packet_array[packets_per_window];
  window_pkt.data_size = packets_per_window;
  send_tail = packets_per_window - 1;

  while (1) {
    //receiving a packet
    receive_length = recvfrom(socketfd, &received_pkt, 
      sizeof(received_pkt), 
      0, (struct sockaddr *)&receiver_addr, 
      &receiver_addr_len);

    //begining of a file transmission
    if (received_pkt.type == FILENAME_TYPE) {
      //find the file
      char* filename = received_pkt.data;
      long filesize;
      printf("%2d) Received FILENAME packet, Filename: %s\n", execution_no++, filename);
      file_p = fopen(filename, "r");

      if (file_p == NULL) {
        printf("Error: file not found\n");
        exit(1);
      }
      else {
        // send the size of the file as the sequence number in the window packet
        // janky, I know
        fseek(file_p, 0, SEEK_END);
        filesize = ftell(file_p);
        rewind(file_p);
        window_pkt.sequence = filesize;
      }


      // let the receiver know our window size
      if (sendto(socketfd, &window_pkt, sizeof(struct packet), 0, (struct sockaddr *)&receiver_addr, receiver_addr_len) < 0) 
      {
          printf("Error: sending window packet\n");
          continue;
      }
      else {
          printf("%2d) Sent WINDOW_SIZE packet, Window Size: %d, Filesize: %ld\n", execution_no++, packets_per_window, filesize);
      }

      //Initialize the first packet array
      for (i=0; i<packets_per_window; i++)
      {
          //TODO: Wrap around sequence number when it exceeds
          packet_array[i].type = DATA_TYPE;
          packet_array[i].sequence =  ftell(file_p);
          packet_array[i].data_size = fread(packet_array[i].data, 1, PACKET_DATA_SIZE, file_p);
      }

      // /* Comment out if doing out of order testing
      // initial sending of packets
      for (i = 0; i < packets_per_window; i++)
      {
        //TODO: update send_base and send_tail

        if (sendto(socketfd, &packet_array[i], sizeof(struct packet), 0, (struct sockaddr *)&receiver_addr, receiver_addr_len) < 0) 
        {
            printf("Error writing to socket\n");
            break;
        }
        else 
        {
            printf("%2d) Sent DATA packet, Sequence: %ld\n", execution_no++, packet_array[i].sequence);
            if (PRINT_DATA)
              printf("Data: \n%s\n", packet_array[i].data);
        }
      }
      // */
      /*
      // testing out of order sending
      int j = 0; 
      int num_array[20]={0,1,3,4,5,6,2,7,8,9,10,11,12,13,14,15,16,17,18,19};
      for( j = 0; j< packets_per_window; j++)

      {
        //TODO: update send_base and send_tail
        // order: 0,2,3,4,5,6,2,7,8,9,10...
        i = num_array[j];
        if (sendto(socketfd, &packet_array[i], sizeof(struct packet), 0, (struct sockaddr *)&receiver_addr, receiver_addr_len) < 0) 
        {
            printf("Error writing to socket\n");
            break;
        }
        else 
        {
            if(packet_array[i].type == END_TYPE)
            {
              printf("%2d) Sent END_TYPE packet.\n", execution_no++);
              break;
            }
            printf("%2d) Sent DATA packet, Sequence: %ld\n", execution_no++, packet_array[i].sequence);
            if (PRINT_DATA)
              printf("Data: \n%s\n", packet_array[i].data);
        }
      }
      */

    }  //end of if (received_pkt.type == FILENAME_TYPE)
    else {

      if (received_pkt.type == ACK_TYPE) {
        printf("%2d) Received ACK packet, Sequence: %ld\n", execution_no++, received_pkt.sequence);
        // if ACK is for the base, read into new base, move the base forward, 
        // send from send_tail, then move that forward
      }
      else {
        printf("%2d) received a packet\n", execution_no++);
      }
    }

  }

  return 0; 
}

