#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include <pthread.h>

#define PORTNUM "1234"
#define BACKLOG 8

/* A bare nude simple HTTP server
 */

int process_request(int*);
int read_line(int, char *, int);


int main(void)
{
    // addrinfo
    int status = 0;
    int sockfd, csock;
    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((status = getaddrinfo("192.168.0.11", PORTNUM, &hints, &servinfo)) == -1)
    {
        printf("getaddrinfo error: %s\n", gai_strerror(status));
        status = 1;
        goto cleanup;
    }

    // get default socket file descriptor
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
        perror("socket error");
        status = 1;
        goto cleanup;
    } else printf("server socket %d created\n", sockfd);

    // bind socket, port
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        perror("bind error");
        status = 1;
        goto cleanup;
    } else printf("bind socket %d to port %s\n", sockfd, PORTNUM);

    // begin listen
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen error");
        status = 1;
        goto cleanup;
    } else printf("listening...\n");

    struct sockaddr_storage caddr;
    socklen_t addr_size = sizeof(caddr);

    // process HTTP service requests in loop
    while (1)
    {
        // accept incoming client request, generate client socket
        if ((csock = accept(sockfd, (struct sockaddr *) &caddr, &addr_size)) == -1)
        {
            perror("accept error");
            status = 1;
            goto cleanup;
        } else
        {
            printf("Ready to communicate on client socket %d\n", csock);
        }

        // create thread to handle client request
        /*pthread_t handle_request_thread;
        if (pthread_create(&handle_request_thread, NULL, process_request, &csock))
        {
            perror("pthread error\n");
        }*/
        char buf[1024];
        const int buflen = sizeof(buf);
        read_line(csock, buf, buflen);
    }

cleanup:
    close(csock);
    return status;
}

int read_line(int sock, char *buf, int len)
/* Read and print the client request line by line
 */
{
    //const char crlf[] = "\r\n";
    int i = 0;
    char c = '\0';
    int recvd = 0;

    for ( ; i < len-1 && c != '\n'; i++)
    {
        if ((recvd = recv(sock, &c, 1, 0)) == -1)
        {
            perror("recv error");
        } else if (!recvd)
        {
            c = '\n';
            return 0;
        } else {
            //if (c == '\r')
            buf[i] = c;
        }
    }
    buf[i+1] = '\0';
    printf("%s\n", buf);
}

/* Process the client's request
 */
int process_request(int* client)
{
    char buf[1024];
    const int buflen = sizeof(buf);
    //char url[256];
    //char path[512];
    
    int nextline = 0;
    while (nextline = read_line(*client, buf, buflen) > 0)
    {
        printf("%s\n", buf);
    }
    close(*client);
}
