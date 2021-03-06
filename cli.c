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

#include "encryption.c"
#include "serialize.h"
#include "serialize.c"

/*#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <arpa/inet.h> // For htonl() */

#define MESSSIZE 1000
#define USERSIZE 30

FILE *rsa_pkey_file; //file descriptor for public key

void* readmess(void* p);

struct threaddata {
	int cfd; //for file descriptor
	struct sockaddr_in servad; //for address of server
	char pubkey[MESSSIZE]; //for public key of server
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
	socklen_t addrlen = sizeof(serveraddr);

	struct threaddata args;
	args.cfd = fd; //save file descriptor
	args.servad = serveraddr; //save server address

	printf("Choose a username (no spaces) using syntax: '/u username'.\n");
	char username[USERSIZE];
	if(fgets(username, sizeof(username), stdin) == NULL)
	{
		perror("Invalid message.");
		return 0;
	}

	if((username[0] != '/') || (username[1] != 'u') || (username[2] != ' ')) //check message
	{
		printf("Invalid username.\n");
		return 0;
	}

	if(sendto(fd, username, strlen(username), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) //send username to server
	{ 
		perror("The sendto command failed."); 
		return 0; 
	}

	char packet2[MESSSIZE]; //receive confirmation of username and instructions
	bzero(packet2, sizeof(packet2));
	if(recvfrom(fd, packet2, sizeof(packet2), 0, (struct sockaddr*) &serveraddr, &addrlen) < 0)
	{ 
		perror("The recvfrom command failed."); 
		return 0; 
	}
	printf("%s", packet2); //print intro message from server

	//BEGIN ENCRYPTION HANDSHAKE

	//FILE *rsa_pkey_file;
	rsa_pkey_file = fopen(argv[1], "rb"); //get public key for client
    if (!rsa_pkey_file) //if error opening
    {
        perror(argv[1]);
        fprintf(stderr, "Error loading PEM RSA Public Key File.\n");
        exit(2);
    }

	//printf("%p\n", rsa_pkey_file);
	
	char pkey[MESSSIZE];
	bzero(pkey, sizeof(pkey));
	size_t keysize;
	keysize = fread(pkey, 1, MESSSIZE, rsa_pkey_file); //read public key
	//fclose(rsa_pkey_file);
	//printf("%s\n%d", pkey, (int) keysize);
	//printf("%s\n", pkey);

	if(sendto(fd, pkey, strlen(pkey), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) //send public key to server
	{ 
		perror("The sendto command failed."); 
		return 0; 
	}

	char packet3[MESSSIZE]; //receive server's public key
	bzero(packet3, sizeof(packet3));
	if(recvfrom(fd, packet3, sizeof(packet3), 0, (struct sockaddr*) &serveraddr, &addrlen) < 0)
	{ 
		perror("The recvfrom command failed."); 
		return 0; 
	}
	//printf("%s PROOFPROOF\n", packet3);
	//printf("%s\n", packet3);

	strncpy(args.pubkey, packet3, strlen(packet3)); //save server key

	//END ENCRYPTION HANDSHAKE

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

		int rv;
		char ciphertext[1000]; //not used
		struct encryption_key key; //will hold encrypted message
		//char *in = "12345";

		rv = do_evp_seal(rsa_pkey_file, &test[0], ciphertext, &key); //encrypt message

		ser_tra_t* tra;
  
		tra = ser_new_tra("enc_key", sizeof(encryption_key), NULL);
		ser_new_field(tra, "string", 0, "1",   offsetof(encryption_key, ek));
		ser_new_field(tra, "int", 0, "2", offsetof(encryption_key, eklen));
		ser_new_field(tra, "string", 0, "3",   offsetof(encryption_key, iv));
		ser_new_field(tra, "string", 0, "4",   offsetof(encryption_key, ciphertext));
		ser_new_field(tra, "int", 0, "5", offsetof(encryption_key, cipertext_len));

		printf("proof\n");
	
		char* result;
		result = ser_ialize(tra, "enc_key", &key, NULL, 0);
		printf("Result:\n%s\n", result);		

		/*
		//OLD serialization
		unsigned char *serialized_key;
		//printf("encrypted key size: %d\n", sizeof(key));
		serialized_key = (unsigned char *)malloc(sizeof(key));
		memcpy(serialized_key, &key, sizeof(key));
		//printf("serialized key size: %d\n", sizeof(serialized_key));

		char skey[sizeof(key)];
		strncpy(skey, serialized_key, strlen(serialized_key));
		printf("serialized key size: %d\n", sizeof(skey));
		//OLD SERIAL
		*/

		if(sendto(fd, result, sizeof(result), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
		{ 
			perror("The sendto command failed."); 
			return 0; 
		}

		if(test[0] == '/' && test[1] == 'q')
		{	
			sleep(1); //wait for any message from server
			break; //end endless loop and close program
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
	int drv; //track size of encrypted messages

	char packet1[MESSSIZE];

	//printf("Entering thread. sfd: %d\n", sfd);

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
