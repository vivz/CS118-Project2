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
#include <sys/fcntl.h>

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
  int i = 0, n = 0, j;
  int last_packet_index = 0;
  int execution_no = 0;
  double p_loss, p_corrupt;
  struct sockaddr_in sender_addr, receiver_addr;
  struct packet received_pkt;
  struct packet window_pkt;
  window_pkt.type = WINDOW_SIZE_TYPE;

  long filesize;
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
  fcntl(socketfd, F_SETFL, O_NONBLOCK);
  
  //reset memory
  memset((char *) &sender_addr, 0, sizeof(sender_addr));    

  //fill in address info
  sender_addr.sin_family = AF_INET;
  sender_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  sender_addr.sin_port = htons(portno);

  if (bind(socketfd, (struct sockaddr *) &sender_addr, sizeof(sender_addr)) < 0) 
    error("Error: bind failed");

  printf("Waiting on port %d\n", portno);
  printf("Packet size: %d\n", sizeof(struct packet));
  //constructing the packets
  int packets_per_window = (window_size * 1024) / sizeof(struct packet);
  struct packet packet_array[packets_per_window];
  window_pkt.data_size = window_size;
  send_tail = packets_per_window - 1;
  time_t timestamps[packets_per_window];
  time_t current_time;
	time(&current_time);

  for (i = 0; i < packets_per_window; i++)
  {
  	// initializing times to a year from now
  	timestamps[i] = current_time + 365 * 24 * 60 * 60;
  }
  while (1) 
  {
  	loop:
    //receiving a packet
    receive_length = recvfrom(socketfd, &received_pkt, 
      sizeof(received_pkt), 
      0, (struct sockaddr *)&receiver_addr, 
      &receiver_addr_len);

    if (receive_length < 0)
    	goto timer;

    if (received_pkt.type == RETRANSMISSION_TYPE || received_pkt.type == ACK_TYPE)
    {
    }
    //begining of a file transmission
    if (received_pkt.type == FILENAME_TYPE) 
    {
      //find the file
      char* filename = received_pkt.data;
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
          printf("%2d) Sent WINDOW_SIZE packet, Window Size: %d, Filesize: %ld, Packets per window: %d\n", 
          	execution_no++, window_size, filesize, packets_per_window);
      }

      //Initialize the first packet array
      for (i = 0; i < packets_per_window; i++)
      {
          packet_array[i].type = DATA_TYPE;
          packet_array[i].sequence = ftell(file_p) % MAX_SEQUENCE_NUMBER;
          packet_array[i].data_size = fread(packet_array[i].data, 1, PACKET_DATA_SIZE, file_p);
      		last_packet_index = i;

      		// case where file is smaller than initial window
          if (feof(file_p))
          {
          		break;
          }
      }

      printf("last_packet_index: %d\n", last_packet_index);
      // /* Comment out if doing out of order testing
      // initial sending of packets

      if (PRINT_SEND_WINDOW_STATUS)
        printPacketArray(packet_array, packets_per_window);

      for (i = 0; i <= last_packet_index; i++)
      {
        if (sendto(socketfd, &packet_array[i], sizeof(struct packet), 0, (struct sockaddr *)&receiver_addr, receiver_addr_len) < 0) 
        {
            printf("Error writing to socket\n");
            break;
        }
        else 
        {
            timestamps[i] = time(NULL);
            printf("%2d) Sent DATA packet, Sequence: %ld, Timestamp: %ld\n", 
            	execution_no++, packet_array[i].sequence, timestamps[i]);
            if (PRINT_DATA)
            {
              printf("Data: \n%s\n", packet_array[i].data);
            }
        }
      }
    }  //end of if (received_pkt.type == FILENAME_TYPE)
    else if (received_pkt.type == ACK_TYPE) 
    {
      float random_prob1 = (rand() % 10000) / 10000.0;
      if (random_prob1 < p_corrupt) {
          printf("%2d) Received ACK packet, Sequence: %ld, Corrupted\n", 
              execution_no++, received_pkt.sequence);
          continue;
      }
      
      float random_prob2 = (rand() % 10000) / 10000.0;
      if (random_prob2 < p_loss) {
          printf("%2d) Received ACK packet, Sequence: %ld, Lost - Dropping packet\n", 
              execution_no++, received_pkt.sequence);
          continue;
      }

      printf("%2d) Received ACK packet, Sequence: %ld,\n", 
        execution_no++, received_pkt.sequence);

      acknowledged_sent_size += received_pkt.data_size;
      // printf("acknowledged file size: %ld\n", acknowledged_sent_size);
      if (acknowledged_sent_size == filesize) 
      {
      	printf("%2d) Full file size acknowledged: %ld\n", execution_no++, filesize);
      	exit(0);
      }

      // if ACK is for the base, read into new base, send that, move base forward
      if (received_pkt.sequence == packet_array[send_base].sequence) 
      {
      	while (received_pkt.sequence == packet_array[send_base].sequence ||
      		 		 packet_array[send_base].type == ACK_TYPE)
	      {
	      	if (feof(file_p))
	      	{
	      		packet_array[send_base].type = ACK_TYPE;
	      		// for (i = 0; i < packets_per_window; i++) 
	      		// {
	      		// 	packet_array[i].type = ACK_TYPE;
	      		// }
	      		break;
	      	}
	      	packet_array[send_base].type = DATA_TYPE;
	        packet_array[send_base].sequence = ftell(file_p) % MAX_SEQUENCE_NUMBER;
	        packet_array[send_base].data_size = fread(packet_array[send_base].data, 1, PACKET_DATA_SIZE, file_p);
	        if (sendto(socketfd, &packet_array[send_base], sizeof(struct packet), 0, (struct sockaddr *)&receiver_addr, receiver_addr_len) < 0) 
	        {
	          printf("Error: sending packet after window shift\n");
	          // TODO - not sure if correct, may need to do some recovery here
	          // or maybe this will just be okay
	          break;
	        }
	        else 
	        {
            timestamps[send_base] = time(NULL);
            printf("%2d) Sent DATA packet, Sequence: %ld, Timestamp: %ld\n", 
            	execution_no++, packet_array[send_base].sequence, timestamps[send_base]);
	          if (PRINT_DATA)
	            printf("Data: \n%s\n", packet_array[send_base].data);
	          send_base = (send_base + 1) % packets_per_window;
	        }
	      }
      }
      // ACK is not the base
      else 
      {
      	// find the packet in the send window that was ack'd, then set it to ACK
      	for (i = (send_base + 1) % packets_per_window; i != send_base; i = (i + 1) % packets_per_window)
      	{
      		if (packet_array[i].sequence == received_pkt.sequence)
      		{
      			packet_array[i] = received_pkt;
      			break;
      		}
      	}
      }
      if (PRINT_SEND_WINDOW_STATUS)
        printPacketArray(packet_array, packets_per_window);

    } // end of ACK_TYPE case

    else if (received_pkt.type == RETRANSMISSION_TYPE)
    {
      float random_prob1 = (rand() % 10000) / 10000.0;
      if (random_prob1 < p_corrupt) {
          printf("%2d) Received RETRANSMISSION packet, Sequence: %ld, Corrupted\n", 
              execution_no++, received_pkt.sequence);
          continue;
      }
      
      float random_prob2 = (rand() % 10000) / 10000.0;
      if (random_prob2 < p_loss) {
          printf("%2d) Received RETRANSMISSION packet, Sequence: %ld, Lost - Dropping packet\n", 
              execution_no++, received_pkt.sequence);
          continue;
      }

      printf("%2d) Received RETRANSMISSION packet, Sequence: %ld\n", 
        execution_no++, received_pkt.sequence);

      for (i = 0; i < packets_per_window; i++) 
      {
      	if (packet_array[i].sequence == received_pkt.sequence) 
      	{
      		// printf("found packet in send window\n");
	        if (sendto(socketfd, &packet_array[i], sizeof(struct packet), 0, (struct sockaddr *)&receiver_addr, receiver_addr_len) < 0) 
	        {
	        	printf("Error resending packet\n");
	        }
	        else 
	        {
            timestamps[i] = time(NULL);
            printf("%2d) Re-sent DATA packet, Sequence: %ld, Timestamp: %ld\n", 
            	execution_no++, packet_array[i].sequence, timestamps[i]);
	        }

      	}
      }
    }
    else 
    {
      printf("%2d) received a packet\n", execution_no++);
    }

  } // end while

  timer:
  	time(&current_time);
  	for (i = 0; i < packets_per_window; i++) 
  	{
  		if ((current_time - timestamps[i]) > TIMEOUT && packet_array[i].type == DATA_TYPE)
  		{
        if (sendto(socketfd, &packet_array[i], sizeof(struct packet), 0, (struct sockaddr *)&receiver_addr, receiver_addr_len) < 0) 
        {
            printf("Error writing to socket\n");
            break;
        }
        else 
        {
        		timestamps[i] = time(NULL);
            printf("%2d) Timeout. Re-sent DATA packet, Sequence: %ld\n", execution_no++, packet_array[i].sequence);
            if (PRINT_DATA)
              printf("Data: \n%s\n", packet_array[i].data);
        }
  		}
  	}
  	goto loop;

}

