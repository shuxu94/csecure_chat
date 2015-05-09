/*
 * secure_server.c - A Secure UDP Chat Server
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
#include <netdb.h>

#define PORT 8080
#define BUFSIZE 256
#define CLIENTS 3
#define MESSSIZE 256
#define USERSIZE 30
#define MSGBUF 15

int clientnum = 0;
int sockfd; /* our socket file descriptor */

void* sendmess(void* p);

struct clientdata {
	struct sockaddr_in cliad; //for address of client
	socklen_t addrlen; //for size of client address
	char username[USERSIZE]; //for client username
};

struct clientdata clientlist[CLIENTS];

struct sendinfo {
	int clinum;
	char message[MESSSIZE];
};

int main(int argc, char **argv)
{
	struct sockaddr_in myaddr; /* our address */
	struct sockaddr_in rmaddr; /* remote address */
	socklen_t addrlen = sizeof(rmaddr); /* length of addresses */
	int recvlen; /* number of bytes recieved */
	char buf[BUFSIZE]; /* receive buffer */
	int newclient = 1; //tracks if client is new or not

	/* creating a UDP socket */
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("Cannot create socket."); 
		return 0;
	}

	/* bind the socket to any valid IP address and our port */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(PORT);

	if(bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
	{
		perror("Bind failed.");
		return 0;
	}

	/* now loop, receiving data and printing what we received */
	while(1)
	{
		newclient = 1;		
		bzero(buf, sizeof(buf));
		//printf("----Waiting on port %d.----\n", PORT);
		recvlen = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&rmaddr, &addrlen);
		printf("Received %d bytes\n", recvlen);

		if(buf[0] == '/') 
		{
			if(buf[1] == 'u')
			{
				printf("Received username: %s", buf);

				int i;
				for(i = 0; i < clientnum; i++) //check to see if this is new client
				{
					if(rmaddr.sin_port == clientlist[i].cliad.sin_port)
						newclient = 0;
					//printf("%d %d\n", rmaddr.sin_port, clientlist[i].cliad.sin_port);
				}

				if(newclient == 1) //if new client, add to list of clients
				{
					strncpy(clientlist[clientnum].username, buf, USERSIZE);
					//NEED TO CHECK THAT USERNAME IS NOT ALREADY USED
					clientlist[clientnum].cliad = rmaddr;
					clientlist[clientnum].addrlen = addrlen;
	
					clientnum++; //increase number of clients
				}

				char intro[MESSSIZE]; //send intro to new client
				char* test = "Connected to server. Use /q to quit, type and press enter to chat.\n";
				strncpy(intro, test, strlen(test));
				if(sendto(sockfd, intro, strlen(intro), 0, (struct sockaddr *) &rmaddr, addrlen) < 0) 
				{ 
					perror("The sendto command failed."); 
					return 0; 
				}
			}
		}
		else
		{
			//buf[recvlen] = 0; //make last character in buff 0
			printf("Received message: %s", buf);
			int sendnum;
			char msg[MESSSIZE];
			bzero(msg, sizeof(msg));

			//printf("clientnum: %d\n", clientnum);

			int i;
			for(i = 0; i < clientnum; i++) //check to see what client sent message
			{
				if(rmaddr.sin_port == clientlist[i].cliad.sin_port)
					sendnum = i;
				//printf("%d %d\n", rmaddr.sin_port, clientlist[i].cliad.sin_port);
			}

			strncpy(msg, buf, strlen(buf)); //save message

			struct sendinfo args;
			args.clinum = sendnum;
			strncpy(args.message, msg, sizeof(msg));

			pthread_t tid;
			int rc;
			if((rc = pthread_create(&tid, NULL, sendmess, &args)) != 0)
			{
				perror("Failed to create thread.");
				return 0;
			}
			if((rc = pthread_detach(tid)) != 0) //don't need to rejoin later
			{
				perror("Failed to detach thread.");
				return 0;
			}
		}
	}
}

void* sendmess(void* parameters) 
{
	struct sendinfo* p = (struct sendinfo*) parameters; //get pointer to struct
	int sender = p->clinum; //get identity of sending client
	char mess[MESSSIZE];
	strncpy(mess, p->message, sizeof(p->message)); //get message
	char username[USERSIZE+MESSSIZE];
	strncpy(username, clientlist[sender].username, strlen(clientlist[sender].username));
	username[strlen(username)-1] = 0; //get rid of newline
	//printf("username: %s\n", username);
	char* addon = " says:\n";
	strncat(username, addon, strlen(addon));
	strncat(username, mess, MESSSIZE);

	int i;
	for(i = 0; i < clientnum; i++)
	{
		if(i != sender) //don't send message back to client that sent it
		{
			struct sockaddr_in cliad = clientlist[i].cliad;
			socklen_t addrlen = clientlist[i].addrlen;
			//char username[USERSIZE] = clientlist[i].username; //ADD TO MESSAGE

			printf("sending to client: %d\n", i);
		
			if(sendto(sockfd, username, strlen(username), 0, (struct sockaddr *) &cliad, addrlen) < 0) 
			{ 
				perror("The sendto command failed."); 
				return 0; 
			}
		}
	}

	bzero(mess, sizeof(mess));
	bzero(username, sizeof(username));

	return NULL;
}
