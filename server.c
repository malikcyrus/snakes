#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main( int argc, char *argv[] )
{
    // Declare variables
    int sockfd, newsockfd, portno, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int  n;

    // Create server socket (AF_INET, SOCK_STREAM)
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    // Initialize socket structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    portno = 5001;  // Server port
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any address
    serv_addr.sin_port = htons(portno);
 
    // Bind the host address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
         perror("ERROR on binding");
		 close(sockfd);
         exit(-1);
    }

    // Start listening for the clients
    if (listen(sockfd, 5) < 0) {
        perror("ERROR on listening\n");
        close(sockfd);
        exit(-1);
    }
    clilen = sizeof(cli_addr);

    // Accept actual connection from the client
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0) 
    {
        perror("ERROR on accept");
		close(sockfd);
        exit(-1);
    }

    // If connection is established then start communicating 
    memset(buffer, 0, 256);
    if ((n = read(newsockfd, buffer, 255 )) < 0)
    {
        perror("ERROR reading from socket");
		close(sockfd);
		close(newsockfd);
        exit(1);
    }
    printf("Here is the message: %s",buffer);

    // Write a response to the client
    if ((n = write(newsockfd, "I got your message", 18)) < 0)
    {
        perror("ERROR writing to socket");
		close(newsockfd);
		close(sockfd);
        exit(1);
    }

    // All done, close sockets
    close(newsockfd);
    close(sockfd);
    exit(0);
}
