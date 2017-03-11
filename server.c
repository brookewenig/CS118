#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <time.h>

#define MAX_PACKET_LEN 1024
#define MAX_SEQ_NUM 30720
#define WINDOW_SIZE 5120
#define RTO 500
#define HEADER_SIZE 20

void error(char *msg)
{
    perror(msg);
    exit(1);
}

struct Packet_Data {
    int seq_key;
    char whole_packet[MAX_PACKET_LEN];
    time_t start_time;
};

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
  int seq_num = 0;//rand() % (MAX_SEQ_NUM + 1);
  char ack_num[HEADER_SIZE];
  int num_structs = WINDOW_SIZE/MAX_PACKET_LEN;
  
  struct Packet_Data packet_array[num_structs]; //Number of packets saved. Number == window/packet len. Here it is 5
  int ack_num_received[num_structs];

  int l;
  for (l =0; l<num_structs; l++){
      packet_array[l].seq_key = -2;
      ack_num_received[l] = -1;
  }
    
  while(1){
      
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
    
    int k;
    FILE *fp;
    fp = fopen(buffer, "rb"); //filename
    int status = 0;
    if(fp != NULL) {
      status = 1;
      int isfirst = 0;
    //the number of total packet to send, divided into groups based on window
      while(!feof(fp)){//for(j = 0; j < (file_length/(MAX_PACKET_LEN-HEADER_SIZE))/(WINDOW_SIZE/MAX_PACKET_LEN) + 40; j ++)
            //by default, should be 5 (send 5 packets at once)
            for(k = 0; k < WINDOW_SIZE/MAX_PACKET_LEN; k++)
            {
                if(seq_num > MAX_SEQ_NUM) {
                    seq_num = seq_num - MAX_SEQ_NUM;
                }
                memset(buffer, 0, MAX_PACKET_LEN-HEADER_SIZE);
                memset(packet, 0, MAX_PACKET_LEN);
                
                //indicates EOF
                if(feof(fp)){
                    break;
                }
                //put header into a buffer
                char header_buffer[HEADER_SIZE];
                snprintf(header_buffer, HEADER_SIZE, "%d", seq_num);
                strcat(header_buffer, ":");
                fread(buffer, sizeof(char), MAX_PACKET_LEN-HEADER_SIZE-1, fp);
                strcpy(packet, header_buffer);
                strcat(packet, buffer);

                int j;
                for (j=0; j< num_structs; j++){
                  printf("J: Packet Array %d\n",packet_array[j].seq_key);
                }

                if (isfirst !=0) {
                  int f;
                  int found_ack = 0;
                  for (f = 0; f < num_structs; f++) {
                      if (packet_array[k].seq_key == ack_num_received[f]){
                          found_ack = 1;
                          printf("Same ACK: %d, K: %d\n", ack_num_received[f], k);
                      }
                  }
                  //can't detect until k wraps, so we need detect if this is curr k or next
                  if (found_ack == 0) {
                      printf("In found ack");
                      if( 1000*(time(NULL)-packet_array[k].start_time) > RTO){
                          // RETRANSMIT!!!
                          printf("Sending packet %d %d Retransmission\n", packet_array[k].seq_key, WINDOW_SIZE);
                          n = sendto(sockfd,packet_array[k].whole_packet,MAX_PACKET_LEN, 0, (struct sockaddr *)&cli_addr, clilen);
                          if (n < 0) error("ERROR reading from socket");
                      }
                    }
                }

                
                packet_array[k].seq_key = seq_num;
                strcpy(packet_array[k].whole_packet, packet);
                packet_array[k].start_time = time(NULL);
                
                
                //printf("Packet: %s", packet);
                if(was_first == 0 && sent_syn == 0) {
                    printf("Sending packet %d %d\n", seq_num, WINDOW_SIZE);
                }
                sent_syn = 0;
                //OK so we're sending a packet with sequence number, colon, data
                n = sendto(sockfd,packet,MAX_PACKET_LEN, 0, (struct sockaddr *)&cli_addr, clilen);
                if (n < 0) error("ERROR reading from socket");
                n = recvfrom(sockfd,ack_num, MAX_PACKET_LEN, 0, (struct sockaddr *)&cli_addr, &clilen);
                if (!(n < 0)) {
                  ack_num_received[k] = atoi(ack_num);
                }
                seq_num += strlen(buffer) + 1;
            }
            isfirst = 1;
       }
        
      // if (atoi(ack_num) == seq_num){
      //         seq_num += sizeof(buffer);
      // }
    }
    else {
        if (status == 0) {
            printf("Nope. Try another file.");
        }
    }
    char header_buffer[HEADER_SIZE];
    memset(header_buffer, 0, HEADER_SIZE);
    snprintf(header_buffer, HEADER_SIZE, "%d", seq_num);
    strcat(header_buffer, ":FIN");
      
    fclose(fp);
    n = sendto(sockfd,header_buffer,HEADER_SIZE, 0, (struct sockaddr *)&cli_addr, clilen);
    if (n < 0) {
        error("ERROR writing to socket");
    }
    char wait[20];
    printf("Sending packet %d %d FIN\n", seq_num, WINDOW_SIZE);
    if(recvfrom(sockfd,wait, MAX_PACKET_LEN, 0, (struct sockaddr *)&cli_addr, &clilen) >= 0){
        printf("Receiving packet FINACK \n");
        //clock_t timer_start = clock();//(clock()/CLOCKS_PER_SEC)/1000;
        time_t timer ;
        time(&timer);
        time_t timer2;
        time(&timer2);
        //timer = timer - timer2;
        //printf("TIMEEE: %d", timer);
        while(1) {
            if( 1000*(time(&timer)-timer2) > 2*RTO){
                break;
            }
        }
    }
   //break;
  }
     
 close(sockfd); 
 return 0; 
}

