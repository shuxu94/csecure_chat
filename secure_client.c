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


void* readmess(void* p);

struct threaddata {
	int cfd; //for file descriptor
	struct sockaddr_in servad; //for address of server
}; 

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

	struct threaddata args;
	args.cfd = fd; //save file descriptor
	args.servad = serveraddr; //save server address

	pthread_t tid;
	int rc;
	if((rc = pthread_create(&tid, NULL, readmess, &args)) != 0)
	{
		perror("Failed to create thread.");
		return 0;
	}
	if((rc = pthread_detach(tid)) != 0) //don't need to rejoin later
	{
		perror("Failed to detach thread.");
		return 0;
	}

	while(1)
	{
		printf("Enter a message.\n");
		char test[512];
		if(fgets(test, sizeof(test), stdin) == NULL)
		{
			perror("Invalid message.");
			return 0;
		}

		if(sendto(fd, test, strlen(test), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
		{ 
			perror("The sendto command failed."); 
			return 0; 
		}
	}

	return 0;
}

/*
 * readmess - Handles all reads from server.
 * 
 * The inputs are the fd of the client socket and the serveraddr struct.
 */
void* readmess(void* parameters) 
{
	struct threaddata* p = (struct threaddata*) parameters; //get pointer to struct
	int sfd = p->cfd; //save values from struct to local variables
	struct sockaddr_in saddr = p->servad;

	printf("Entering thread. sfd: %d\n", sfd);

	/*int rc;
	if((rc = close(sfd)) < 0)
	{
		perror("Failed to close socket.");
		return 0;
	}*/

	return NULL;
}
