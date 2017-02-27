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


  int seq_num = 0;
  while(1){
    int n;
    char buffer[MAX_PACKET_LEN];
     
    memset(buffer, 0, MAX_PACKET_LEN);	//reset memory

    //read client's message
    n = recvfrom(sockfd,buffer, MAX_PACKET_LEN-1, 0, (struct sockaddr *)&cli_addr, &clilen);
    printf("Sending packet %d %d SYN\n", seq_num, WINDOW_SIZE);
    if (n < 0) error("ERROR reading from socket");

    FILE *fp;
    fp = fopen(buffer, "rb"); //filename
    if(fp != NULL) {
      fseek(fp, 0, SEEK_END);
      long file_length = ftell(fp);
      rewind(fp);
    	memset(buffer, 0, MAX_PACKET_LEN);
    	//char buff1[HEADER_SIZE];
    	//snprintf(buff1, HEADER_SIZE, "%d", seq_num);	
    	fread(buffer, sizeof(char), MAX_PACKET_LEN-1, fp);//-HEADER_SIZE
    	//printf("buffer: %s", buffer);
    	printf("Sending packet %d %d\n", seq_num, WINDOW_SIZE);
    	n = sendto(sockfd,buffer,MAX_PACKET_LEN-1, 0, (struct sockaddr *)&cli_addr, clilen);
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

