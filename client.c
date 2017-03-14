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
    
    printf("Sending packet SYN\n");

    n = sendto(sockfd,filename,strlen(filename), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (n < 0) error("ERROR writing to socket");
    char packet_data[MAX_PACKET_LEN - HEADER_SIZE];
    memset(packet_data, 0, MAX_PACKET_LEN - HEADER_SIZE);

    FILE * data;

   
    int seq_num = 0;
    int prev_seq_num = 0;
    char seq_string[HEADER_SIZE];
    int is_first = 1;
    struct Packet receivedData;
    
    while(1){
        memset(buffer,0,MAX_PACKET_LEN);
        memset(seq_string, 0, HEADER_SIZE);
        memset(packet_data,0,MAX_PACKET_LEN-HEADER_SIZE);
        
        memset((char*)&receivedData, 0, sizeof(receivedData));
        n = recvfrom(sockfd, &receivedData,sizeof(receivedData), 0, (struct sockaddr *)&serv_addr, &serv_len); //read from the socket
        if (n < 0) {error("ERROR reading from socket");}
          //printf("buffer: %s", buffer);
        //now parse the buffer at the first colon
        int k;
        int has_seen_col = 0;
        
        //printf("buffer: %s\n", buffer);
        
        /*
        for (k = 0; k < strlen(buffer); k++){
            if(has_seen_col == 0){
                char tmp2[4];
                sprintf(tmp2, "%c", buffer[k]);
               
                //printf("tmp2: %s", tmp2);
                
                if(strcmp(tmp2, ":") == 0){
                    has_seen_col = 1;
                    //break;
                }
                else {
                    strcat(seq_string, tmp2);
                }
                
            }
            else {
                //char tmp[4];
                //printf("buffer k: %c, %d\n", buffer[k], k);
                //sprintf(tmp, "%c", buffer[k]);
                //strcat(packet_data, tmp);
                //printf("packet k: %s, %d\n", packet_data, k);
                bzero(packet_data, 1004);
                //printf("seq string: %s", seq_string);
                //printf("buff: %s\n", buffer);
                memcpy(packet_data, buffer + strlen(seq_string) + 1, 1004);
            }
            
        }
         */
       
    
        //seq_num = atoi(seq_string);
        seq_num = receivedData.sequence_num;
        //memcpy(packet_data, buffer+k+1, 21);
       // packet_data = receivedData.full_data;
        //printf("seq: %d\n", seq_num);
        //printf("packet data: %s\n", packet_data);
        if(strcmp("FIN", packet_data) == 0){
            printf("Receiving packet %d FIN\n", seq_num);
            sendto(sockfd,"FINACK", 7, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            break;
        }
        else{
            printf("Receiving packet %d\n", seq_num);
            n = sendto(sockfd,seq_string,HEADER_SIZE, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            if (n < 0) {error("ERROR writing from socket");}
            
            //printf("first packet");
            data = fopen("received.data", "a");

            //data = fopen("received2.data", "a");
            // fwrite(packet_data, sizeof(char), 21, data); // will need to update seq_num in both places
            // fclose(data);

            //printf("packet_data %s, %lu", packet_data, strlen(packet_data));
            //fputs(buffer+k+1, data); // MIGHT NEED TO UPDATE TO FWRITE
            fwrite(receivedData.full_data, 1, receivedData.size, data);
            fclose(data);

        }
    }

    
    close(sockfd); //close socket
    return 0;
}
