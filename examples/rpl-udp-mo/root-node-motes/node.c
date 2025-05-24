#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uiplib.h"
#include "net/mac/tsch/tsch.h"
#include "sys/energest.h"
#include "random.h"

#include "metrics-packet.h"

#define UDP_CLIENT_PORT   8765
#define UDP_SERVER_PORT   5678
#define SEND_INTERVAL     (10 * CLOCK_SECOND) // Intervalo de envio de pacotes
#define CPU_POWER_ACTIVE  1.8                 // Consumo de energia da CPU ativa (mW)
#define LPM_POWER         0.0545              // Consumo de energia em modo LPM (mW)
#define RADIO_TX_POWER    17.4                // Consumo de energia no rádio (TX) (mW)
#define RADIO_RX_POWER    19.7                // Consumo de energia no rádio (RX) (mW)

/* DEBUG DEFINES */
//#define DEBUG_ENERGY_TIME_IN_SECONDS
//#define DEBUG_PRINT_METRICS_PACKET
//#define DEBUG_RX_CALLBACK

/* Variáveis Globais */
static struct simple_udp_connection udp_conn;   // Conexão UDP
static uint64_t root_to_node_latency = 0;
static uint32_t total_sent = 0, total_received = 0;
static uint16_t bytes_tx = 0, bytes_rx = 0;

/* Funções auxiliares */
static float to_seconds(uint64_t time) {
    return (float)time / ENERGEST_SECOND;
}

static void print_own_link_local(void) {
    uip_ds6_addr_t *ll_addr = uip_ds6_get_link_local(ADDR_PREFERRED);
    if (ll_addr != NULL) {
      char addr_str[UIPLIB_IPV6_MAX_STR_LEN];
      uiplib_ipaddr_snprint(addr_str, sizeof(addr_str), &ll_addr->ipaddr);
      printf("My addr link-local IPv6: %s\n", addr_str);
    } else {
      printf("No link-local address available\n");
    }
}

#ifdef DEBUG_PRINT_METRICS_PACKET
static void print_metrics(node_metrics_packet_t *metrics) {
    printf("Packet:\n");
    printf("    number: %d\n", metrics->packet_number);
    printf("    time: %lu\n", metrics->current_time);
    printf("Energy compsumption:\n");
    printf("    energest_cpu_mJ=%d\n", metrics->cpu_energy_mJ);
    printf("    lpm_energy_mJ=%d\n", metrics->lpm_energy_mJ);
    printf("    radio_tx_energy_mJ=%d\n", metrics->radio_tx_energy_mJ);
    printf("    radio_rx_energy_mJ=%d\n", metrics->radio_rx_energy_mJ);
    printf("Network:\n");
    printf("    total_sent=%d\n", metrics->total_sent);
    printf("    total_received=%d\n", metrics->total_received);
    printf("    bytes_tx=%d\n", metrics->bytes_tx);
    printf("    bytes_rx=%d\n", metrics->bytes_rx);
    printf("    from_root_to_node_latency=%lu\n", metrics->from_root_to_node_latency);
}
#endif // DEBUG_PRINT_METRICS_PACKET

static void fill_node_metrics_packet(node_metrics_packet_t *metrics) {
    // Cálculo da energia com consumo específico de cada modo (mW)    
    float cpu_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));
    float lpm_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_LPM));
    float radio_tx_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));
    float radio_rx_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));

#ifdef DEBUG_ENERGY_TIME_IN_SECONDS
    printf("CPU Energy Time in seconds\n");
    printf("cpu:%f lpm:%f rtx:%f rrx:%f\n", cpu_time_seconds, lpm_time_seconds, radio_tx_time_seconds, radio_rx_time_seconds);
#endif // DEBUG_ENERGY_TIME_IN_SECONDS

    metrics->cpu_energy_mJ = CPU_POWER_ACTIVE * cpu_time_seconds;
    metrics->lpm_energy_mJ = LPM_POWER * lpm_time_seconds;
    metrics->radio_tx_energy_mJ = RADIO_TX_POWER * radio_tx_time_seconds;
    metrics->radio_rx_energy_mJ = RADIO_RX_POWER * radio_rx_time_seconds;

    metrics->total_sent = total_sent;
    metrics->total_received = total_received;
    metrics->bytes_tx = bytes_tx;
    metrics->bytes_rx = bytes_rx;

    metrics->packet_number = total_sent-1;
    metrics->current_time = tsch_get_network_uptime_ticks();
    metrics->from_root_to_node_latency = root_to_node_latency;
}

static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen) 
{
#ifdef DEBUG_RX_CALLBACK
    // Converte o endereço IPv6 para string
    char addr_str[UIPLIB_IPV6_MAX_STR_LEN];
    uiplib_ipaddr_snprint(addr_str, sizeof(addr_str), sender_addr);
    printf("UDP RX Sender = %s\n", addr_str);
    printf("Received bytes = %d\n", datalen);
#endif // DEBUG_RX_CALLBACK
    total_received++;
    bytes_rx = datalen;
    uint64_t current_time = tsch_get_network_uptime_ticks();
    
    if (datalen == sizeof(server_packet_t)) {
        server_packet_t *server_pkt = (server_packet_t *)data;
        root_to_node_latency = current_time - server_pkt->time;
    }
}

/*------------------------Processos do Contiki-NG------------------------*/
PROCESS(udp_client_process, "UDP client");

AUTOSTART_PROCESSES(&udp_client_process);

PROCESS_THREAD(udp_client_process, ev, data) {
    static struct etimer periodic_timer;
    uip_ipaddr_t dest_ipaddr;
    static node_metrics_packet_t metrics;

    PROCESS_BEGIN();
    
    simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);
    etimer_set(&periodic_timer, SEND_INTERVAL);
    NETSTACK_MAC.on();

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        if (NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
            total_sent++;
            bytes_tx = sizeof(metrics);
            energest_flush();
            fill_node_metrics_packet(&metrics);
            simple_udp_sendto(&udp_conn, &metrics, bytes_tx, &dest_ipaddr);
            print_own_link_local();
#ifdef DEBUG_PRINT_METRICS_PACKET
            print_metrics(&metrics);
#endif // DEBUG_ENERGY_TIME_IN_SECONDS
        } else {
            printf("Not reachable yet\n");
        }

        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}
