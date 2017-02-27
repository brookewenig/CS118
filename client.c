#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <strings.h>

#define MAX_PACKET_LEN 102
#define MAX_SEQ_NUM 30720
#define WINDOW_SIZE 5120
#define RTO 500
#define HEADER_SIZE 20

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
        error("ERROR bind failed hihihihi");

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    int serv_len = sizeof(serv_addr);

    memset(buffer,0, MAX_PACKET_LEN);
    char* filename = argv[3];
    //printf("File to send: %s", filename);




    printf("Sending packet SYN\n");
    //n = sendto(sockfd,"SYN",4, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (n < 0) {error("ERROR sending to socket");}

    //n = recvfrom(sockfd,buffer,MAX_PACKET_LEN-1, 0, (struct sockaddr *)&serv_addr, &serv_len);
    //if (n < 0) {error("ERROR reading from socket");}
    printf("Received SYN From server");

    n = sendto(sockfd,filename,strlen(filename), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (n < 0) error("ERROR writing to socket");
    
	FILE * data;
	
	//char fin[6];
	//strcpy(fin, "FIN");
    memset(buffer,0,MAX_PACKET_LEN);
	int seq_num = 0; //UPDATE!!!!
	
    n = recvfrom(sockfd,buffer,MAX_PACKET_LEN-1, 0, (struct sockaddr *)&serv_addr, &serv_len); //read from the socket
	//printf("n: %d", n);
    if (n < 0) {error("ERROR reading from socket");}
        //printf("%s\n",buffer);	//print server's response
    /*
    	if(strcmp(buffer, fin)) {
    		n = sendto(sockfd,"FINACK", 7, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));		
    		break;	
    	}
    */
	printf("Receiving packet %d\n", seq_num);
	data = fopen("received.data", "a");
	//printf("buffer client: %s", buffer);
	//Need to parse out sequence number
    fprintf(data, buffer);
	
	fclose(data);
	
    
    close(sockfd); //close socket
    
    return 0;
}
