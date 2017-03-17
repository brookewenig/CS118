#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h> /* for the waitpid() system call */
#include <signal.h> /* signal name macros, and the kill() prototype */
#include <time.h>

#define MAX_PACKET_LEN 1024
#define MAX_SEQ_NUM 30720
#define WINDOW_SIZE 5120
#define RTO 500
#define HEADER_SIZE 20

struct Packet {
    int sequence_num;
    char full_data[MAX_PACKET_LEN-HEADER_SIZE];
    int size;
    int syn;
    int fin;
};

void error(char *msg)
{
    perror(msg);
    exit(1);
}

struct Packet_Data {
    struct Packet packet;
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
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);  //create socket
  if (sockfd < 0) error("ERROR opening socket");
  memset((char *) &serv_addr, 0, sizeof(serv_addr));  //reset memory

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
  struct Packet fileToSend, filePacket, ackPacket;
  struct Packet_Data packet_data_array[5];
    int num_packets_in_channel = 0;
    
  while(1){
      seq_num = 0;
    int n;
    char buffer[MAX_PACKET_LEN-HEADER_SIZE];
    char packet[MAX_PACKET_LEN];
      
     
    memset(buffer, 0, MAX_PACKET_LEN-HEADER_SIZE);  //reset memory
    memset(packet, 0, MAX_PACKET_LEN);

      printf("got up");
    //read client's message
    n = recvfrom(sockfd,&filePacket, MAX_PACKET_LEN-1, 0, (struct sockaddr *)&cli_addr, &clilen);
      if (filePacket.syn == 1){
          printf("Sending packet %d %d SYN\n", seq_num, WINDOW_SIZE);
          was_first = 0;
          sent_syn = 1;
      }
    if (n < 0) error("ERROR reading from socket");
    
    int k;
    FILE *fp;
    fp = fopen(filePacket.full_data, "rb"); //filename
    
    int status = 0;
      
    int ack_num_array[5];

    if(fp != NULL) {
      status = 1;
      int isfirst = 0;

    //the number of total packet to send, divided into groups based on window
      while(!feof(fp)){//for(j = 0; j < (file_length/(MAX_PACKET_LEN-HEADER_SIZE))/(WINDOW_SIZE/MAX_PACKET_LEN) + 40; j ++)
            //by default, should be 5 (send 5 packets at once)
          if(isfirst == 0) {
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

                size_t val = fread(buffer, sizeof(char), MAX_PACKET_LEN-HEADER_SIZE-1, fp);

                if(was_first == 0 && sent_syn == 0) {
                    printf("Sending packet %d %d\n", seq_num, WINDOW_SIZE);
                }
                fileToSend.sequence_num = seq_num;
                fileToSend.size = val;
                memcpy(fileToSend.full_data, buffer, fileToSend.size);
                
                packet_data_array[k].packet = fileToSend;
                packet_data_array[k].start_time = time(NULL);
                
                
                sent_syn = 0;
                //OK so we're sending a packet struct
                if(num_packets_in_channel <= 5) {
                    n = sendto(sockfd,&fileToSend,sizeof(fileToSend), 0, (struct sockaddr *)&cli_addr, clilen);
                    if (n < 0) error("ERROR reading from socket");
                    num_packets_in_channel++;
                }
                
                /*
                if(recvfrom(sockfd,&ackPacket, MAX_PACKET_LEN, 0, (struct sockaddr *)&cli_addr, &clilen) >= 0) {
                    printf("Receiving packet %d\n", ackPacket.sequence_num);
                    num_packets_in_channel--;
                    ack_num_array[k] = ackPacket.sequence_num;
                    int q, w;
                    for(q = 0; q < 5; q++){
                        for(w = 0; w < 5; w++){
                            if (ack_num_array[q] == packet_data_array[w].packet.sequence_num){
                                // Checking to see if received an ack for any of the previously sent packets
                                ack_num_array[q] = -1;
                                packet_data_array[w].packet.sequence_num = seq_num + val;
                            }
                        }
                    }
                }
                 */
                seq_num += val;
            }
          }
            isfirst = 1;
          if(recvfrom(sockfd,&ackPacket, MAX_PACKET_LEN, 0, (struct sockaddr *)&cli_addr, &clilen) >= 0) {
              printf("Receiving packet %d\n", ackPacket.sequence_num);
              
              if(seq_num > MAX_SEQ_NUM) {
                  seq_num = seq_num - MAX_SEQ_NUM;
              }
              
              if(feof(fp)){
                  break;
              }
              
              size_t val = fread(buffer, sizeof(char), MAX_PACKET_LEN-HEADER_SIZE-1, fp);
              
              if(was_first == 0 && sent_syn == 0) {
                  printf("Sending packet %d %d\n", seq_num, WINDOW_SIZE);
              }
              fileToSend.sequence_num = seq_num;
              fileToSend.size = val;
              memcpy(fileToSend.full_data, buffer, fileToSend.size);
              
              packet_data_array[k].packet = fileToSend;
              packet_data_array[k].start_time = time(NULL);
              
              
             
              n = sendto(sockfd,&fileToSend,sizeof(fileToSend), 0, (struct sockaddr *)&cli_addr, clilen);
              if (n < 0) error("ERROR writing to socket");
               seq_num += val;
              
          }
       }
    }

    fclose(fp);

    char header_buffer[HEADER_SIZE];
    memset(header_buffer, 0, HEADER_SIZE);
    strcat(header_buffer, "FIN");
      
    fileToSend.sequence_num = seq_num;
    fileToSend.size = 4; //sizeof(buffer);
    memcpy(fileToSend.full_data, header_buffer, fileToSend.size);
    
  SEND:
    n = sendto(sockfd,&fileToSend,sizeof(fileToSend), 0, (struct sockaddr *)&cli_addr, clilen);
    printf("Sending packet %d %d FIN\n", seq_num, WINDOW_SIZE);
    //n = sendto(sockfd,header_buffer,HEADER_SIZE, 0, (struct sockaddr *)&cli_addr, clilen);
    if (n < 0) {
        error("ERROR writing to socket");
    }
      char wait[20];
      time_t timer;
      time(&timer);
      time_t timer2;
      time(&timer2);
      
      while(1) {
          if(recvfrom(sockfd, wait, MAX_PACKET_LEN, MSG_DONTWAIT, (struct sockaddr *)&cli_addr, &clilen) >= 0){
              printf("Receiving packet FINACK \n");
              //TIMED WAIT
              time(&timer);
              time(&timer2);
              while(1) {
                  if( 1000*(time(&timer)-timer2) > 2*RTO){
                      break;
                  }
              }
              break;
          }
          
        if( 1000*(time(&timer)-timer2) > RTO) {
            //please don't judge me
            goto SEND;
        }
          
      }
    else {
        printf("Nope. Try another file.");
        break;
    }
      

  }

    
 close(sockfd);
 return 0; 
}
