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

/* 
 * Sobre a definição default TLL: 
 * Em redes 6LoWPAN valores tipicos de 30 a 64. O valor limite costuma ser 255. 
 * O valor padrão mais comum em redes RPL/IPv6 é 64.
 */
#define DEFAULT_TTL_HOP_COUNTER 64 

#define PRINT_JSON_LOG // Habilite este flag para realizar print de JSON de métricas, util para obter dados analíticos da simulação.
//#define PRINT_TAB_LOG // Habilite este flag para realizar print de Tabular de métricas, util para depuração e acompanhamento manual.

static struct etimer periodic_timer;
static ping_packet_t ping_pkt = { 0, 0 };

static struct simple_udp_connection udp_conn;

typedef struct {
    uip_ipaddr_t addr;
    uint32_t rx_count;
    uint32_t tx_count;
    uint32_t latency; 
    uint16_t index;
    char used;
} mote_t;

static mote_t motes[MAX_MOTES];

static int compare_ipaddr(const uip_ipaddr_t *a, const uip_ipaddr_t *b) {
    return memcmp(a, b, sizeof(uip_ipaddr_t));
}

static mote_t* rx_handle_mote_counters(const uip_ipaddr_t *sender_addr) {
    /* Procura na lista se o mote já possui um contador.
    Se não encontrar, aloca um novo slot. */
    uint8_t found = 0;
    mote_t* ptr = NULL;
    for (int i = 0; i < MAX_MOTES; i++) {
        if (motes[i].used) {
            if (compare_ipaddr(sender_addr, &motes[i].addr) == 0) {
                motes[i].rx_count++;
                motes[i].index = i;
                found = 1;
                ptr = &motes[i];
                break;
            }
        } else {
            /* Novo mote: copia o endereço e inicializa o contador */
            memcpy(&motes[i].addr, sender_addr, sizeof(uip_ipaddr_t));
            motes[i].rx_count = 1;
            motes[i].tx_count = 0;
            motes[i].used = 1;
            motes[i].index = i;
            found = 1;
            ptr = &motes[i];
            break;
        }
    }
    if (!found) {
        printf("No space for mote counter!\n");
    }
    return ptr;
}

static void send_ping_to_all_nodes(void) {
    ping_pkt.ping_seq++; // incrementa o número de sequência
    ping_pkt.send_timestamp = tsch_get_network_uptime_ticks();

    for (int i = 0; i < MAX_MOTES; i++) {
        if (motes[i].used) {
            motes[i].tx_count++;
            simple_udp_sendto(&udp_conn, &ping_pkt, sizeof(ping_pkt), &motes[i].addr);
            //printf("Sending ping packet seq=%u for mote %d\n", ping_pkt.ping_seq, i);
        }
    }
}

static void metrics_print(char* addr_str, 
                          node_metrics_packet_t* metrics,
                          mote_t* scp_mote,
                          uint64_t now,
                          uint8_t hops,
                          uint16_t datalen) 
{
#ifdef PRINT_JSON_LOG
    // Formata JSON
    printf("{\"node\":\"%s\", ", addr_str);

    printf("\"cpu_energy_mj\":%u, ", metrics->cpu_energy_mJ);
    printf("\"lpm_energy_mj\":%u, ", metrics->lpm_energy_mJ);
    printf("\"radio_tx_energy_mj\":%u, ", metrics->radio_tx_energy_mJ);
    printf("\"radio_rx_energy_mj\":%u, ", metrics->radio_rx_energy_mJ);

    printf("\"node_time\":%lu, ", metrics->current_time);
    printf("\"total_sent\":%u, ", metrics->total_sent);
    printf("\"total_received\":%u, ", metrics->total_received);
    printf("\"bytes_tx\":%u, ", metrics->bytes_tx);
    printf("\"bytes_rx\":%u, ", metrics->bytes_rx);

    printf("\"r2n_latency\":%lu, ", metrics->from_root_to_node_latency);
    printf("\"lqi\":%u, ", metrics->last_lqi);
    printf("\"rssi\":%d, ", metrics->last_rssi);

    printf("\"server_sent\":%u, ", scp_mote->tx_count);
    printf("\"server_received\":%u, ", scp_mote->rx_count);
    printf("\"server_bytes_rx\":%u, ", datalen);

    printf("\"n2r_latency\":%lu, ", (unsigned long)(now - metrics->current_time));
    printf("\"hops\":%u, ", DEFAULT_TTL_HOP_COUNTER - hops);
    printf("\"rtt_latency\":%u, ", scp_mote->latency);
    printf("\"root_time_now\":%lu", now);

    printf("}\n");  // finaliza o JSON

#endif // PRINT_JSON_LOG

#ifdef PRINT_TAB_LOG
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
    printf("    Last RSSI:           %d dBm\n", metrics->last_rssi);
    printf("    Server Sent:         %d\n", scp_mote->tx_count);
    printf("    Server Received:     %d\n", scp_mote->rx_count);
    printf("    Server Bytes RX:     %d\n", datalen);
    printf("    N2R Latency:         %lu ms\n", (long unsigned int)(now - metrics->current_time));
    printf("    Hops Count:          %d\n", DEFAULT_TTL_HOP_COUNTER - hops);
    printf("    Last RTT Latency:    %d ms\n", scp_mote->latency);
    printf("    Root Time Now:       %lu ms\n", now);
#endif // PRINT_TAB_LOG

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

    //printf("UDP Packet received from %s\n", addr_str);

    mote_t* scp_mote = rx_handle_mote_counters(sender_addr);

    uint64_t now = tsch_get_network_uptime_ticks();

    if (datalen == sizeof(ping_packet_t)) {
        ping_packet_t *received_pkt = (ping_packet_t *)data;
        uint64_t rtt = now - received_pkt->send_timestamp;
        motes[scp_mote->index].latency = rtt/2;
    } 
    else if (datalen == sizeof(node_metrics_packet_t)) {
        node_metrics_packet_t *metrics = (node_metrics_packet_t *)data;
        uint8_t hops = UIP_IP_BUF->ttl;
        metrics_print(addr_str, metrics, scp_mote, now, hops, datalen);
    } 
    else {
        printf("Received bytes %d\n", datalen);
    }
}


PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);

PROCESS_THREAD(udp_server_process, ev, data) {
    PROCESS_BEGIN();

    printf("UDP Root process started\n");

    for (int i=0; i<MAX_MOTES; i++) {
        motes[i].used = 0;
        motes[i].latency = 0;
    }

    NETSTACK_ROUTING.root_start();
    simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL, UDP_CLIENT_PORT, udp_rx_callback);

    etimer_set(&periodic_timer, SEND_INTERVAL);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        send_ping_to_all_nodes();
        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}
