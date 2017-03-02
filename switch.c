//
// Created by Ross on 3/2/2017.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "switch.h"
#include "net.h"
#include "host.h"
#include "packet.h"
#include "main.h"


void switch_main(int switch_id) {

    struct net_port *node_port_list;
    struct net_port **node_port;  // Array of pointers to node ports
    int node_port_num;            // Number of node ports

    int i,k,n;


    struct packet *in_packet; /* Incoming packet */
    struct packet *new_packet;

    struct net_port *p;
    struct switch_job *new_job;
    struct switch_job *new_job2;

    struct switch_queue *job_q;
    job_q_init(&job_q);
    struct route **forward_table;


    // Generate linked list with clients
    node_port_list = net_get_port_list(switch_id);

    /*  Count the number of network link ports */
    node_port_num = 0;
    for (p=node_port_list; p!=NULL; p=p->next) {
        node_port_num++;
    }
    /* Create memory space for the array */
    node_port = (struct net_port **)
            malloc(node_port_num*sizeof(struct net_port *));

    // Allocate routing table
    forward_table = (struct route **)
            calloc(node_port_num,sizeof(struct route *));


    /* Load ports into the array */
    p = node_port_list;
    for (k = 0; k < node_port_num; k++) {
        node_port[k] = p;
        p = p->next;
    }
    while(1) {

        /*
         * Get packets from incoming links and translate to jobs
           * Put jobs in job queue
          */

        for (k = 0; k < node_port_num; k++) { /* Scan all ports */

            in_packet = (struct packet *) malloc(sizeof(struct packet));
            n = packet_recv(node_port[k], in_packet);
            // If there was data on that port
            if (n > 0) {

                new_job = (struct switch_job *)
                        malloc(sizeof(struct switch_job));
                new_job->src = (int) in_packet->src;
                new_job->dst = (int) in_packet->dst;
                new_job->packet = in_packet;
                //Packet destination is switch, catch here and discard
                if(new_job->dst == switch_id){
                    free(in_packet);
                    free(new_job);
                    continue;
                }

                //Check and update routing table
                for(i = 0; i < node_port_num; i++){
                    //Table already has entry for client, refresh routes
                    if((int) forward_table[k]->client_id == new_job->src){
                        forward_table[k]->client_port = node_port[k]; // Save ports
                        forward_table[k]->valid = 1; // Mark valid
                        break;
                    } else{ // Table doesn't have entry, find first invalid and replace
                        if((int) forward_table[k]->valid == 0){
                            forward_table[k]->client_port = node_port[k]; // Save ports
                            forward_table[k]->valid = 1; // Mark valid
                        }
                    }
                }

                // Check if the dst is routing table
                new_job->dstport = isRouteable(new_job->dst,forward_table,node_port_num);
                if(new_job->dstport != NULL){
                    new_job->type = JOB_SEND_PKT_ROUTED;
                }else{
                    //Not routable, send to all
                    new_job->type = JOB_SEND_PKT_ALL_PORTS;
                }
                job_q_add(&job_q, new_job); // Add job

            }
            else {
                free(in_packet);
            }
        }

// Packets recieved, process queue

        if (job_q_num(&job_q) > 0) {

            /* Get a new job from the job queue */
            new_job = job_q_remove(&job_q);


            /* Send packet on all ports */
            switch(new_job->type) {

                /* Send packets on all ports */
                case JOB_SEND_PKT_ALL_PORTS:
                    for (k=0; k<node_port_num; k++) {
                        packet_send(node_port[k], new_job->packet);
                    }
                    free(new_job->packet);
                    free(new_job);
                    break;
                case JOB_SEND_PKT_ROUTED:
                    packet_send(new_job->dstport, new_job->packet);
                    free(new_job->packet);
                    free(new_job);
                default:
                    break;
            }

        }



    } /* End of while loop */



}


struct net_port * isRouteable(int dstnode, struct route **table, int tablesize){
    for(int temp = 0; temp < tablesize; temp++){
        if(table[temp]->client_id == dstnode)
            return table[temp]->client_port;
    }

    return NULL;
}



/* Job queue operations */

/* Add a job to the job queue */
void job_q_add(struct switch_queue *j_q, struct switch_job *j)
{
    if (j_q->head == NULL ) {
        j_q->head = j;
        j_q->tail = j;
        j_q->occ = 1;
    }
    else {
        (j_q->tail)->next = j;
        j->next = NULL;
        j_q->tail = j;
        j_q->occ++;
    }
}

/* Remove job from the job queue, and return pointer to the job*/
struct switch_job *job_q_remove(struct switch_queue *j_q)
{
    struct switch_job *j;

    if (j_q->occ == 0) return(NULL);
    j = j_q->head;
    j_q->head = (j_q->head)->next;
    j_q->occ--;
    return(j);
}

/* Initialize job queue */
void job_q_init(struct switch_queue *j_q)
{
    j_q->occ = 0;
    j_q->head = NULL;
    j_q->tail = NULL;
}

int job_q_num(struct switch_queue *j_q)
{
    return j_q->occ;
}