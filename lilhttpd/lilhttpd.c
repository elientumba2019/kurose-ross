#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include <pthread.h>

#define IP_ADDR "192.168.0.12"
#define PORTNUM "8000"
#define BACKLOG 8
#define FBUFLEN 99999

static const char *http_ack = "HTTP/1.1 200 OK\n\n";
static const char *http_nack = "HTTP/1.1 404 Not Found\n";

/* 
 * A simple lil' HTTP server
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 */

#ifdef THREADED
void * process_request(void *);
#else
int process_request(int *);
#endif
int read_line(int, char *, int);

int
main(void)
{
    int status = 0;
    int sockfd, csock;
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage caddr;

    /* address information */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((status = getaddrinfo(IP_ADDR, PORTNUM, &hints, &servinfo)) == -1) {
        printf("getaddrinfo error: %s\n", gai_strerror(status));
        status = 1;
        goto out;
    }

    /* get default socket file descriptor */
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, 
                         servinfo->ai_protocol)) == -1) {
        perror("socket error");
        status = 1;
        goto out;
    } else printf("server socket %d created\n", sockfd);

    /* bind socket, port */
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(int));
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("bind error");
        status = 1;
        goto out;
    } else printf("bind socket %d to port %s\n", sockfd, PORTNUM);

    /* begin listen */
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen error");
        status = 1;
        goto out;
    } else printf("listening...\n");

    socklen_t addr_size = sizeof(caddr);

    /* process HTTP service requests in loop */
    while (1) {
        /* accept incoming client request, generate client socket */
        if ((csock = accept(sockfd, (struct sockaddr *) &caddr, 
                            &addr_size)) == -1) {
            //perror("accept error");
            //status = 1;
            //goto out;
            continue;
        } else {
            printf("Ready to communicate on client socket %d\n", csock);
        }

        /* create thread to handle client request */
#ifdef THREADED
        pthread_t handle_request_thread;
        if (pthread_create(&handle_request_thread, NULL, process_request, &csock))
        {
            perror("pthread error\n");
        }
#else
        process_request(&csock);
#endif
        if (close(csock) < 0) {
            perror("close client socket error\n");
            status = 1;
            goto out;
        }
    }

out:
    close(sockfd);
    return status;
}

/*
 * Read and print the client request line by line
 * Returns 0 if recv() fails; otherwise returns line length
 */
int
read_line(int sock, char *buf, int len)
{
    int i;
    char c = '\0';
    int recvd = 0;

    for (i = 0; i < len-1 && c != '\n'; i++)
    {
        if ((recvd = recv(sock, &c, 1, 0)) == -1) {
            perror("recv error");
            return 0;
        } else if (!recvd) {
            return 0;
        } else {
            buf[i] = c;
        }
    }
    buf[i] = '\0';
    return i;
}

/*
 * Process the client's request
 */
int
process_request(int *client)
{
    char buf[1024];
    const int buflen = sizeof(buf);
    const char *resource = "index.html";
    FILE *fp;
    //char url[256];
    //char path[512]; 
    char file_buf[FBUFLEN+1];
    size_t nbytes = 0;

    int nextline = 0, flag = 0;
    while (nextline = read_line(*client, buf, buflen) > 0) {
        printf("%s", buf);
        if (buf[0] == '\r') break;//XXX
    }

    char c;
    if ((fp = fopen(resource, "r")) != NULL) {
        send(*client, http_ack, strlen(http_ack), 0);
        nbytes = fread(file_buf, sizeof(char), FBUFLEN, fp);
        file_buf[nbytes+1] = '\0';
        printf("%s\n", file_buf);
        send(*client, file_buf, nbytes, 0);
        /*while ((bytes_read=read(fd, data_to_send, BYTES))>0 )
        write (clients[n], data_to_send, bytes_read);*/
        fclose(fp);
    } else {
        send(*client, http_nack, strlen(http_nack), 0);
    }

    return 0;
}

//http://www.cs.bu.edu/fac/matta/Teaching/CS552/F99/proj4/
//https://github.com/AaronKalair/C-Web-Server
//http://blog.eviac.com/2012/11/web-server-in-c.html
//http://css.dzone.com/articles/web-server-c
//http://www.paulgriffiths.net/program/c/srcs/webservsrc.html
//http://blog.abhijeetr.com/2010/04/very-simple-http-server-writen-in-c.html
//http://stackoverflow.com/questions/5594042/c-send-file-to-socket
