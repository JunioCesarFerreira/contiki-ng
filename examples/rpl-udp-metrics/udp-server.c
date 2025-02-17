#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uiplib.h"
#include <stdio.h>

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678
#define MAX_MOTES 10

static struct simple_udp_connection udp_conn;

typedef struct {
    unsigned int cpu_energy_mJ;
    unsigned int lpm_energy_mJ;
    unsigned int radio_tx_energy_mJ;
    unsigned int radio_rx_energy_mJ;
    long unsigned int latency_ms;
    unsigned int total_sent;
    unsigned int total_received;
    long unsigned int response_time_ms;
    unsigned int transfer_rate;
} metrics_packet_t;

typedef struct {
    uip_ipaddr_t addr;
    unsigned int count;
    int used;
} mote_counter_t;

static mote_counter_t mote_counters[MAX_MOTES];

static int compare_ipaddr(const uip_ipaddr_t *a, const uip_ipaddr_t *b) {
    return memcmp(a, b, sizeof(uip_ipaddr_t));
  }

static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen) 
{
    // Converte o endereço IPv6 para string
    char addr_str[UIPLIB_IPV6_MAX_STR_LEN];
    uiplib_ipaddr_snprint(addr_str, sizeof(addr_str), sender_addr);

    /* Procura na lista se o mote já possui um contador.
    Se não encontrar, aloca um novo slot. */
    int found = 0;
    unsigned int received_count = 1;
    for(int i = 0; i < MAX_MOTES; i++) {
        if(mote_counters[i].used) {
            if(compare_ipaddr(sender_addr, &mote_counters[i].addr) == 0) {
                mote_counters[i].count++;
                received_count = mote_counters[i].count;
                found = 1;
                break;
            }
        } else {
            /* Novo mote: copia o endereço e inicializa o contador */
            memcpy(&mote_counters[i].addr, sender_addr, sizeof(uip_ipaddr_t));
            mote_counters[i].count = 1;
            mote_counters[i].used = 1;
            found = 1;
            break;
        }
    }
    if(!found) {
        printf("No space for mote counter!\n");
    }

    printf("UDP Packet received from %s\n", addr_str);
    
    if (datalen == sizeof(metrics_packet_t)) {
        metrics_packet_t *metrics = (metrics_packet_t *)data;

        printf("Node metrics received from %s\n", addr_str);
        printf("  CPU Energy: %u mJ\n", metrics->cpu_energy_mJ);
        printf("  LPM Energy: %u mJ\n", metrics->lpm_energy_mJ);
        printf("  Radio TX Energy: %u mJ\n", metrics->radio_tx_energy_mJ);
        printf("  Radio RX Energy: %u mJ\n", metrics->radio_rx_energy_mJ);
        printf("  Latency: %lu ms\n", metrics->latency_ms);
        printf("  Total Sent: %u\n", metrics->total_sent);
        printf("  Total Received: %d\n", received_count);
        printf("  Response Time: %lu ms\n", metrics->response_time_ms);
        printf("  Transfer Rate: %u packets/sec\n", metrics->transfer_rate);
    }
}

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);

PROCESS_THREAD(udp_server_process, ev, data) {
    PROCESS_BEGIN();

    printf("UDP Server process started\n");

    NETSTACK_ROUTING.root_start();
    simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL, UDP_CLIENT_PORT, udp_rx_callback);

    PROCESS_END();
}
