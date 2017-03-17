#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <strings.h>
#include <time.h>

#define MAX_PACKET_LEN 1024
#define MAX_SEQ_NUM 30720
#define WINDOW_SIZE 5120
#define RTO 500
#define HEADER_SIZE 20

struct Packet_Data {
    int seq_key;
    char whole_packet[MAX_PACKET_LEN];
};

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
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd; //Socket descriptor
    int portno, n;
    struct sockaddr_in cli_addr, serv_addr;
    struct hostent *server; //contains tons of information, including the server's IP address

    char buffer[MAX_PACKET_LEN];
    if (argc < 4) {
       fprintf(stderr,"usage %s hostname port filename\n", argv[0]);
       exit(0);
    }
    
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); //create a new socket
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    memset((char *) &cli_addr, 0, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET; //initialize server's address
    cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    cli_addr.sin_port = htons(0);
    
    if (bind(sockfd,(struct sockaddr *)&cli_addr,sizeof(cli_addr)) < 0) //establish a connection to the server
        error("ERROR bind failed");

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    int serv_len = sizeof(serv_addr);

    memset(buffer,0, MAX_PACKET_LEN);
    char* filename = argv[3];
    //printf("File to send: %s", filename);

//////////////////////////////////////////////////////////////////////////////////


    int num_structs = WINDOW_SIZE/MAX_PACKET_LEN;
    struct Packet_Data packet_array[num_structs]; //Number of packets saved. Number == window/packet len. Here it is 5
    
    struct Packet receivedData, filePacket, ackPacket;
    printf("Sending packet SYN\n");

    memcpy(filePacket.full_data, filename, strlen(filename));
    filePacket.size = strlen(filename);
    filePacket.syn = 1;
    int retrans = 0;
SEND:
    n = sendto(sockfd,&filePacket,sizeof(filePacket), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (n < 0) error("ERROR writing to socket");
    if(retrans == 1){
        printf("Sending packet SYN RETRANSMISSION\n");
    }
    retrans = 1;

    FILE * data;

   
    int seq_num = 0;
    int prev_seq_num = 0;
    char seq_string[HEADER_SIZE];
    int is_first = 1;
    
    time_t timer ;
    time(&timer);
    time_t timer2;
    time(&timer2);
    
    while(1){
        if(is_first == 1){
            if( 1000*(time(&timer)-timer2) > RTO) {
                //please don't judge me
                goto SEND;
            }
            
            memset((char*)&receivedData, 0, sizeof(receivedData));
            if(recvfrom(sockfd, &receivedData,sizeof(receivedData), MSG_DONTWAIT, (struct sockaddr *)&serv_addr, &serv_len) >= 0) { //read from the socket
                is_first = 0;
                seq_num = receivedData.sequence_num;
                printf("Receiving packet %d\n", seq_num);
                n = sendto(sockfd,seq_string,HEADER_SIZE, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                if (n < 0) {error("ERROR writing to socket");}
                printf("Sending packet %d\n", seq_num);
                
               
                prev_seq_num = seq_num;
                //printf("first packet");
                data = fopen("received.data", "a");
                fwrite(receivedData.full_data, 1, receivedData.size, data);
                fclose(data);
            }
        }
        else {
            memset((char*)&receivedData, 0, sizeof(receivedData));
            n = recvfrom(sockfd, &receivedData,sizeof(receivedData), 0, (struct sockaddr *)&serv_addr, &serv_len); //read from the socket
            if (n < 0) {error("ERROR reading from socket");}

            //now parse the buffer at the first colon
            int k;
        
            seq_num = receivedData.sequence_num;
            
            if((seq_num <= prev_seq_num + MAX_PACKET_LEN) || (seq_num <= MAX_PACKET_LEN)) {
                if(strcmp("FIN", receivedData.full_data) == 0){
                    printf("Receiving packet %d FIN\n", seq_num);
                    //TODO: what if this is dropped?
                    sendto(sockfd,"FINACK", 7, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                    printf("Sending packet %d FINACK\n", seq_num);
                    break;
                }
                else{
                    printf("Receiving packet %d\n", seq_num);
                    ackPacket.sequence_num = seq_num;
                    n = sendto(sockfd,&ackPacket,sizeof(ackPacket), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                    printf("Sending packet %d\n", seq_num);
                    if (n < 0) {error("ERROR writing from socket");}
                    
                    data = fopen("received.data", "a");
                    fwrite(receivedData.full_data, 1, receivedData.size, data);
                    fclose(data);
                }
                prev_seq_num = seq_num;
            }
            else {
                
                //for now just stop
                break;
                
                //packet out of order
                printf("Receiving packet %d\n", seq_num);
                n = sendto(sockfd,seq_string,HEADER_SIZE, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                if (n < 0) {error("ERROR writing from socket");}
                
                data = fopen("received.data", "a");
                fwrite(receivedData.full_data, 1, receivedData.size, data);
                fclose(data);
            }

        }
    }

    
    close(sockfd); //close socket
    return 0;
}
