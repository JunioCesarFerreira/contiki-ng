/**
 * @file main.c
 * @brief Código para medição de consumo de energia, latência de rede e exposição de métricas via servidor web.
 *
 * Este código é destinado para o simulador Cooja para Sky MSP430, utilizando o Contiki OS.
 * Implementa:
 *  - Medição do consumo de energia através do módulo energest.
 *  - Medição de latência de rede através de troca de timestamps via UDP.
 *  - Servidor web simples que retorna as métricas em formato JSON.
 */

 #include "contiki.h"
 #include <string.h>
 #include "net/routing/routing.h"
 #include "net/ipv6/uip-ds6-nbr.h"
 #include "net/ipv6/uip-ds6-route.h"
 #include "net/ipv6/uip-sr.h"
 #include <time.h>
 #include <setjmp.h>
 #include <signal.h>
 
 #include "net/netstack.h"
 #include "net/ipv6/simple-udp.h"
 #include "net/mac/tsch/tsch.h"
 #include "lib/random.h"
 #include "sys/node-id.h"
 #include "sys/log.h"
 
 #include <stdio.h>
 #include "sys/energest.h"
 #include "webserver/httpd-simple.h"
 
 /* Configuração de log */
 #define LOG_MODULE "App"
 #define LOG_LEVEL LOG_LEVEL_INFO
 
 /* Portas UDP e intervalo de envio */
 #define UDP_CLIENT_PORT 8765
 #define UDP_SERVER_PORT 5678
 #define SEND_INTERVAL (60 * CLOCK_SECOND)
 
 /* Características de potência do Sky mote (valores em mW ou mJ conforme a conversão desejada) */
 #define CPU_POWER_ACTIVE 1.8    /**< Potência da CPU ativa */
 #define LPM_POWER        0.0545 /**< Potência em modo LPM */
 #define RADIO_TX_POWER   17.4   /**< Potência em transmissão pelo rádio */
 #define RADIO_RX_POWER   19.7   /**< Potência em recepção pelo rádio */
 
 /* Variáveis globais para energia consumida (em mJ) */
 int cpu_energy_mJ = 0, lpm_energy_mJ = 0, radio_tx_energy_mJ = 0, radio_rx_energy_mJ = 0;
 
 /* Conexão UDP */
 static struct simple_udp_connection client_conn;
 
 /* Variável para armazenar a latência medida (em ticks) */
 static uint64_t latency_sensor = 0;
 
 /**
  * @brief Define o valor da latência medido.
  *
  * @param latency_in Ponteiro para o valor de latência (em ticks).
  */
 void setLatency(uint64_t *latency_in) {
   if(latency_in != NULL) {
     latency_sensor = *latency_in;
   }
 }
 
 /**
  * @brief Retorna a latência medida.
  *
  * @return Latência (em ticks).
  */
 uint64_t getLatency() {
   return latency_sensor;
 }
 
 /*===========================================================================
  * Declaração dos Processos do Contiki
  *===========================================================================*/
 PROCESS(webserver_nogui_process, "Web Server (AWS)");
 PROCESS(energest_example_process, "Energest Example Process");
 PROCESS(node_process, "RPL Node Process");
 
 AUTOSTART_PROCESSES(&webserver_nogui_process, &energest_example_process, &node_process);
 
 /*===========================================================================
  * Função: udp_rx_callback
  * ---------------------------------------------------------------------------
  * Callback de recepção UDP que realiza:
  *   - Extração do timestamp enviado.
  *   - Conversão dos tempos (ticks para milissegundos).
  *   - Cálculo da latência (diferença entre o tempo atual e o timestamp recebido).
  *   - Impressão dos valores medidos.
  *===========================================================================*/
 static void
 udp_rx_callback(struct simple_udp_connection *c,
                 const uip_ipaddr_t *sender_addr,
                 uint16_t sender_port,
                 const uip_ipaddr_t *receiver_addr,
                 uint16_t receiver_port,
                 const uint64_t *data,
                 uint16_t datalen) {
 
   if(datalen != sizeof(uint64_t)) {
     printf("Received data of unexpected size.\n");
     return;
   }
   
   /* Extrai o timestamp recebido */
   uint64_t received_timestamp_ticks = *data;
   uint64_t received_timestamp_ms = (received_timestamp_ticks * 1000) / CLOCK_SECOND;
   printf("Received timestamp (ticks): %lu\n", received_timestamp_ticks);
   printf("metric:Received_timestamp_ms\t%lu\n", received_timestamp_ms);
   
   /* Obtém o timestamp atual */
   uint64_t current_time_ticks = tsch_get_network_uptime_ticks();
   uint64_t current_time_ms = (current_time_ticks * 1000) / CLOCK_SECOND;
   printf("Current time (ticks): %lu\n", current_time_ticks);
   printf("metric:Current_time_ms\t%lu\n", current_time_ms);
   
   /* Calcula a latência (em ticks e convertido para milissegundos) */
   uint64_t latency_ticks = current_time_ticks - received_timestamp_ticks;
   uint64_t latency_ms = (latency_ticks * 1000) / CLOCK_SECOND;
   printf("metric:Latency_ms\t%lu\n", latency_ms);
   
   /* (Opcional) Atualiza estatísticas: contagem de pacotes recebidos */
   static int cont = 0;
   static int soma = 0;
   if(*data >= 0) {
     soma += *data;
     cont++;
   }
   printf("metric:total_mens(latency)_send_sensor_for_router_is\t%u\n", cont);
 }
 
 /*===========================================================================
  * Processo: node_process
  * ---------------------------------------------------------------------------
  * Este processo realiza o envio periódico de um timestamp de uptime da rede
  * para o nó raiz (DAG root) utilizando o protocolo UDP.
  *===========================================================================*/
 PROCESS_THREAD(node_process, ev, data)
 {
   static struct etimer periodic_timer;
   uip_ipaddr_t dest_ipaddr;
   
   PROCESS_BEGIN();
   
   /* Registra a conexão UDP com o callback de recepção */
   simple_udp_register(&client_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);
   NETSTACK_MAC.on();
   
   /* Inicializa o timer com atraso aleatório */
   etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
   
   while(1) {
     PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
     
     /* Se o nó raiz for alcançável, envia o timestamp atual */
     if(NETSTACK_ROUTING.node_is_reachable() &&
        NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
       
       uint64_t network_uptime = tsch_get_network_uptime_ticks();
       simple_udp_sendto(&client_conn, &network_uptime, sizeof(network_uptime), &dest_ipaddr);
       LOG_INFO("sent_network_uptime_timestamp %lu to ", (unsigned long)network_uptime);
       LOG_INFO_6ADDR(&dest_ipaddr);
       LOG_INFO_("\n");
     } else {
       LOG_INFO("Not reachable yet\n");
     }
     
     /* Reinicia o timer com jitter */
     etimer_set(&periodic_timer,
                SEND_INTERVAL - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
   }
   
   PROCESS_END();
 }
 
 /*===========================================================================
  * Função: to_seconds
  * ---------------------------------------------------------------------------
  * Converte um tempo em ticks para segundos.
  *===========================================================================*/
 static inline unsigned long
 to_seconds(uint64_t time) {
   return (unsigned long)(time / ENERGEST_SECOND);
 }
 
 /*===========================================================================
  * Função: calculate_cpu_energy
  * ---------------------------------------------------------------------------
  * Calcula a energia consumida (em mJ) pela CPU, modos LPM e pelo rádio em
  * transmissão e recepção.
  *
  * Para isso:
  *   - Obtém os tempos de atividade através do módulo energest.
  *   - Converte os tempos para segundos.
  *   - Multiplica os tempos pelas respectivas potências.
  *===========================================================================*/
 void calculate_cpu_energy() {
   unsigned long cpu_time_seconds, lpm_time_seconds, radio_tx_time_seconds, radio_rx_time_seconds;
   
   cpu_time_seconds   = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));
   lpm_time_seconds   = to_seconds(energest_type_time(ENERGEST_TYPE_LPM));
   radio_tx_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));
   radio_rx_time_seconds = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));
   
   cpu_energy_mJ      = CPU_POWER_ACTIVE * cpu_time_seconds;
   lpm_energy_mJ      = LPM_POWER * lpm_time_seconds;
   radio_tx_energy_mJ = RADIO_TX_POWER * radio_tx_time_seconds;
   radio_rx_energy_mJ = RADIO_RX_POWER * radio_rx_time_seconds;
 }
 
 /*===========================================================================
  * Função: generate_routes
  * ---------------------------------------------------------------------------
  * Gera um JSON com as métricas do nó, incluindo:
  *   - Identificação do nó.
  *   - Temperatura e umidade (valores aleatórios para simulação).
  *   - Tempos de atividade (CPU, rádio, etc).
  *   - Energia consumida.
  *   - Latência medida.
  *
  * Esta função é utilizada pelo servidor web para responder às requisições HTTP.
  *===========================================================================*/
 static PT_THREAD(generate_routes(struct httpd_state *s)) {
   char buff[256];  /* Buffer para armazenar o JSON */
   
   PSOCK_BEGIN(&s->sout);
   
   energest_flush();
   
   /* Coleta dos tempos de atividade */
   unsigned long cpu         = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));
   unsigned long total_time  = to_seconds(ENERGEST_GET_TOTAL_TIME());
   unsigned long radio_rx    = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));
   unsigned long radio_tx    = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));
   unsigned long radio_total_time =
     to_seconds(ENERGEST_GET_TOTAL_TIME() -
                energest_type_time(ENERGEST_TYPE_TRANSMIT) -
                energest_type_time(ENERGEST_TYPE_LISTEN));
   
   /* Valores aleatórios para temperatura e umidade (simulação) */
   int temperature = 15 + rand() % 25;
   int humidity    = 80 + rand() % 10;
   
   /* Formata o JSON com as métricas coletadas */
   snprintf(buff, sizeof(buff),
            "{\"id\":%d,\"temp\":%u,\"hum\":%u,"
            "\"t_cpu\":%lu,\"t_total_time\":%lu,\"t_radio_rx\":%lu,"
            "\"t_radio_tx\":%lu,\"t_radio_total_time\":%lu,"
            "\"cpu_energy_mJ\":%lu,\"lpm_energy_mJ\":%lu,\"radio_tx_energy_mJ\":%lu,"
            "\"radio_rx_energy_mJ\":%lu,\"latency\":%lu}",
            node_id, temperature, humidity, cpu, total_time, radio_rx, radio_tx,
            radio_total_time, (unsigned long)cpu_energy_mJ, (unsigned long)lpm_energy_mJ,
            (unsigned long)radio_tx_energy_mJ, (unsigned long)radio_rx_energy_mJ, getLatency());
   
   printf("\nsend json to requester\n");
   printf("JSON: %s\n", buff);
   
   SEND_STRING(&s->sout, buff);
   
   PSOCK_END(&s->sout);
 }
 
 /*===========================================================================
  * Função: httpd_simple_get_script
  * ---------------------------------------------------------------------------
  * Retorna o script HTTP que será executado para responder às requisições.
  *===========================================================================*/
 httpd_simple_script_t
 httpd_simple_get_script(const char *name) {
   return generate_routes;
 }
 
 /*===========================================================================
  * Processo: webserver_nogui_process
  * ---------------------------------------------------------------------------
  * Inicializa e mantém o servidor web simples, processando os eventos TCP/IP.
  *===========================================================================*/
 PROCESS_THREAD(webserver_nogui_process, ev, data) {
   PROCESS_BEGIN();
   
   httpd_init();  /* Inicializa o servidor web */
   random_init(0);
   
   while(1) {
     PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
     httpd_appcall(data);
   }
   
   PROCESS_END();
 }
 
 /*===========================================================================
  * Processo: energest_example_process
  * ---------------------------------------------------------------------------
  * Processo responsável por atualizar periodicamente (a cada 10 segundos)
  * as métricas de energia, calculá-las e imprimi-las.
  *===========================================================================*/
 PROCESS_THREAD(energest_example_process, ev, data) {
   static struct etimer periodic_timer;
   
   PROCESS_BEGIN();
   
   etimer_set(&periodic_timer, CLOCK_SECOND * 10);
   
   while(1) {
     PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
     etimer_reset(&periodic_timer);
     
     energest_flush();
     
     /* Calcula a energia consumida em cada módulo */
     calculate_cpu_energy();
     
     /* Impressão das métricas de energia */
     printf("metric:energest_cpu_mJ\t%4d\n", cpu_energy_mJ);
     printf("metric:lpm_energy_mJ\t%4d\n", lpm_energy_mJ);
     printf("metric:radio_tx_energy_mJ\t%4d\n", radio_tx_energy_mJ);
     printf("metric:radio_rx_energy_mJ\t%4d\n", radio_rx_energy_mJ);
     
     /* Impressão dos tempos de atividade */
     printf("metric:energest_cpu\t%4lu\n", to_seconds(energest_type_time(ENERGEST_TYPE_CPU)));
     printf("metric:energest_lpm\t%4lu\n", to_seconds(energest_type_time(ENERGEST_TYPE_LPM)));
     printf("metric:energest_deep_lpm\t%4lu\n", to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)));
     printf("metric:energest_total_time\t%4lu\n", to_seconds(ENERGEST_GET_TOTAL_TIME()));
     
     /* Impressão dos tempos de atividade do rádio */
     printf("metric:energest_radio_listen\t%4lu\n", to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)));
     printf("metric:energest_radio_transmit\t%4lu\n", to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)));
     printf("metric:energest_radio_total_time\t%4lu\n",
            to_seconds(ENERGEST_GET_TOTAL_TIME() -
                       energest_type_time(ENERGEST_TYPE_TRANSMIT) -
                       energest_type_time(ENERGEST_TYPE_LISTEN)));
   }
   
   PROCESS_END();
 }
 