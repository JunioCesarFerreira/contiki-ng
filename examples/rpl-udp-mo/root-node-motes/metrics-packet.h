/* metrics_packet.h – estrutura com métricas de execução e rede */
#ifndef METRICS_PACKET_H_
#define METRICS_PACKET_H_

#include <stdint.h>

typedef struct {
    unsigned int packet_number;
    long unsigned int current_time;
    // Atributos de energia
    unsigned int cpu_energy_mJ;
    unsigned int lpm_energy_mJ;
    unsigned int radio_tx_energy_mJ;
    unsigned int radio_rx_energy_mJ;
    // Atributos de rede
    unsigned int total_sent;
    unsigned int total_received;
    unsigned int bytes_tx;
    unsigned int bytes_rx;
    long unsigned int from_root_to_node_latency;
    // Conectividade
    int last_rssi;
    int last_lqi;
} __attribute__((packed)) node_metrics_packet_t;
//__attribute__((packed)) garante que o servidor interprete o payload exatamente como enviado, mesmo em arquiteturas onde o alignment natural difere.

typedef struct {
    unsigned int ping_seq;
    long unsigned int send_timestamp; // ts do root
} __attribute__((packed)) ping_packet_t;

#endif /* METRICS_PACKET_H_ */
