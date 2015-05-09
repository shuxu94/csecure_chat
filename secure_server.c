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
	int connected; //tracks if connected or not
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
			int i;
			int sendnum2;
			for(i = 0; i < clientnum; i++) //check to see if this is new client
			{
				if(rmaddr.sin_port == clientlist[i].cliad.sin_port)
				{
					newclient = 0; //not a new client
					sendnum2 = i; //save identity of client
				}

				//printf("%d\n%d\n", strlen(buf), strlen(clientlist[i].username));

				if(!(strncmp(buf, clientlist[i].username, strlen(buf))))
				{
					printf("Received invalid username.\n"); //if username already used
					char newuser[MESSSIZE]; //tell client to choose new one
					bzero(newuser, sizeof(newuser));
					char* test4 = "Username already in use. Please choose another.\n";
					strncpy(newuser, test4, strlen(test4));
					if(sendto(sockfd, newuser, strlen(newuser), 0, (struct sockaddr *) &rmaddr, addrlen) < 0) 
					{ 
						perror("The sendto command failed."); 
						return 0; 
					}
					goto nextmessage;
				}
			}			
	
			if(buf[1] == 'u')
			{
				printf("Received username: %s", buf);

				if(newclient == 1) //if new client, add to list of clients
				{
					strncpy(clientlist[clientnum].username, buf, USERSIZE);
					//NEED TO CHECK THAT USERNAME IS NOT ALREADY USED
					clientlist[clientnum].cliad = rmaddr;
					clientlist[clientnum].addrlen = addrlen;
					clientlist[clientnum].connected = 1;
	
					clientnum++; //increase number of clients
					if(clientnum > CLIENTS) //if too many clients
					{	
						char* fail = "Server shutting down.\n";
					
						/*struct sendinfo args1;
						args1.clinum = CLIENTS+1; //need to send to all clients
						strncpy(args1.message, fail, sizeof(fail));

						pthread_t tid1;
						int rc1;
						if((rc1 = pthread_create(&tid1, NULL, sendmess, &args1)) != 0)
						{
							perror("Failed to create thread.");
							return 0;
						}
						if((rc1 = pthread_detach(tid1)) != 0) //don't need to rejoin later
						{
							perror("Failed to detach thread.");
							return 0;
						}
						sleep(1); //give chance to send out shutdown message*/
						break;
					}

					char intro[MESSSIZE]; //send intro to new client
					bzero(intro, sizeof(intro));
					char* test = "Connected to server. Type and press enter to chat.\n Commands:\n Use '/u username' to change username.\n Use '/t username message' to send a direct message to another user.\n Use '/q' to quit.\n";
					strncpy(intro, test, strlen(test));
					if(sendto(sockfd, intro, strlen(intro), 0, (struct sockaddr *) &rmaddr, addrlen) < 0) 
					{ 
						perror("The sendto command failed."); 
						return 0; 
					}
				}
				else //otherwise, just change the client's username
				{
					strncpy(clientlist[sendnum2].username, buf, USERSIZE);
					
					char usechange[MESSSIZE]; //send intro to new client
					bzero(usechange, sizeof(usechange));
					char* test2 = "Username changed.\n";
					strncpy(usechange, test2, strlen(test2));
					if(sendto(sockfd, usechange, strlen(usechange), 0, (struct sockaddr *) &rmaddr, addrlen) < 0) 
					{ 
						perror("The sendto command failed."); 
						return 0; 
					}
				}
			}
			else if(buf[1] == 'q')
			{
				printf("Received quit request: %s", buf);

				clientlist[sendnum2].connected = 0; //set status of client to disconnected

				char exiter[MESSSIZE]; //send exit message to client
				bzero(exiter, sizeof(exiter));
				char* test3 = "Disconnected.\n";
				strncpy(exiter, test3, strlen(test3));
				if(sendto(sockfd, exiter, strlen(exiter), 0, (struct sockaddr *) &rmaddr, addrlen) < 0) 
				{ 
					perror("The sendto command failed."); 
					return 0; 
				}
				
			}
			else if(buf[1] == 't')
			{
				char* temp_ptr; //tracks location in buffer
				char* bufcpy = (char*) buf;
				
				bufcpy = strtok_r((char*) buf, " ", &temp_ptr); //get only the username
				printf("username grabbed: %s\n", buf);
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
		nextmessage: if(0); //does nothing, just end of while loop
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
		if(i != sender && clientlist[i].connected != 0) //don't send message back to client that sent it
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
