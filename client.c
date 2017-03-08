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
    
    while(1){
        memset(buffer,0,MAX_PACKET_LEN);
        memset(seq_string, 0, HEADER_SIZE);
        memset(packet_data,0,MAX_PACKET_LEN-HEADER_SIZE);
        
        n = recvfrom(sockfd,buffer,MAX_PACKET_LEN-1, 0, (struct sockaddr *)&serv_addr, &serv_len); //read from the socket
        if (n < 0) {error("ERROR reading from socket");}
          //printf("buffer: %s", buffer);
        //now parse the buffer at the first colon
        int k;
        int has_seen_col = 0;
        
        
        for (k = 0; k < strlen(buffer); k++){
            if(has_seen_col == 0){
                char tmp2[4];
                sprintf(tmp2, "%c", buffer[k]);
               
                //printf("tmp2: %s", tmp2);
                
                if(strcmp(tmp2, ":") == 0){
                    has_seen_col = 1;
                }
                else {
                    strcat(seq_string, tmp2);
                }
                
            }
            else {
                char tmp[4];
                sprintf(tmp, "%c", buffer[k]);
                strcat(packet_data, tmp);
            }
            
        }
    
        //printf("made it through loop");
        seq_num = atoi(seq_string);
        
        //printf("seq: %d\n", seq_num);
        //printf("packet data: %s\n", packet_data);
        if(strcmp("FIN", packet_data) == 0){
            printf("Receiving packet %d FIN\n", seq_num);
            break;
        }
        else{
            printf("Receiving packet %d\n", seq_num);
            n = sendto(sockfd,seq_string,HEADER_SIZE, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            if (n < 0) {error("ERROR writing from socket");}
            
            if (prev_seq_num == 0) {
                //printf("first packet");
                data = fopen("received.data", "a");
                //printf("packet_data %s", packet_data);
                fprintf(data, packet_data);
                fclose(data);
                prev_seq_num = seq_num;
            }

            else{//else if (seq_num == prev_seq_num + (strlen(packet_data) +1)) {
                //printf("seq_num: %d; prev_seq_num: %d; packet data length: %d\n", seq_num, prev_seq_num, strlen(packet_data));
                // Don't want to write unconditionally
                data = fopen("received.data", "a");
                //printf("packet_data %s", packet_data);
                fprintf(data, packet_data);
                fclose(data);
                prev_seq_num = seq_num;
            }
            /*else { // END OF FILE
                //printf("EOFFFFFF");
                data = fopen("received.data", "a");
                //printf("packet_data %s", packet_data);
                fprintf(data, packet_data);
                fclose(data);
                prev_seq_num = seq_num;
            } */
            
            ///FIXME
            
        }
    }

	
    close(sockfd); //close socket
    return 0;
}
