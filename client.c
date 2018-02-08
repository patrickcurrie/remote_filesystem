#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include "client.h"

/*
 * Creates a client message
 */
void createClientMessage(char *msg, char op, char fm, int info, int len, char *str);

/*
 * Print server messages
 */
void printServerMessage(char *msg);

/*
 * Global variables are evil, so this mess is only temporary.
 */
static int file_mode = 0; // Gloabal varaible that designates filemode for all lib functions.
static int net_server_init_success = 0; // 1 if net_server_connection_init was run/successful.
static char *host_name;
struct addrinfo hints, *infoptr;

int net_server_connection_init(char *hostname, int fm) {
        hints.ai_family = AF_INET;
        int res = getaddrinfo(hostname, NULL, &hints, &infoptr);
        /*
         * getaddrinfo() returns 8 if the host cannot be found.
         */
        if (res == 8) {
                h_errno = HOST_NOT_FOUND;
                net_server_init_success = 0;
                return -1;
        }
        file_mode = fm;
        host_name = hostname;
        /*
         * This loops through a list of all IP's associated with the host.
         *
        struct addrinfo *p;
        char host[256], service[256];


        for(p = infoptr; p != NULL; p = p->ai_next) {
                getnameinfo(p->ai_addr, p->ai_addrlen, host, sizeof(host), service, sizeof(service), NI_NUMERICHOST);
                puts(host);
        }
        */
        freeaddrinfo(infoptr);
        net_server_init_success = 1;
        return 0;
}

int net_open(const char *path_name, int flag) {
        if (net_server_init_success == 0) {
                h_errno = HOST_NOT_FOUND;
                return -1;
        }
        /*
         * Address Family - AF_INET (this is IP version 4)
         * Type - SOCK_STREAM (this means connection oriented TCP protocol)
         * Protocol - 0 [ or IPPROTO_IP This is IP protocol]
         */
        int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) {
                perror("Socket creation failed");
                return 0;
        }
        struct sockaddr_in serv_addr_info;
        bzero((char *) &serv_addr_info, sizeof(serv_addr_info));
        serv_addr_info.sin_port = htons(PORTNO);
        serv_addr_info.sin_family = AF_INET;
        struct hostent *serv_addr = gethostbyname(host_name);
        if (serv_addr == NULL) {
                fprintf(stderr, "ERROR, no such host\n");
                return 0;
        }
        bcopy((char *)serv_addr->h_addr, (char *) &serv_addr_info.sin_addr.s_addr, serv_addr->h_length);
        int res = connect(sock_fd, (struct sockaddr *) &serv_addr_info, sizeof(serv_addr_info));
        if (res < 0) {
                perror("ERROR connecting");
                return 0;
        }
        char buf[BLOCK_SIZE];
        bzero(buf, BLOCK_SIZE);
        int fp = flag;
        int len = strlen(path_name);
        char * str = malloc(len + 1);
        strncpy(str, path_name, sizeof(path_name));
        // Message stored in buf after this function call.
        createClientMessage(buf, 0, file_mode, fp, len, str);
        res = write(sock_fd, buf, METADATA_SIZE + len);
        if (res < 0) {
                perror("Writing to socket failed\n");
                return 0;
        }
        bzero(buf, BLOCK_SIZE);
        res = read(sock_fd, buf, BLOCK_SIZE);
        if (res < 0) {
                perror("Error reading from socket");
                return 0;
        }
        //printServerMessage(buf);
        // Get error from buf.
        int error;
        memcpy(&error, buf + 2, sizeof(int));
        if (error) {
                errno = error;
                return -1;
        }
        // Gets fd of remote file from buf.
        int fd;
        memcpy(&fd, buf + 2 + 2 * sizeof(int), sizeof(int));
        return fd;
}

ssize_t net_read(int fd, void *read_buf, size_t nbyte) {
        if (net_server_init_success == 0) {
                h_errno = HOST_NOT_FOUND;
                return -1;
        }
        int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) {
                return -1;
        }
        struct sockaddr_in serv_addr_info;
        bzero((char *) &serv_addr_info, sizeof(serv_addr_info));
        serv_addr_info.sin_port = htons(PORTNO);
        serv_addr_info.sin_family = AF_INET;
        struct hostent *serv_addr = gethostbyname(host_name);
        if (serv_addr == NULL) {
                fprintf(stderr, "ERROR, no such host\n");
                return -1;
        }
	bcopy((char *) serv_addr->h_addr, (char *) &serv_addr_info.sin_addr.s_addr, serv_addr->h_length);
	int res = connect(sock_fd, (struct sockaddr *) &serv_addr_info, sizeof(serv_addr_info));
	if (res < 0) {
                return -1;
	}
        char buf[BLOCK_SIZE];
        int len = (int) nbyte;
        createClientMessage(buf, 1, file_mode, fd, sizeof(int), (char *) &len);
        res = write(sock_fd, buf, METADATA_SIZE + BLOCK_SIZE);
        if (res < 0) {
                return -1;
        }
        bzero(buf, BLOCK_SIZE);
        res = read(sock_fd, buf, BLOCK_SIZE);
        if (res < 0) {
                return -1;
        }
        //printServerMessage(buf);
        int error;
        memcpy(&error, buf + 2, sizeof(int));
        if (error) {
                errno = error;
                return -1;
        }
        int size = 0;
        memcpy(&size, buf + 2 + sizeof(int), sizeof(int));
        if (size <= 0) {
                return -1;
        }
        int read_len = size;
        if (read_len > 0) {
                memcpy(read_buf, buf + 2 + 2 * sizeof(int), read_len);
        }
        return read_len;
}

ssize_t net_write(int fd, const void *write_buf, size_t nbyte) {
        if (net_server_init_success == 0) {
                h_errno = HOST_NOT_FOUND;
                return -1;
        }
        int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

        if (sock_fd < 0) {
                return -1;
        }
        struct sockaddr_in serv_addr_info;
        bzero((char *) &serv_addr_info, sizeof(serv_addr_info));
        serv_addr_info.sin_port = htons(PORTNO);
        serv_addr_info.sin_family = AF_INET;
        struct hostent *serv_addr = gethostbyname(host_name);
        if (serv_addr == NULL) {
                fprintf(stderr, "ERROR, no such host\n");
                return -1;
        }
        bcopy((char *) serv_addr->h_addr, (char *) &serv_addr_info.sin_addr.s_addr, serv_addr->h_length);
        int res = connect(sock_fd, (struct sockaddr *) &serv_addr_info, sizeof(serv_addr_info));
        if (res < 0) {
                return -1;
        }
        char buf[BLOCK_SIZE];
        int len = strlen(write_buf);
        createClientMessage(buf, 2, file_mode, fd, len, (char *) write_buf);
        res = write(sock_fd, buf, METADATA_SIZE + BLOCK_SIZE);
        if (res < 0) {
                return -1;
        }
        bzero(buf, BLOCK_SIZE);
        res = read(sock_fd, buf, BLOCK_SIZE);
        if (res < 0) {
                return -1;
        }
        //printServerMessage(buf);
        //Get error from buf
        int error;
        memcpy(&error, buf + 2, sizeof(int));
        if (error) {
                errno = error;
                return -1;
        }
        int write_len;
        memcpy(&write_len, buf + 2 + 2 * sizeof(int), sizeof(int));
        return write_len;
}

int net_close(int fd) {
        if (net_server_init_success == 0) {
                h_errno = HOST_NOT_FOUND;
                return -1;
        }
        int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) {
                return -1;
        }
        struct sockaddr_in serv_addr_info;
        bzero((char *) &serv_addr_info, sizeof(serv_addr_info));
        serv_addr_info.sin_port = htons(PORTNO);
        serv_addr_info.sin_family = AF_INET;
        struct hostent *serv_addr = gethostbyname(host_name);
        if (serv_addr == NULL) {
                fprintf(stderr, "ERROR, no such host\n");
                return -1;
        }
        bcopy((char *)serv_addr->h_addr, (char *) &serv_addr_info.sin_addr.s_addr, serv_addr->h_length);
        int res = connect(sock_fd, (struct sockaddr *) &serv_addr_info, sizeof(serv_addr_info));
        if (res < 0) {
                return -1;
        }
        char buf[BLOCK_SIZE];
        createClientMessage(buf, 3, file_mode, fd, 0, NULL);
        res = write(sock_fd, buf, METADATA_SIZE + BLOCK_SIZE);
        if (res < 0) {
                return -1;
        }
        bzero(buf, BLOCK_SIZE);
        res = read(sock_fd, buf, BLOCK_SIZE);
        if (res < 0) {
                return -1;
        }
        //printServerMessage(buf);
        //Get error from buf
        int error;
        memcpy(&error, buf + 2, sizeof(int));
        if (error) {
                errno = error;
                return -1;
        }
        return 0;
}

void createClientMessage(char *msg, char op, char fm, int info,
	int len, char *str) {
        msg[0] = op;
        msg[1] = fm;
        memcpy(msg + 2, &info, sizeof(int));
        memcpy(msg + 2 + sizeof(int), &len, sizeof(int));
        memcpy(msg + 2 + 2 * sizeof(int), str, len);
}

void printServerMessage(char * msg) {
        char op = msg[0];
        char fm = msg[1];
        int len;
        int error;
        memcpy(&error, msg + 2, sizeof(int));
        memcpy(&len, msg + 2 + sizeof(int), sizeof(int));
        char str[BLOCK_SIZE];
        strncpy(str, msg + 2 + sizeof(int) * 2, len);
        printf("Operation: %d. Filemode: %d. Error code: %d\n",
        op, fm, error);
        errno = error;
        printf("Length: %d. String: ", len);
        int i;
        char tmp;
        for(i = 0; i < len; i++) {
                tmp = msg[2 + sizeof(int) * 2 + i];
                if (tmp > 31) {
                        printf("%c ", tmp);
                }
                else {
                        printf("%hhu ", tmp);
                }
        }
        printf("\n");
}
