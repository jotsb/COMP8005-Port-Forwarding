/* 
 * File:   common.h
 * Author: root
 *
 * Created on February 15, 2014, 2:05 PM
 */

#ifndef COMMON_H
#define	COMMON_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdbool.h>


#define SERVER_TCP_PORT 7000	// Default port
#define BUFLEN	1024		//Buffer length
#define LISTENQ	5
#define EPOLL_QUEUE_LEN 256
#define LINE_BUFFER_SIZE 128
#define VAL(str) #str
#define TOSTRING(str) VAL(str)

    // llist.c 
    typedef struct _llist llist;
    typedef struct _node node;
    typedef struct _data data;
    typedef struct _fd_info fd_info;
    typedef struct _ipinfo ipinfo;
    typedef struct _port_fwd port_fwd;
    typedef struct _server server;
    typedef struct _client client;

    struct _llist {
        node *link;
    };

    struct _node {
        node *next;
        void *data;
    };

    struct _fd_info {
        bool first_time;
        int sd;
    };

    struct _ipinfo {
        int src_sd;
        int dest_port;
        char ipAddress[100];
        char *ip_address;
    };

    struct _port_fwd {
        int src_sd;
        int dst_sd;
    };

    struct _server {
        int listen_sd;
        int maxfd;
        pthread_mutex_t dataLock;
        llist *portfd_list;
        llist *local_to_remote;
        llist *remote_to_local;
        fd_info *fdinfo;
        /* for epoll on client connections */
        int epoll_fd;
        int num_fds;
        struct epoll_event events[EPOLL_QUEUE_LEN];
        struct epoll_event event;
        llist *e_client_list;
    };

    struct _client {
        struct sockaddr_in sa;
        socklen_t sa_len;
    };

    // FUNCTION PROTOTYPES llist.c
    bool ipinfo_compare(const void * _a, const void * _b);
    bool sd_compare(const void * _a, const int _b);
    bool portfwd_compare(const void * _a, const void * _b);
    llist* llist_new(void);
    void llist_free(llist *l, void (*free_func)(void*));
    llist* llist_append(llist *l, void *data);
    llist* llist_remove(llist *l, void *data, bool (*compare_func)(const void*, const void*));
    void *llist_find(llist *l, int fd, bool (*compare_func)(const void*, int fd));
    int llist_length(llist *l);
    bool llist_is_empty(llist *l);


    // COMMON
    void SystemFatal(const char*);
    void signal_Handler(int);
    void* client_manager(void *);
    void read_from_socket(server *s);


    // SERVER
    server* new_server(void);
    void init_server(server *);

    // CLIENT
    client *client_new(void);
    bool process_client_req(server *s, int sd);

#ifdef	__cplusplus
}
#endif

#endif	/* COMMON_H */

