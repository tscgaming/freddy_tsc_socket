/******************************************************************************
* echo_client.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo client.  The  *
*              client connects to an arbitrary <host,port> and sends input    *
*              from stdin.                                                    *
*                                                                             *
* Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*                                                                             *
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#define ECHO_PORT 9999
#define BUF_SIZE 4096

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <server-ip> <port>",argv[0]);
        return EXIT_FAILURE;
    }

    char buf[BUF_SIZE];
        
    int status, sock;
    struct addrinfo hints; //filled up relevantly
	memset(&hints, 0, sizeof(struct addrinfo)); // make sure the struct is empty
    struct addrinfo *servinfo; //will point to the results
    hints.ai_family = AF_INET;  //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP  streamsockets
    hints.ai_flags = AI_PASSIVE; //fill in my IP for me
    FILE *fp;
    fp = fopen(argv[3], "rt");
    if (fp == NULL) {
        perror("Unable to open file");
        return 1;
    }
    if ((status = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo error: %s \n", gai_strerror(status));
        return EXIT_FAILURE;
    }
// servinfo now points to a linked list of 1 or more struct addrinfos
    if((sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
//socket() simply returns to you a socket descriptor that you can use in later system calls, or -1 on error.
        fprintf(stderr, "Socket failed");
        return EXIT_FAILURE;
    }
    
    if (connect (sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        fprintf(stderr, "Connect");
        return EXIT_FAILURE;
    }
        
    char msg[BUF_SIZE];

    int bytes_received;
    // long fileSize = ftell(fp);
    // size_t bytesRead = fread(msg, 1, fileSize, fp);
    // if (bytesRead != fileSize) {
    //     perror("Error reading file");
    //     free(msg);
    //     freeaddrinfo(servinfo);
    //     close(sock);
    //     fclose(fp);
    //     return EXIT_FAILURE;
    // }
    size_t bytes_read;
    int bytes_sent;
    while ((bytes_read = fread(msg, 1, BUF_SIZE, fp)) > 0) {
        bytes_sent = send(sock, msg, bytes_read, 0);
        if (bytes_sent == -1) {
            perror("send");
            fclose(fp);
            close(sock);
            freeaddrinfo(servinfo);
            return -1;
        }
    }

    fprintf(stdout, "Sending\n %s", msg);
    if((bytes_received = recv(sock, buf, BUF_SIZE, 0)) > 1)
    {
        buf[bytes_received] = '\0';
        fprintf(stdout, "Received\n %s", buf);
    }        

    freeaddrinfo(servinfo);
    close(sock);    
    return EXIT_SUCCESS;
}
