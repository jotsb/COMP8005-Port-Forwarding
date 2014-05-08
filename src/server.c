#include "common.h"

/**
 * new_server
 *
 * Creates a server structure and initializes the variables
 *
 * @return s Returns the server structure
 */
server* new_server(void) {
    server *s = malloc(sizeof (server));
    if (s == NULL)
        fprintf(stderr, "Server Malloc() Failed\n");

    s->e_client_list = llist_new();
    s->portfd_list = llist_new();
    s->local_to_remote = llist_new();
    s->remote_to_local = llist_new();

    /* create the mutexes for controlling access to thread data */
    if (pthread_mutex_init(&s->dataLock, NULL) != 0) {
        free(s);
        return NULL;
    }

    s->epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
    if (s->epoll_fd < 0) {
        perror("epoll_create() Failed\n");
        return NULL;
    }

    return s;
}

/**
 * init_server
 *
 * Creates a socket and puts the socket into a listening mode
 *
 * @param s server structure variable.
 */
void init_server(server *s) {
    const int on = 1;
    struct sockaddr_in servaddr;
    char each_line[LINE_BUFFER_SIZE], dest_ip[LINE_BUFFER_SIZE];
    int src_port, dest_port;
    ipinfo *new_ip_info;
    fd_info *fdinfo;
    FILE *fp = fopen("config", "r");
    if (fp == NULL)
        SystemFatal("fopen(): Error opening the file\n");

    //each_line = malloc(sizeof (char *));

    fprintf(stderr, "> Creating TCP socket\n");
    while (fgets(each_line, LINE_BUFFER_SIZE, fp) != NULL) {

        //read the line and save the 1st as source_ port, 2nd as ip ,and 3rd as destination port
        sscanf(each_line, "%d %s %d", &src_port, dest_ip, &dest_port);

        new_ip_info = malloc(sizeof (ipinfo));
        fdinfo = malloc(sizeof (fd_info));

        /* create TCP socket to listen for client connections */
        if ((s->listen_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            SystemFatal("socket(): Socket Creation Failed\n");

        /* set the socket to allow re-bind to same port without wait issues */
        setsockopt(s->listen_sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (int));

        /* make the server Socket non-blocking */
        if (fcntl(s->listen_sd, F_SETFL, O_NONBLOCK | fcntl(s->listen_sd, F_GETFL, 0)) == -1)
            SystemFatal("fcntl(): Server Non-Block Failed\n");

        bzero(&servaddr, sizeof (servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(src_port);

        if (bind(s->listen_sd, (struct sockaddr *) &servaddr, sizeof (servaddr)) < 0)
            SystemFatal("bind(): Failed to bind socket\n");

        new_ip_info->dest_port = dest_port;
        strcpy(new_ip_info->ipAddress, dest_ip);
        //new_ip_info->ipAddress = dest_ip;
        new_ip_info->src_sd = s->listen_sd;
        s->portfd_list = llist_append(s->portfd_list, (void *) new_ip_info);

        //ip_info = llist_find(s->portfd_list, s->listen_sd, sd_compare);
        fdinfo->sd = s->listen_sd;
        fdinfo->first_time = true;
        /* setup the socket for listening to incoming connection  */
        if (listen(fdinfo->sd, LISTENQ) < 0)
            SystemFatal("Unable to listen on socket \n");

        /* Add the server socket to the epoll event loop  */
        s->event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
        s->event.data.ptr = fdinfo;

        if (epoll_ctl(s->epoll_fd, EPOLL_CTL_ADD, s->listen_sd, &s->event) == -1) {
            perror("epoll_ctl(): error - listen_sd\n");
            exit(EXIT_FAILURE);
        }
    }
    fclose(fp);
    //free(new_ip_info);
    ///free(fdinfo);
    fprintf(stderr, ">> Waiting for Connections\n\n");
}


