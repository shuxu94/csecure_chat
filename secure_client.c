/*
 * chatserv.c - A Secure UDP Chat Server
 *
 * Course Name: 14:332:456 - Network Centric Programming
 * Final Project
 * Student Names: Matthew Chatten, Shu Xu, Chris Geraldpaulraj
 * 
 * This program implements a UDP server able to securely accept and handle chat messages from multiple clients at once.
 */

#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	int fd;	

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) //create socket
	{ 
		perror("Cannot create socket."); 
		return 0;
	}

	struct sockaddr_in clientaddr; //built structure for client socket
	bzero((char *) &clientaddr, sizeof(clientaddr));
    	clientaddr.sin_family = AF_INET; 
    	clientaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	clientaddr.sin_port = htons(0); //random port
 
    	if(bind(fd, (struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0) //bind socket
	{
		perror("Bind failed.");
		return 0;
	}

	struct sockaddr_in serveraddr;
	bzero((char *) &serveraddr, sizeof(serveraddr));
    	serveraddr.sin_family = AF_INET; 
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    	serveraddr.sin_port = htons(8080);

	printf("Enter a message to start.\n");
	char test[512];
	fgets(test, sizeof(test), stdin);

	if(sendto(fd, test, strlen(test), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
	{ 
		perror("The sendto command failed."); 
		return 0; 
	}

	return 0;
}
