#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>


#define PORT 8080
#define BUFSIZE 2048

int main(int argc, char **argv)
{
    struct sockaddr_in myaddr; /* our address */
    struct sockaddr_in rmaddr; /* remote address */
    socklen_t addrlen = sizeof(rmaddr); /* length of addresses */
    int recvlen; /* number of bytes recieved */
    int sockfd; /* our socket file descriptor */
    unsigned char buf[BUFSIZE]; /* receive buffer */
    
    
    /* creating a UDP socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket \n");
        return 0;
    }
    
    /* bind the socket to any valid IP address and our port */
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(PORT);
    
    if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }
    
    /* now loop, receiving data and printing what we received */
    while(1) {
        printf("----waiting on port %d----\n", PORT);
        recvlen = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&rmaddr, &addrlen);
        printf("received %d bytes\n", recvlen);
        if (recvlen > 0) {
            buf[recvlen] = 0;
            printf("received message: \"%s\"\n", buf);
        }
    }
    
    
}