/* 
 * A simple lil' HTTP server
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * Test with http://192.168.0.12:8000/index.html
 */

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

#define DEFAULT_MAXHEADERLINE 1024
#define APACHE_MAXHEADERLINE 8192
#define CRLF "\r\n"
#define CRLFLEN 2

#define VERBOSE
#ifdef VERBOSE
#define PRINT(format, arg...) printf(format, ##arg)
#else
#define PRINT(format, arg...) (void)0
#endif

static const char *http200 = "HTTP/1.1 200 OK\n\n";
static const char *http404 = "HTTP/1.1 404 Not Found\n";

int read_line(int, char *, int);
#ifdef THREADED
void * process_request(void *);
#else
int process_request(int *);
#endif

int
main(void)
{
    int status = 0;
    int sockfd, csock;
    struct addrinfo hints, *servinfo, *si;
    struct sockaddr_storage caddr;

    /* address information */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((status = getaddrinfo(IP_ADDR, PORTNUM, &hints, &servinfo)) == -1) {
        PRINT("getaddrinfo error: %s\n", gai_strerror(status));
        status = 1;
        goto out;
    }

    for (si = servinfo; si != NULL; si = si->ai_next) {
        /* get default socket file descriptor */
        if ((sockfd = socket(si->ai_family, si->ai_socktype, 
                             si->ai_protocol)) == -1) {
            perror("socket error");
            continue;
        } else PRINT("server socket %d created\n", sockfd);

        /* bind socket, port */
        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &opt,
                   sizeof(int));
        if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind error");
            continue;
        }

        PRINT("bound socket %d to port %s\n", sockfd, PORTNUM);
        break;
    }
    if (si == NULL) {
        perror("could not find valid socket to bind");
        status = 2;
        goto out;
    }

    /* begin listen */
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen error");
        status = 1;
        goto cleanup;
    } else PRINT("listening...\n");

    socklen_t addr_size = sizeof(caddr);

    /* process HTTP service requests in loop */
    while (1) {
        /* accept incoming client request, generate client socket */
        if ((csock = accept(sockfd, (struct sockaddr *) &caddr, 
                            &addr_size)) == -1) {
            continue;
        } else {
            PRINT("Ready to communicate on client socket %d\n", csock);
        }

        /* create thread to handle client request */
#ifdef THREADED
        pthread_t handle_request_thread;
        if (pthread_create(&handle_request_thread, NULL, process_request, &csock))
        {
            perror("pthread error");
        }
#else
        process_request(&csock);
#endif
        if (close(csock) < 0) {
            perror("close client socket error");
            status = 1;
            goto cleanup;
        }
    }

cleanup:
    close(sockfd);
out:
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
 */

/*
 * Process the client's request
 */
int
process_request(int *client)
{
    char buf[DEFAULT_MAXHEADERLINE];
    char file_buf[APACHE_MAXHEADERLINE];
    const int buflen = sizeof(buf);
    const char *resource = "index.html";
    FILE *fp;
    //char path[512]; 
    size_t nbytes = 0, sent, offset;

    if (nbytes = read_line(*client, buf, buflen) > 0) {
        PRINT("%s", buf);
        /* check for resource request */
        //parse_request(buf);//TODO

        while ((nbytes = read_line(*client, buf, buflen)) > 0) {
            PRINT("%s", buf);
            /* check for end of HTTP request */
            if (nbytes == CRLFLEN) {
                if (strncmp(buf, CRLF, nbytes) == 0)
                    break;
            }
        }
    }

    if ((fp = fopen(resource, "r")) != NULL) {
        send(*client, http200, strlen(http200), 0);
        nbytes = fread(file_buf, sizeof(char), APACHE_MAXHEADERLINE, fp);
        file_buf[nbytes] = '\0';
        PRINT("%s\n", file_buf);
        offset = 0;
        while ((sent = send(*client, file_buf, nbytes, 0)) > 0 ||
               (sent == -1 && errno == EINTR)) {
            if (sent > 0) {
                offset += sent;
                nbytes -= sent;
            }
        }
        fclose(fp);
    } else {
        send(*client, http404, strlen(http404), 0);
    }

    return 0;
}
