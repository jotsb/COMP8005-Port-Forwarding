#include "common.h"

/**
 * client_new
 *
 * create a client struct and initialize default variables.
 *
 * @return c returns the client struct
 */
client * client_new(void) {
    client *c = malloc(sizeof (client));
    c->sa_len = sizeof (c->sa);
    return c;
}

/**
 * process_client_req
 *
 * Handles forward data between the request client to the forward
 * server and back from forward server to requesting client.
 *
 * @param s server information
 * @param sd socket descriptor
 */
bool process_client_req(server *s, int sd) {
    int where_to_fwd = -1, bytes_read;
    port_fwd *it = NULL;
    char buffer[BUFLEN];

    it = (port_fwd *) llist_find(s->local_to_remote, sd, sd_compare);
    if (it != NULL)
        where_to_fwd = it->dst_sd;
    else {
        it = (port_fwd *) llist_find(s->remote_to_local, sd, sd_compare);
        if (it != NULL) {
            where_to_fwd = it->dst_sd;
        } else
            fprintf(stderr, "process_client_req(): forward location not found.\n");
    }

    while (1) {
        bytes_read = recv(sd, buffer, BUFLEN, 0);
        if (bytes_read <= 0) {
            return true;
        } else if (bytes_read <= BUFLEN) {
            send(where_to_fwd, buffer, bytes_read, 0);
            if (bytes_read < BUFLEN) {
                return true;
            }
        }
    }
    return true;
}
