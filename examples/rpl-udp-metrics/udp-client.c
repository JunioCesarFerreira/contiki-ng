#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uiplib.h"
#include "net/mac/tsch/tsch.h"
#include "sys/energest.h"
#include <stdio.h>
#include <inttypes.h>

#define UDP_CLIENT_PORT   8765
#define UDP_SERVER_PORT   5678
#define SEND_INTERVAL     (10 * CLOCK_SECOND) // Intervalo de envio de pacotes
#define CPU_POWER_ACTIVE  1.8                 // Consumo de energia da CPU ativa (mW)
#define LPM_POWER         0.0545              // Consumo de energia em modo LPM (mW)
#define RADIO_TX_POWER    17.4                // Consumo de energia no rádio (TX) (mW)
#define RADIO_RX_POWER    19.7                // Consumo de energia no rádio (RX) (mW)


/* Variáveis Globais */
static struct simple_udp_connection udp_conn;   // Conexão UDP
static uint64_t last_tx_timestamp = 0;
static uint32_t total_sent = 0, total_received = 0;

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

static void print_own_link_local(void) {
    uip_ds6_addr_t *ll_addr = uip_ds6_get_link_local(ADDR_PREFERRED);
    if(ll_addr != NULL) {
      char addr_str[UIPLIB_IPV6_MAX_STR_LEN];
      uiplib_ipaddr_snprint(addr_str, sizeof(addr_str), &ll_addr->ipaddr);
      printf("My addr link-local IPv6: %s\n", addr_str);
    } else {
      printf("No link-local address available\n");
    }
  }

static float to_seconds(uint64_t time) {
    return (float)time / ENERGEST_SECOND;
}

static void calculate_metrics(metrics_packet_t *metrics) {
    // Cálculo da energia com consumo específico de cada modo (mW)    
    printf("CPU Energy Time in seconds\n");
    float cpu_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));
    float lpm_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_LPM));
    float radio_tx_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));
    float radio_rx_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));

    printf("cpu:%f lpm:%f rtx:%f rrx:%f\n", cpu_time_seconds, lpm_time_seconds, radio_tx_time_seconds, radio_rx_time_seconds);

    metrics->cpu_energy_mJ = CPU_POWER_ACTIVE * cpu_time_seconds;
    metrics->lpm_energy_mJ = LPM_POWER * lpm_time_seconds;
    metrics->radio_tx_energy_mJ = RADIO_TX_POWER * radio_tx_time_seconds;
    metrics->radio_rx_energy_mJ = RADIO_RX_POWER * radio_rx_time_seconds;

    printf("Energy compsumption:\n");
    printf("    energest_cpu_mJ=%d\n", metrics->cpu_energy_mJ);
    printf("    lpm_energy_mJ=%d\n", metrics->lpm_energy_mJ);
    printf("    radio_tx_energy_mJ=%d\n", metrics->radio_tx_energy_mJ);
    printf("    radio_rx_energy_mJ=%d\n", metrics->radio_rx_energy_mJ);

    metrics->total_sent = total_sent;
    metrics->total_received = total_received;

    uint64_t current_time = tsch_get_network_uptime_ticks();
    metrics->latency_ms = (current_time - last_tx_timestamp) * 1000 / CLOCK_SECOND;
    metrics->response_time_ms = metrics->latency_ms;
    metrics->transfer_rate = total_sent / (current_time / CLOCK_SECOND);
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
    printf("UDP RX Sender = %s\n", addr_str);
    total_received++;
    last_tx_timestamp = tsch_get_network_uptime_ticks();
}

/*------------------------Processos do Contiki-NG------------------------*/
PROCESS(udp_client_process, "UDP client");

AUTOSTART_PROCESSES(&udp_client_process);

PROCESS_THREAD(udp_client_process, ev, data) {
    static struct etimer periodic_timer;
    uip_ipaddr_t dest_ipaddr;
    static metrics_packet_t metrics;

    PROCESS_BEGIN();
    
    simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);
    etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
    NETSTACK_MAC.on();

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        if (NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
            total_sent++;
            energest_flush();
            calculate_metrics(&metrics);
            simple_udp_sendto(&udp_conn, &metrics, sizeof(metrics), &dest_ipaddr);
            printf("Sent metrics to server\n");
            print_own_link_local();
        } else {
            printf("Not reachable yet\n");
        }

        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}
