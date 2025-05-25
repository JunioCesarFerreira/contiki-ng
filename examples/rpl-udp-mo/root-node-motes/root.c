#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uiplib.h"
#include "net/mac/tsch/tsch.h"
#include <stdio.h>

#include "metrics-packet.h"

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678
#define MAX_MOTES 100
#define SEND_INTERVAL (10 * CLOCK_SECOND)

static struct etimer periodic_timer;
static server_packet_t pkt = { 0, 0 };

static struct simple_udp_connection udp_conn;

typedef struct {
    uip_ipaddr_t addr;
    unsigned int rx_count;
    unsigned int tx_count;
    char used;
} mote_counter_t;

static mote_counter_t mote_counters[MAX_MOTES];

static int compare_ipaddr(const uip_ipaddr_t *a, const uip_ipaddr_t *b) {
    return memcmp(a, b, sizeof(uip_ipaddr_t));
}

static mote_counter_t* rx_handle_mote_counters(const uip_ipaddr_t *sender_addr) {
    /* Procura na lista se o mote já possui um contador.
    Se não encontrar, aloca um novo slot. */
    int found = 0;
    mote_counter_t* ptr = NULL;
    for (int i = 0; i < MAX_MOTES; i++) {
        if (mote_counters[i].used) {
            if (compare_ipaddr(sender_addr, &mote_counters[i].addr) == 0) {
                mote_counters[i].rx_count++;
                found = 1;
                ptr = &mote_counters[i];
                break;
            }
        } else {
            /* Novo mote: copia o endereço e inicializa o contador */
            memcpy(&mote_counters[i].addr, sender_addr, sizeof(uip_ipaddr_t));
            mote_counters[i].rx_count = 1;
            mote_counters[i].tx_count = 0;
            mote_counters[i].used = 1;
            found = 1;
            ptr = &mote_counters[i];
            break;
        }
    }
    if (!found) {
        printf("No space for mote counter!\n");
    }
    return ptr;
}

static void send_packets_to_all_nodes(void) {
    pkt.seq++; // incrementa o número de sequência
    pkt.time = tsch_get_network_uptime_ticks();

    for (int i = 0; i < MAX_MOTES; i++) {
        if (mote_counters[i].used) {
            mote_counters[i].tx_count++;
            simple_udp_sendto(&udp_conn, &pkt, sizeof(pkt), &mote_counters[i].addr);
            printf("Sending packet seq=%u for mote %d\n", pkt.seq, i);
        }
    }
}


static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen) 
{
    uint32_t timestamp = tsch_get_network_uptime_ticks();
    // Converte o endereço IPv6 para string
    char addr_str[UIPLIB_IPV6_MAX_STR_LEN];
    uiplib_ipaddr_snprint(addr_str, sizeof(addr_str), sender_addr);

    printf("UDP Packet received from %s\n", addr_str);

    mote_counter_t* scp_mote = rx_handle_mote_counters(sender_addr);

    if (datalen == sizeof(node_metrics_packet_t)) {
        node_metrics_packet_t *metrics = (node_metrics_packet_t *)data;
        uint8_t hops = UIP_IP_BUF->ttl;

        printf("Node metrics received from %s\n", addr_str);
        printf("    CPU Energy:          %u mJ\n", metrics->cpu_energy_mJ);
        printf("    LPM Energy:          %u mJ\n", metrics->lpm_energy_mJ);
        printf("    Radio TX Energy:     %u mJ\n", metrics->radio_tx_energy_mJ);
        printf("    Radio RX Energy:     %u mJ\n", metrics->radio_rx_energy_mJ);
        printf("    Node Time:           %lu ms\n", metrics->current_time);
        printf("    Node Total Sent:     %u\n", metrics->total_sent);
        printf("    Node Total Received: %u\n", metrics->total_received);
        printf("    Node Bytes TX:       %u\n", metrics->bytes_tx);
        printf("    Node Bytes RX:       %u\n", metrics->bytes_rx);
        printf("    R2N Latency:         %lu ms\n", metrics->from_root_to_node_latency);
        printf("    Last LQI:            %d\n", metrics->last_lqi);
        printf("    Last RSSI:           %d\n", metrics->last_rssi);
        printf("    Server Sent:         %d\n", scp_mote->tx_count);
        printf("    Server Received:     %d\n", scp_mote->rx_count);
        printf("    Server Bytes RX:     %d\n", datalen);
        printf("    N2R Latency:         %lu ms\n", (long unsigned int)(timestamp - metrics->current_time));
        printf("    HOPS:                %d\n", hops);
    } else {
        printf("Received bytes %d\n", datalen);
    }
}


PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);

PROCESS_THREAD(udp_server_process, ev, data) {
    PROCESS_BEGIN();

    printf("UDP Server process started\n");

    NETSTACK_ROUTING.root_start();
    simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL, UDP_CLIENT_PORT, udp_rx_callback);

    etimer_set(&periodic_timer, SEND_INTERVAL);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        send_packets_to_all_nodes();
        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}
