#include "test_forwarding.h"

/* Init full routing table according to topology t3.txt
 #     R3
 #   /    \
 #  R1     R2
 #   \    /
 #     R4
 */
void init_full_routing_table(routing_table_t *rt) {

    overlay_addr_t r1, r2, r3, r4;
    init_node(&r1, 1, LOCALHOST);
    init_node(&r2, 2, LOCALHOST);
    init_node(&r3, 3, LOCALHOST);
    init_node(&r4, 4, LOCALHOST);

    switch (MY_ID) {
        case 1:
            init_routing_table(rt);
            add_route(rt, 3, &r3, 1);
            add_route(rt, 4, &r4, 1);
            add_route(rt, 2, &r3, 2);
            break;

        case 2:
            init_routing_table(rt);
            add_route(rt, 3, &r3, 1);
            add_route(rt, 4, &r4, 1);
            add_route(rt, 1, &r3, 2);
            break;

        case 3:
            init_routing_table(rt);
            add_route(rt, 1, &r1, 1);
            add_route(rt, 2, &r2, 1);
            add_route(rt, 4, &r1, 2);
            break;

        case 4:
            init_routing_table(rt);
            add_route(rt, 1, &r1, 1);
            add_route(rt, 2, &r2, 1);
            add_route(rt, 3, &r1, 2);
            break;
    }
}
