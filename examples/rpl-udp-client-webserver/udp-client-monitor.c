#include "contiki.h"
#include <string.h>
#include "net/routing/routing.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include <time.h>
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/mac/tsch/tsch.h"
#include "lib/random.h"
#include "sys/node-id.h"
#include "sys/log.h"
#include "sys/energest.h"
#include "webserver/httpd-simple.h"

/* Definições de Constantes */
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678
#define SEND_INTERVAL (60 * CLOCK_SECOND) // Intervalo de envio de pacotes
#define CPU_POWER_ACTIVE 1.8              // Consumo de energia da CPU ativa (mW)
#define LPM_POWER 0.0545                  // Consumo de energia em modo LPM (mW)
#define RADIO_TX_POWER 17.4               // Consumo de energia no rádio (TX) (mW)
#define RADIO_RX_POWER 19.7               // Consumo de energia no rádio (RX) (mW)

/* Variáveis Globais */
static struct simple_udp_connection client_conn; // Conexão UDP
static uint32_t latency_sensor = 0;             // Latência do sensor
static uint32_t cpu_energy_mJ = 0;              // Energia consumida pela CPU (mJ)
static uint32_t lpm_energy_mJ = 0;              // Energia consumida em LPM (mJ)
static uint32_t radio_tx_energy_mJ = 0;         // Energia consumida no rádio (TX) (mJ)
static uint32_t radio_rx_energy_mJ = 0;         // Energia consumida no rádio (RX) (mJ)

/* Protótipos de Funções */
static void udp_rx_callback(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port, const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port, const uint8_t *data, uint16_t datalen);
static void calculate_cpu_energy(void);
static uint32_t to_seconds(uint64_t time);
static PT_THREAD(generate_routes(struct httpd_state *s));
static void setLatency(uint64_t *latency_in);
static uint64_t getLatency(void);

/*------------------------Processos do Contiki-NG------------------------*/

/* Processo: Servidor Web */
PROCESS(webserver_nogui_process, "Web server AWS");
/* Processo: Monitoramento de Energia */
PROCESS(energest_example_process, "Energest example process");
/* Processo: Nó RPL (Latência e Comunicação) */
PROCESS(node_process, "RPL Node");

/* Inicialização Automática dos Processos */
AUTOSTART_PROCESSES(&webserver_nogui_process, &energest_example_process, &node_process);

/*------------------------Funções de Apoio------------------------*/

/**
 * Converte o tempo de ticks para segundos.
 * @param time Tempo em ticks.
 * @return Tempo em segundos.
 */
static uint32_t to_seconds(uint64_t time) {
    return (uint32_t)(time / ENERGEST_SECOND);
}

/**
 * Define a latência do sensor.
 * @param latency_in Valor da latência.
 */
static void setLatency(uint64_t *latency_in) {
    if (latency_in != NULL) {
        latency_sensor = *latency_in;
    }
}

/**
 * Retorna a latência do sensor.
 * @return Latência do sensor.
 */
static uint64_t getLatency() {
    return latency_sensor;
}

/*------------------------Cálculo de Energia------------------------*/

/**
 * Calcula o consumo de energia da CPU, LPM, rádio (TX e RX).
 */
static void calculate_cpu_energy() {
    uint32_t cpu_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));
    uint32_t lpm_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_LPM));
    uint32_t radio_tx_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));
    uint32_t radio_rx_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));

    cpu_energy_mJ = CPU_POWER_ACTIVE * cpu_time_seconds;
    lpm_energy_mJ = LPM_POWER * lpm_time_seconds;
    radio_tx_energy_mJ = RADIO_TX_POWER * radio_tx_time_seconds;
    radio_rx_energy_mJ = RADIO_RX_POWER * radio_rx_time_seconds;
}

/*------------------------Callback UDP------------------------*/

/**
 * Callback chamado quando um pacote UDP é recebido.
 * Calcula a latência e imprime métricas.
 */
static void udp_rx_callback(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port, const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port, const uint8_t *data, uint16_t datalen) {
    if (datalen != sizeof(uint64_t)) {
        printf("Received data of unexpected size.\n");
        return;
    }

    uint64_t received_timestamp_ticks = ((uint64_t)data[3] << 24) |
                                        ((uint64_t)data[2] << 16) |
                                        ((uint64_t)data[1] << 8)  |
                                        ((uint64_t)data[0]);

    uint64_t received_timestamp_ms = (received_timestamp_ticks * 1000) / CLOCK_SECOND;
    uint64_t current_time_ticks = tsch_get_network_uptime_ticks();
    uint64_t current_time_ms = (current_time_ticks * 1000) / CLOCK_SECOND;
    uint64_t latency_ticks = current_time_ticks - received_timestamp_ticks;
    uint64_t latency_ms = (latency_ticks * 1000) / CLOCK_SECOND;

    printf("metric:Received_timestamp_ms\t%lu\n", received_timestamp_ms);
    printf("metric:Current_time_ms\t%lu\n", current_time_ms);
    printf("metric:Latency_ms\t%lu\n", latency_ms);

    setLatency(&latency_ms);
}

/*------------------------Processo: Nó RPL------------------------*/

/**
 * Processo principal do nó RPL.
 * Gerencia a comunicação UDP e o envio de métricas.
 */
PROCESS_THREAD(node_process, ev, data) {
    static struct etimer periodic_timer;
    uip_ipaddr_t dest_ipaddr;

    PROCESS_BEGIN();

    // Inicializa a conexão UDP
    simple_udp_register(&client_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);
    NETSTACK_MAC.on();

    // Configura o temporizador para envio periódico
    etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        if (NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
            uint64_t network_uptime = tsch_get_network_uptime_ticks();
            simple_udp_sendto(&client_conn, &network_uptime, sizeof(network_uptime), &dest_ipaddr);

            LOG_INFO("sent_network_uptime_timestamp %lu to ", network_uptime);
            LOG_INFO_6ADDR(&dest_ipaddr);
            LOG_INFO_("\n");
        } else {
            LOG_INFO("Not reachable yet\n");
        }

        // Reinicia o temporizador com um intervalo aleatório
        etimer_set(&periodic_timer, SEND_INTERVAL - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
    }

    PROCESS_END();
}

/*------------------------Processo: Servidor Web------------------------*/

/**
 * Gera rotas para o servidor web.
 * Retorna dados de sensores e métricas de energia em formato JSON.
 */
static PT_THREAD(generate_routes(struct httpd_state *s)) {
    static char buff[256]; // Buffer para armazenar o JSON

    PSOCK_BEGIN(&s->sout);

    // Coleta métricas de energia
    energest_flush();
    calculate_cpu_energy();

    // Gera dados de sensores simulados
    int temperature = 15 + rand() % 25;
    int humidity = 80 + rand() % 10;

    uint64_t energest_total_time = ENERGEST_GET_TOTAL_TIME() + 0;

    uint32_t cpu = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));        
    uint32_t total_time = to_seconds(energest_total_time);  
    uint32_t radio_rx = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));
    uint32_t radio_tx = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));        
    uint32_t radio_total_time = to_seconds(energest_total_time - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN));
            
    // Formata os dados em JSON
    snprintf(buff, sizeof(buff),
             "{\"id\":%d,"
             "\"temp\":%d,"
             "\"hum\":%d,"
             "\"t_cpu\":%d,"
             "\"t_total_time\":%d,"
             "\"t_radio_rx\":%d,"
             "\"t_radio_tx\":%d,"
             "\"t_radio_total_time\":%d,"
             "\"cpu_energy_mJ\":%d,"
             "\"lpm_energy_mJ\":%d,"
             "\"radio_tx_energy_mJ\":%d,"
             "\"radio_rx_energy_mJ\":%d,"
             "\"latency\":%lu}",
             node_id, 
             temperature, 
             humidity, 
             cpu, 
             total_time, 
             radio_rx, 
             radio_tx, 
             radio_total_time, 
             cpu_energy_mJ, 
             lpm_energy_mJ, 
             radio_tx_energy_mJ, 
             radio_rx_energy_mJ, 
             getLatency());

    // Envia o JSON para o cliente
    SEND_STRING(&s->sout, buff);

    PSOCK_END(&s->sout);
}

/*------------------------Processo: Monitoramento de Energia------------------------*/

/**
 * Processo que monitora e imprime métricas de energia.
 */
PROCESS_THREAD(energest_example_process, ev, data) {
    static struct etimer periodic_timer;

    PROCESS_BEGIN();

    // Configura o temporizador para verificação periódica
    etimer_set(&periodic_timer, CLOCK_SECOND * 10);

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        etimer_reset(&periodic_timer);

        // Calcula e imprime as métricas de energia
        calculate_cpu_energy();
        printf("metric:energest_cpu_mJ\t%d\n", cpu_energy_mJ);
        printf("metric:lpm_energy_mJ\t%d\n", lpm_energy_mJ);
        printf("metric:radio_tx_energy_mJ\t%d\n", radio_tx_energy_mJ);
        printf("metric:radio_rx_energy_mJ\t%d\n", radio_rx_energy_mJ);
    }

    PROCESS_END();
}


/*------------------------Método exigido pelo servidor Web----------------------------*/

httpd_simple_script_t httpd_simple_get_script(const char *name){
  return generate_routes;
}

PROCESS_THREAD(webserver_nogui_process, ev, data){    

    PROCESS_BEGIN();

    httpd_init(); // Iniciando o servidor Web
    random_init(0);

    while(1) {	
        
        PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
        httpd_appcall(data);
    }    

    PROCESS_END();
}