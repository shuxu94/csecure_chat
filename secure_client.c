/*
 * secure_client.c - A Secure UDP Chat Client
 *
 * Course Name: 14:332:456 - Network Centric Programming
 * Final Project
 * Student Names: Matthew Chatten, Shu Xu, Chris Geraldpaulraj
 * 
 * This program implements a UDP client able to securely accept and handle chat messages from multiple clients at once through a central server.
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

#define MESSSIZE 256
#define USERSIZE 30

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

	printf("Choose a username, using syntax: '/u username'.\n");
	char username[USERSIZE];
	if(fgets(username, sizeof(username), stdin) == NULL)
	{
		perror("Invalid message.");
		return 0;
	}

	if(sendto(fd, username, strlen(username), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) //send username to server
	{ 
		perror("The sendto command failed."); 
		return 0; 
	}

	char packet2[MESSSIZE]; //receive confirmation of username and instructions
	bzero(packet2, sizeof(packet2));
	socklen_t addrlen = sizeof(serveraddr);
	if(recvfrom(fd, packet2, sizeof(packet2), 0, (struct sockaddr*) &serveraddr, &addrlen) < 0)
	{ 
		perror("The recvfrom command failed."); 
		return 0; 
	}
	printf("%s", packet2);

	pthread_t tid; //create new thread to handle all messages from server from now on
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

	while(1) //loop to send new messages to server
	{
		//printf("Enter a message.\n");
		char test[MESSSIZE];
		bzero(test, sizeof(test));
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
	socklen_t addrlen = sizeof(saddr);

	char packet1[140];

	printf("Entering thread. sfd: %d\n", sfd);

	while(1) //loop to receive messages from server
	{
		bzero(packet1, sizeof(packet1));		
		if(recvfrom(sfd, packet1, sizeof(packet1), 0, (struct sockaddr*) &saddr, &addrlen) < 0)
		{ 
			perror("The recvfrom command failed."); 
			return 0; 
		}
		
		printf("%s", packet1);
	}		

	/*int rc;
	if((rc = close(sfd)) < 0)
	{
		perror("Failed to close socket.");
		return 0;
	}*/

	return NULL;
}


/*char username[USERSIZE];
char *u = username;
int bytesread;
size_t usesize = USERSIZE;
bytesread = getline(&u, &usesize, stdin);
if(sendto(fd, username, bytesread, 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
{ 
	perror("The sendto command failed."); 
	return 0; 
}*/
