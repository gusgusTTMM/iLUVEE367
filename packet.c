
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "packet.h"
#include "net.h"


void packet_send(struct net_port *port, struct packet *p) {
    char msg[PAYLOAD_MAX + 4];
    int i;

    if (port->type == PIPE) {
        msg[0] = (char) p->src;
        msg[1] = (char) p->dst;
        msg[2] = (char) p->type;
        msg[3] = (char) p->length;
        for (i = 0; i < p->length; i++) {
            msg[i + 4] = p->payload[i];
        }
        write(port->pipe_send_fd, msg, p->length + 4);
//printf("PACKET SEND, src=%d dst=%d p-src=%d p-dst=%d\n", 
//		(int) msg[0], 
//		(int) msg[1], 
//		(int) p->src, 
//		(int) p->dst);
    }

    return;
}

int packet_recv(struct net_port *port, struct packet *p) {
    char msg[PAYLOAD_MAX + 4];
    int n;
    int i;

    if (port->type == PIPE) {
        n = (int) read(port->pipe_recv_fd, msg, PAYLOAD_MAX + 4);
        if (n > 0) {
            p->src = (char) msg[0];
            p->dst = (char) msg[1];
            p->type = (char) msg[2];
            p->length = (int) msg[3];
            for (i = 0; i < p->length; i++) {
                p->payload[i] = msg[i + 4];
            }

// printf("PACKET RECV, src=%d dst=%d p-src=%d p-dst=%d\n", 
//		(int) msg[0], 
//		(int) msg[1], 
//		(int) p->src, 
//		(int) p->dst);
        }
    }

    return (n);
}

void switch_send(struct packet *netdata) {

    packet_send(getSwitch(), netdata);

}

struct net_port *getSwitch() {
    struct net_port *node_list;
    struct net_port *p_node;
    node_list = net_get_port_list(-1);

    for (p_node = node_list; p_node != NULL; p_node = p_node->next) {
        if (p_node->type == SWITCH)
            return p_node;
    }
    return NULL;
}
