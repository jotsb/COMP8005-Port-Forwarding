/*---------------------------------------------------------------------------------------
--	SOURCE FILE:                    main.c -   A simple port forwarding client, uses epoll API to manage multiple 
--                                      connections
--
--	PROGRAM:			Port Forwarding
--					gcc -Wall -ggdb -lpthread -o port_fd main.c server.c client.c llist.c
--
--	FUNCTIONS:			Berkeley Socket API
--
--	DATE:				February 19, 2014
--
--	REVISIONS:			(Date and Description)
--
--	DESIGNERS:			Design based on various code snippets found on C10K links
--					Modified and improved: Aman Abdulla - February 2008
--                                      Adapted to suit the requirements of the port forwarding assignment: 
--                                      Jivanjot Brar & Shan Bains - February 2014.
--
--	PROGRAMMERS:                    Jivanjot S Brar & Shan Bains
--
--	NOTES:
--	The program will accept TCP connections from client machines.
-- 	The program will read data from the client socket and simply echo it back.
--	Design is a simple, single-threaded server using non-blocking, edge-triggered
--	I/O to handle simultaneous inbound connections.
--	Test with accompanying client application: clnt.c
--      https://code.google.com/p/ganymed-ssh-2/source/browse/trunk/?r=2#trunk%2Fexamples
---------------------------------------------------------------------------------------*/
#include "common.h"

// Globals
pthread_t master_thread;
bool running = true;

int main(int argc, char **argv) {

    int ret;
    struct sigaction act;
    server *s;

    act.sa_handler = signal_Handler;
    act.sa_flags = 0;

    if ((sigaction(SIGINT, &act, NULL) == -1)) {
        SystemFatal("Failed to set SIGINT handler\n");
    }
    if ((sigaction(SIGPIPE, &act, NULL) == -1)) {
        SystemFatal("Failed to set SIGPIPE handler\n");
    }
    if ((sigaction(SIGSEGV, &act, NULL) == -1)) {
        SystemFatal("Failed to set SIGPIPE handler\n");
    }

    s = new_server();

    if ((ret = pthread_create(&master_thread, NULL, client_manager, s)) != 0)
        SystemFatal("pthread_create(): Unable to create client management thread\n");

    pthread_join(master_thread, NULL);

    return EXIT_SUCCESS;
}

/**
 * client_manager
 * 
 * Function run by the thread to manage new and existing connections
 * 
 * @param data
 * @return 
 */
void* client_manager(void *data) {
    server *s = (server *) data;

    init_server(s);

    for (; running;) {
        pthread_mutex_lock(&s->dataLock);
        s->num_fds = epoll_wait(s->epoll_fd, s->events, EPOLL_QUEUE_LEN, -1);
        pthread_mutex_unlock(&s->dataLock);

        if (s->num_fds < 0)
            SystemFatal("epoll_wait(): Error\n");

        read_from_socket(s);
    }

    free(s);
    pthread_exit(NULL);
}

/**
 * read_from_socket
 * 
 * looks for and handles the event occurring on the descriptors
 * 
 * @param s server structure
 */
void read_from_socket(server *s) {
    int i, sd_new, fwd_sd;
    static fd_info *newfdinfo, *fdinfo, *fwd_fdinfo;
    static port_fwd *local_to_remote, *remote_to_local;
    struct sockaddr_in svr_fwd;
    struct hostent *hp;
    ipinfo *ip_info = NULL;
    client *c = NULL;

    newfdinfo = malloc(sizeof (fd_info));
    fwd_fdinfo = malloc(sizeof (fd_info));
    ip_info = malloc(sizeof (ipinfo));
    local_to_remote = malloc(sizeof (port_fwd));
    remote_to_local = malloc(sizeof (port_fwd));

    for (i = 0; i < s->num_fds; i++) {
        /* Error check */
        if (s->events[i].events & (EPOLLHUP | EPOLLERR)) {
            fprintf(stderr, "epoll(): EPOLLERR\n");
            continue;
        }
        assert(s->events[i].events & EPOLLIN);
        c = client_new();
        c->sa_len = sizeof (c->sa);
        fdinfo = (fd_info *) (s->events[i].data.ptr);
        if (fdinfo->first_time) {
            pthread_mutex_lock(&s->dataLock);
            
            sd_new = accept(fdinfo->sd, (struct sockaddr *) &c->sa, &c->sa_len);
            if (sd_new == -1) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("accept(): Error\n");
                }
                pthread_mutex_lock(&s->dataLock);
                continue;
            }
            fprintf(stderr, " >>> PORT FWD: Accepted Connection from Machine 1 [%s:%d]\n",
                    inet_ntoa(c->sa.sin_addr),
                    ntohs(c->sa.sin_port));
            // Make the fd_new non-blocking
            if (fcntl(sd_new, F_SETFL, O_NONBLOCK | fcntl(sd_new, F_GETFL, 0)) == -1) {
                SystemFatal("fcntl(): Error\n");
            }

            newfdinfo->sd = sd_new;
            newfdinfo->first_time = false;

            /* Add the server socket to the epoll event loop  */
            s->event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
            s->event.data.ptr = newfdinfo;

            if (epoll_ctl(s->epoll_fd, EPOLL_CTL_ADD, sd_new, &s->event) == -1) {
                SystemFatal("epoll_ctl(): error - New SD\n");
            }

            // CREATING SOCKET TO FORWARD DATA
            if ((fwd_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
                SystemFatal("Cannot create the forward socket.\n");

            //fprintf(stderr, " >>>> Binding to forward socket\n");
            ip_info = llist_find(s->portfd_list, fdinfo->sd, sd_compare);

            bzero((char *) &svr_fwd, sizeof (struct sockaddr_in));
            svr_fwd.sin_family = AF_INET;
            svr_fwd.sin_port = htons(ip_info->dest_port);
            if ((hp = gethostbyname(ip_info->ipAddress)) == NULL) {
                fprintf(stderr, "  gethostbyname(): host name failed [%s]\n", ip_info->ipAddress);
            }
            bcopy(hp->h_addr, (char *) &svr_fwd.sin_addr, hp->h_length);
            fprintf(stderr, " >>>> Connecting to Machine 3 - [%s:%d].\n",
                    inet_ntoa(svr_fwd.sin_addr),
                    ntohs(svr_fwd.sin_port));

            if (connect(fwd_sd, (struct sockaddr *) &svr_fwd,
                    sizeof (svr_fwd)) == -1) {
                SystemFatal("connect(): Connection Failed\n");
            }

            fwd_fdinfo->sd = fwd_sd;
            fwd_fdinfo->first_time = false;

            /* Add the server socket to the epoll event loop  */
            s->event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
            s->event.data.ptr = fwd_fdinfo;

            if (epoll_ctl(s->epoll_fd, EPOLL_CTL_ADD, fwd_sd, &s->event) == -1) {
                SystemFatal("epoll_ctl(): error - Forward SD\n");
            }

            fprintf(stdout, " >>> PORT FWD: Established Connection with Machine 3 [%s:%d]\n\n", hp->h_name,
                    ip_info->dest_port);
            local_to_remote->src_sd = sd_new;
            local_to_remote->dst_sd = fwd_sd;
            remote_to_local->src_sd = fwd_sd;
            remote_to_local->dst_sd = sd_new;

            s->local_to_remote = llist_append(s->local_to_remote, (void *) local_to_remote);
            s->remote_to_local = llist_append(s->remote_to_local, (void *) remote_to_local);

            pthread_mutex_unlock(&s->dataLock);
            continue;
        }

        pthread_mutex_lock(&s->dataLock);
        if (!process_client_req(s, ((fd_info *) (s->events[i].data.ptr))->sd))
            close(((fd_info *) (s->events[i].data.ptr))->sd);
        pthread_mutex_unlock(&s->dataLock);
    }

}

/**
 * signal_Handler
 *
 * Handles signals that occur during program execution.
 *
 * @param signo The Signal Received
 */
void signal_Handler(int signo) {
    switch (signo) {
        case SIGINT:
            fprintf(stderr, "\nReceived SIGINT signal\n");
            exit(EXIT_FAILURE);
            break;

        case SIGSEGV:
            fprintf(stderr, "\nReceived SIGSEGV signal\n");
            break;

        case SIGPIPE:
            fprintf(stderr, "\nReceived SIGPIPE signal\n");
            break;

        default:
            fprintf(stderr, "\nUn handled signal %s\n", strsignal(signo));
            exit(EXIT_FAILURE);

            break;
    }
}

/**
 * SystemFatal
 *
 * Displays a perror message and exits the program.
 *
 * @param message takes in a string message
 */
void SystemFatal(const char* message) {
    perror(message);
    exit(EXIT_FAILURE);
}
