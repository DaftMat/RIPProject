#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> // inet_addr, htons
#include <time.h>

#include "router.h"
#include "console.h"
#include "packet.h"
#include "test_forwarding.h"

#define BUF_SIZE 1024
#define RTR_BASE_PORT 5555
#define BROADCAST_PERIOD 10
#define FWD_DELAY_IN_MS 10
#define LOG_MSG_MAX_SIZE 256
#define INFINITY 255

#define PORT(x) (x+RTR_BASE_PORT)

/* ==================================================================== */
/* ========================= LOG FUNCTIONS ============================ */
/* ==================================================================== */

// Log message to file log/Ri.txt
// Format: DATE [TAG]: MESSAGE
void logger(const char *tag, const char *message, ...) {

    time_t now;
    char buf[256], file_name[32];
    va_list params;

    time(&now);
    strncpy(buf, ctime(&now), sizeof(buf));
    buf[strlen(buf)-1]='\0'; // remove new line from ctime function

    sprintf(file_name, "%s%d%s", "log/R", MY_ID, ".txt");
    FILE *f = fopen(file_name, "at");

    fprintf(f, "%s [%s]: ", buf, tag);
    va_start(params, message);
    vfprintf(f, message, params);
    fprintf(f, ".\n");
    va_end(params);
    fclose(f);
}

// Log Distance Vector (DV) included in packet *p
// if output then the DV is sent to neigh, else it is received from neigh
void log_dv(packet_ctrl_t *p, node_id_t neigh, int output) {

    char buf_dv[256];
    char buf_dve[32];
    strcpy(buf_dv, "\t DEST | METRIC \n");
    for (int i=0; i<p->dv_size; i++) {
        sprintf(buf_dve, "\t   %d  |  %d\n", p->dv[i].dest, p->dv[i].metric);
        strcat(buf_dv, buf_dve);
    }
    if (output)
        logger("HELLO TH", "DV sent to R%d :\n %s", neigh, buf_dv);
    else
        logger("SERVER TH", "DV received from R%d :\n %s", neigh, buf_dv);
}

/* ==================================================================== */
/* =============== INIT NEIGHBORS AND ROUTING TABLE =================== */
/* ==================================================================== */

// Init node's overlay address
void init_node(overlay_addr_t *addr, node_id_t id, char *ip) {

    addr->id = id;
    addr->port = PORT(id);
    strcpy(addr->ipv4, ip);
}

// Add node to neighbor's table
void add_neighbor(neighbors_table_t *ntable, const overlay_addr_t *node) {

    assert(ntable->size < MAX_NEIGHBORS);
    ntable->nt[ntable->size] = *node;
    ntable->size++;
}

// Read topo from conf file
void read_neighbors(char *file, int rid, neighbors_table_t *nt) {

    FILE *fichier = NULL;
    char ligne[80];
    int id = 0;
    overlay_addr_t node;
    char *token;

   	fichier = fopen(file, "rt");
   	if (fichier == NULL) {
   		perror("[Config] Error opening configuration file.\n");
   		exit(EXIT_FAILURE);
   	}

    while (!feof(fichier)) {
        // read line
        fgets(ligne, sizeof(ligne), fichier);
        ligne[strlen(ligne)-1]='\0'; // remove '\n'
        // printf("%s\n", ligne);
        if (ligne[0]!='#') {
            sscanf(ligne, "%d", &id);
            if (id == rid) {
                // read neighbors
                token = strtok(ligne, " ");
                token = strtok(NULL, " "); // discard first number (rid)
                while (token != NULL) {
                    // printf( "|%s|", token );
                    id = atoi(token);
                    init_node(&node, id, "127.0.0.1");
                    add_neighbor(nt, &node);
                    token = strtok(NULL, " ");
                }
                fclose(fichier);
                return ;
            }
        }
    }
    fclose(fichier);
}

// Add route to routing table
void add_route(routing_table_t *rtable, node_id_t dest, const overlay_addr_t *next, short metric) {
    assert(rtable->size < MAX_ROUTES);
    rtable->rt[rtable->size].dest    = dest;
    rtable->rt[rtable->size].nexthop = *next;
    rtable->rt[rtable->size].metric  = metric;
    rtable->rt[rtable->size].time    = time(NULL);
    rtable->size++;
}

// Init routing table with one entry (myself)
void init_routing_table(routing_table_t *rt) {

    overlay_addr_t me;
    init_node(&me, MY_ID, LOCALHOST);
    add_route(rt, MY_ID, &me, 0);
}


/* ========================================= */
/* ========== FORWARD DATA PACKET ========== */
/* ========================================= */

/* ========================================================================= */
/* ************************ A FAIRE PAR LES ETUDIANTS ********************** */
/* ========================================================================= */
int forward_packet(packet_data_t *packet, int psize, routing_table_t *rt) {
    //initialization
    int sock = 0;
    bool found = false;
    struct sockaddr_in to;
    overlay_addr_t dest;
    memset(&to, 0, sizeof(to));
    memset(&dest, 0, sizeof(dest));

    //find the right router to send
    for (int i = 0 ; i < rt->size ; ++i){
        if (rt->rt[i].dest == packet->dst_id){
            //get overlay_addr
            dest = rt->rt[i].nexthop;
            found = true;
            break;
        }
    }

    if (!found){
        return 0;
    }

    //open socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    //send packet to socket
    to.sin_family = AF_INET;
    to.sin_port = htons(dest.port);
    to.sin_addr.s_addr = inet_addr(dest.ipv4);
    if (sendto(sock, packet, psize, 0, (struct sockaddr*)&to, sizeof(to)) < 0){
        perror("Sending error");
        exit(EXIT_FAILURE);
    }

    close(sock);
    return 1;
}
/* ========================================================================= */
/* *************************** FIN FORWARD PACKET ************************** */
/* ========================================================================= */


/* ==================================================================== */
/* ========================== HELLO THREAD ============================ */
/* ==================================================================== */

/* ========================================================================= */
/* ************************ A FAIRE PAR LES ETUDIANTS ********************** */
/* ========================================================================= */

// Build distance vector packet
void build_dv_packet(packet_ctrl_t *p, routing_table_t *rt) {
    p->type = CTRL;
    p->src_id = rt->rt[0].dest;
    p->dv_size = rt->size;
    for (int i = 0 ; i < rt->size ; ++i){
        p->dv[i].dest = rt->rt[i].dest;
        p->dv[i].metric = rt->rt[i].metric;
    }
}

// DV to prevent count to infinity problem
// Build a DV that contains the routes that have not been learned via this neighbour
void build_dv_specific(packet_ctrl_t *p, routing_table_t *rt, node_id_t neigh) {
    p->type = CTRL;
    p->src_id = rt->rt[0].dest;
    p->dv_size = 0;
    for (int i = 0 ; i < rt->size ; ++i){
        if (rt->rt[i].nexthop.id != neigh && rt->rt[i].dest != neigh){
            p->dv[p->dv_size].dest = rt->rt[i].dest;
            p->dv[p->dv_size].metric = rt->rt[i].metric;
            p->dv_size++;
        }
    }
}

//remove an entry from routing table
void remove_entry(routing_table_entry_t *rt, int i, int size){
    for (i = i+1; i < size ; ++i){
        rt[i-1] = rt[i];
    }
}

// Remove old RT entries
void remove_obsolete_entries(routing_table_t *rt) {
    //Do not remove itself.
    for (int i = 1 ; i < rt->size ; ++i){
        if (difftime(rt->rt[i].time, time(NULL)) < 0){
            --rt->size;
            remove_entry(rt->rt, i, rt->size);
            --i;
        }
    }

}

// Hello thread to broadcast state to neighbors
void *hello(void *args) {

    /* Cast the pointer to the right type */
    struct th_args *pargs = (struct th_args *) args;
    
    //prepare data
    packet_ctrl_t p = { 0 };
    int sock = 0;
    struct sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;

    while (1) {
        //open socket
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
            perror("Socket error");
            exit(EXIT_FAILURE);
        }

        //send to all neighbors
        for (int i = 0 ; i < pargs->nt->size ; ++i){ 
            //v1
            //build_dv_packet(&p, pargs->rt);
            //v2
            build_dv_specific(&p, pargs->rt, pargs->nt->nt[i].id);
            to.sin_port = htons(pargs->nt->nt[i].port);
            to.sin_addr.s_addr = inet_addr(pargs->nt->nt[i].ipv4);
            if (sendto(sock, &p, sizeof(p), 0, (struct sockaddr*)&to, sizeof(to)) < 0){
                perror("Sending error");
                exit(EXIT_FAILURE);
            }
            log_dv(&p, pargs->nt->nt[i].id, i);
        }

        close(sock);

        sleep(BROADCAST_PERIOD);
        remove_obsolete_entries(pargs->rt);
    }
}

/* ==================================================================== */
/* ======================== UDP SERVER THREAD ========================= */
/* ==================================================================== */

/* ========================================================================= */
/* ************************ A FAIRE PAR LES ETUDIANTS ********************** */
/* ========================================================================= */


// Update routing table from received distance vector
int update_rt(routing_table_t *rt, overlay_addr_t *src, dv_entry_t dv[], int dv_size) {
    bool found = false;

    //for each entries in dv
    for (int i = 0 ; i < dv_size ; ++i){
        //for each entries in rt
        for (int j = 0 ; j < rt->size ; ++j){
            //if it already exists
            if (dv[i].dest == rt->rt[j].dest){
                //update it only if it's necessary
                if (dv[i].metric+1 < rt->rt[j].metric || rt->rt[j].nexthop.id == src->id){
                    rt->rt[j].nexthop = *src;
                    rt->rt[j].metric = (dv[i].metric)+1;
                    rt->rt[j].time = time(NULL);
                }
                found = true;
            }
        }
        //else
        if (!found){
            add_route(rt, dv[i].dest, src, dv[i].metric+1);
        }
        found = false;
    }
    
    return 0;
}

// Server thread waiting for input packets
void *process_input_packets(void *args) {

    int sock;
    struct sockaddr_in my_adr, neigh_adr;
    socklen_t adr_len = sizeof(struct sockaddr_in);
    char buffer_in[BUF_SIZE];
    /* Cast the pointer to the right type */
    struct th_args *pargs = (struct th_args *) args;

    // routing_table_t *rt = (routing_table_t *) arg;
    int port = PORT(MY_ID);
    int size = 0;

    /* Create (server) socket */
    /* ---------------------- */
    if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    /* Bind address and port */
    /*-----------------------*/
    /* Init server adr  */
    memset(&my_adr, 0, sizeof(my_adr));
    my_adr.sin_family = AF_INET;
    my_adr.sin_port = htons(port);
    my_adr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *) &my_adr, sizeof(my_adr)) < 0) {
        perror("bind error");
        close(sock);
        exit(EXIT_FAILURE);
    }

    logger("SERVER TH","waiting for incoming messages");
    while (1) {

        if ( (size = recvfrom(sock, buffer_in, BUF_SIZE, 0, (struct sockaddr *)&neigh_adr, &adr_len)) < 0 ) {
            perror("recvfrom error");
            exit(EXIT_FAILURE);
        }

        switch (buffer_in[0]) {

            case DATA:
                logger("SERVER TH","DATA packet received");
                packet_data_t *pdata = (packet_data_t *) buffer_in;
                if (pdata->dst_id == MY_ID) {
                    switch (pdata->subtype) {
                        case ECHO_REQUEST:
                            send_ping_reply(pdata, pargs->rt);
                            break;
                        case ECHO_REPLY:
                            print_ping_reply(pdata);
                            break;
                        case TR_REQUEST:
                            send_traceroute_reply(pdata, pargs->rt);
                            break;
                        case TR_TIME_EXCEEDED:
                            print_traceroute_path(pdata);
                            break;
                        case TR_ARRIVED:
                            print_traceroute_last(pdata);
                            break;
                        default:
                            logger("SERVER TH","unidentified data packet received");
                    }
                }else {
                    /* I am NOT the recipient ==> forward packet */
                    pdata->ttl--;
                    if (!pdata->ttl){
                        send_time_exceeded(pdata, pargs->rt);
                    }else{
                        forward_packet(pdata, sizeof(pdata), pargs->rt);
                    }
                }
                break;

            case CTRL:
                logger("SERVER TH","CTRL packet received");
                packet_ctrl_t *pctrl = (packet_ctrl_t *) buffer_in;
                log_dv(pctrl, pctrl->src_id, 0);
                
                overlay_addr_t src = { 0 };
                src.id = pctrl->src_id;
                strncpy(src.ipv4,inet_ntoa(neigh_adr.sin_addr),IPV4_ADR_STRLEN);
                src.port = PORT(src.id);
                update_rt(pargs->rt, &src, pctrl->dv, pctrl->dv_size);
                break;

            default:
                // drop
                logger("SERVER TH","unidentified packet received");
                break;
        }
    }
}

/* ==================================================================== */
/* ========================== MAIN PROGRAM ============================ */
/* ==================================================================== */

void process_command(char *cmd, routing_table_t *rt, neighbors_table_t *nt) {

    pthread_t th_id;

    if (!strcmp(cmd, HELP)) {
        print_help();
        return;
    }
    if (!strcmp(cmd, CLEAR)) {
        clear_screen();
        return;
    }
    if (!strcmp(cmd, SH_IP_ROUTE)) {
        print_rt(rt);
        return;
    }
    if (!strcmp(cmd, SH_IP_NEIGH)) {
        print_neighbors(nt);
        return;
    }
    if (!strncmp(cmd, PING, strlen(PING)) && cmd[strlen(PING)]==' ') {
        char temp[16];
        int did;
        sscanf(cmd, "%s%d", temp, &did);
        struct ping_traceroute_args args = {did, rt};
        pthread_create(&th_id, NULL, &ping, &args);
        pthread_join(th_id, NULL);
        return;
    }
    if (!strncmp(cmd, PINGFORCE, strlen(PINGFORCE))) {
        char temp[16];
        int did;
        sscanf(cmd, "%s%d", temp, &did);
        struct ping_traceroute_args args = {did, rt};
        pthread_create(&th_id, NULL, &pingforce, &args);
        pthread_join(th_id, NULL);
        return;
    }
    if (!strncmp(cmd, TRACEROUTE, strlen(TRACEROUTE))) {
        char temp[16];
        int did;
        sscanf(cmd, "%s%d", temp, &did);
        struct ping_traceroute_args args = {did, rt};
        pthread_create(&th_id, NULL, &traceroute, &args);
        pthread_join(th_id, NULL);
        return;
    }
    if (strlen(cmd)!=0)
        print_unknown_command();
}

int main(int argc, char **argv) {

    routing_table_t myrt;
    neighbors_table_t mynt;
    pthread_t th1_id, th2_id;
    struct th_args args;
    int test_forwarding = 0;

    if (argc!=3) {
        printf("Usage: %s <id> <net_topo_conf>\n", argv[0]);
        printf("or\n");
        printf("Usage: %s <id> --test-forwarding\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // ==== Init ROUTER ====
    myrt.size = 0;
    mynt.size = 0;
    int rid = atoi(argv[1]);
    MY_ID = rid; // shared ID between threads
    printf("**************\n");
    printf("* RTR ID : %d *\n", MY_ID);
    printf("**************\n");

    if (strcmp(argv[2], "--test-forwarding") == 0) {
        init_full_routing_table(&myrt);
        test_forwarding = 1;
    }
    else {
        read_neighbors(argv[2], rid, &mynt);
        init_routing_table(&myrt);
    }
    // ====================
    // print_neighbors(&mynt);
    // print_rt(&myrt);
    args.rt = &myrt;
    args.nt = &mynt;

    /* Create a new thread th1 (process input packets) */
    pthread_create(&th1_id, NULL, &process_input_packets, &args);
    logger("MAIN TH","process input packets thread created with ID %u", (int) th1_id);

    if ( !test_forwarding ) {
        /* Create a new thread th2 (hello broadcast) */
        pthread_create(&th2_id, NULL, &hello, &args);
        logger("MAIN TH","hello thread created with ID %u", (int) th2_id);
    }

    int quit=0, len;
    char *command = NULL;
    size_t size;
    while (!quit) {
        print_prompt();
        len = getline(&command, &size, stdin);
        command[len-1] = '\0'; // remove newline
        quit = !strcmp("quit", command) || !strcmp("exit", command);
        if (!quit)
            process_command(command, &myrt, &mynt);
        free(command);
        command = NULL;
    }

    return EXIT_SUCCESS;
}
