#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */

#define MAX_PACKET_LEN 102
#define MAX_SEQ_NUM 30720
#define WINDOW_SIZE 5120
#define RTO 500
#define HEADER_SIZE 20

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
  clilen = sizeof(cli_addr);

  if (argc < 2) {
     fprintf(stderr,"ERROR, no port provided\n");
     exit(1);
  }
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);	//create socket
  if (sockfd < 0) error("ERROR opening socket");
  memset((char *) &serv_addr, 0, sizeof(serv_addr));	//reset memory

  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
          error("ERROR on binding");
	
//////////////////////////////////////////////////////////////////////////////////

  int sent_syn = 0;
    int was_first = 1;
    srand (time(NULL));
    int seq_num = rand() % (MAX_SEQ_NUM + 1);
  
  while(1){
      //wrap
      if(seq_num == MAX_SEQ_NUM) {
          seq_num = 0;
      }
    int n;
    char buffer[MAX_PACKET_LEN-HEADER_SIZE];
      char packet[MAX_PACKET_LEN];
      
     
    memset(buffer, 0, MAX_PACKET_LEN-HEADER_SIZE);	//reset memory
      memset(packet, 0, MAX_PACKET_LEN);

    //read client's message
    n = recvfrom(sockfd,buffer, MAX_PACKET_LEN-1, 0, (struct sockaddr *)&cli_addr, &clilen);
      if (was_first == 1){
          printf("Sending packet %d %d SYN\n", seq_num, WINDOW_SIZE);
          was_first = 0;
          sent_syn = 1;
      }
    if (n < 0) error("ERROR reading from socket");
    
    FILE *fp;
    fp = fopen(buffer, "rb"); //filename
    if(fp != NULL) {
      fseek(fp, 0, SEEK_END);
      long file_length = ftell(fp);
      rewind(fp);
    	memset(buffer, 0, MAX_PACKET_LEN-HEADER_SIZE);
        memset(packet, 0, MAX_PACKET_LEN);
        
        //put header into a buffer
        char header_buffer[HEADER_SIZE];
    	snprintf(header_buffer, HEADER_SIZE, "%d", seq_num);
        strcat(header_buffer, ":");
    	fread(buffer, sizeof(char), MAX_PACKET_LEN-HEADER_SIZE-1, fp);//-HEADER_SIZE
        strcpy(packet, header_buffer);
        strcat(packet, buffer);
        //printf("Packet: %s", packet);
        if(was_first == 0 && sent_syn == 0) {
            printf("Sending packet %d %d\n", seq_num, WINDOW_SIZE);
        }
        sent_syn = 0;
        //OK so we're sending a packet with sequence number, newline, data
    	n = sendto(sockfd,packet,MAX_PACKET_LEN-1, 0, (struct sockaddr *)&cli_addr, clilen);
    	seq_num++;
    	if (n < 0) {
    	  error("ERROR writing to socket");
    	}	
    }
    else {
  	 printf("Nope. Try another file.");
    }
    fclose(fp);
    printf("Sending packet %d %d FIN\n", seq_num, WINDOW_SIZE);
    break;
  }
     
 close(sockfd); 
 return 0; 
}

