/* metrics_packet.h – estrutura com métricas de execução e rede */
#ifndef METRICS_PACKET_H_
#define METRICS_PACKET_H_

#include <stdint.h>

typedef struct {
    // Atributos de energia
    unsigned int cpu_energy_mJ;
    unsigned int lpm_energy_mJ;
    unsigned int radio_tx_energy_mJ;
    unsigned int radio_rx_energy_mJ;
    // Atributos de rede
    long unsigned int latency_ms;
    unsigned int total_sent;
    unsigned int total_received;
    long unsigned int response_time_ms;
    unsigned int transfer_rate;
} __attribute__((packed)) metrics_packet_t;


#endif /* METRICS_PACKET_H_ */
