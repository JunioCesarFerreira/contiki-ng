/*
 * Copyright (c) 201, RISE SICS
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

/*------------------------Latency Server-------------------------------------------------------------------*/

#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/mac/tsch/tsch.h"
#include "lib/random.h"
#include "sys/node-id.h"
#include "net/ipv6/uiplib.h"

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678 

/*------------------------Latency Server-------------------------------------------------------------------*/

static struct simple_udp_connection server_conn; //client_conn, server_conn;

/*------------------------Declare and auto-start this file's process------------------------------------*/

PROCESS(contiki_ng_br, "Contiki-NG Border Router");

/*Latency Server*/
PROCESS(node_process, "RPL Server");

AUTOSTART_PROCESSES(&contiki_ng_br, &node_process);


/*------------------------Latency Server-------------------------------------------------------------------*/

static void udp_rx_callback(struct simple_udp_connection *c,
	 const uip_ipaddr_t *sender_addr,
	 uint16_t sender_port,
	 const uip_ipaddr_t *receiver_addr,
	 uint16_t receiver_port,
	 const uint64_t *data,
	 uint16_t datalen){		
	
	 // Copied this from examples/6tisch/timesync-demo/node.c 
	 uint64_t local_time_clock_ticks = tsch_get_network_uptime_ticks();
	 uint64_t remote_time_clock_ticks;	
	 unsigned long long final_time; 	
	 
	 
	 //if(datalen >= sizeof(remote_time_clock_ticks )) {
     if(datalen >= sizeof(remote_time_clock_ticks ) && sizeof(remote_time_clock_ticks) > 0 && data != NULL) {
		memcpy(&remote_time_clock_ticks, data, sizeof(remote_time_clock_ticks));		
		final_time = (unsigned long)(local_time_clock_ticks - remote_time_clock_ticks); 			 		
		//printf("metric:latency_router_is\t%lu\n", final_time);

		printf("metric:local_time_clock_ticks\t%" PRIu64, local_time_clock_ticks); 
		printf("\n");
		printf("metric:remote_time_clock_ticks\t%" PRIu64, remote_time_clock_ticks);
		printf("\n");
		printf("metric:latency_router_for_sensor_is\t%" PRIu64, final_time);
		printf("\n");
		printf("metric:clock_ticks_for ");		
		char addr_str[UIPLIB_IPV6_MAX_STR_LEN];
		uiplib_ipaddr_snprint(addr_str, sizeof(addr_str), sender_addr);
		printf("%s\n", addr_str);
		printf("\n");
		
		//Sensor Latency begin					
		//printf("Pedido Recebido '%lu' de ", *data); (linha de teste)
		//printf("Pedido Recebido '%.*s' de ", datalen, (char *) data);  (Esse é o usado!)
		//LOG_INFO_6ADDR(sender_addr);
		//printf("\n");
			// /#if WITH_SERVER_REPLY //Observação! Quando descomento o "if" a Latência não exbida NADA no cooja
				// send back the same string to the client as an echo reply 
				//printf("Enviando resposta para o sensor.\n");
				simple_udp_sendto(&server_conn, &final_time, sizeof (final_time), sender_addr);
			// /#endif
		//Sensor Latency end */

	 }			 
}

PROCESS_THREAD(node_process, ev, data)
{
	 PROCESS_BEGIN();
		 // Makes this node the TSCH coordinator (and RPL root)
		 NETSTACK_ROUTING.root_start();
		 
		 // Copied following lines from examples/rpl-udp/udp-server.c and udp-node.c 
		 // Crie um socket de servidor UDP para ouvir
		 simple_udp_register(&server_conn, UDP_SERVER_PORT, NULL, UDP_CLIENT_PORT, udp_rx_callback);
		
		 // Creates udp client socket for sending (not used at the moment)
		 //simple_udp_register(&client_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, NULL);
				 
		 // Initialize TSCH process (which is the MAC protocoll being used, see Makefile)		 
		 NETSTACK_MAC.on();
	 
	 PROCESS_END();
}

/*------------------------Contiki-NG Border Router------------------------------------------------------*/

PROCESS_THREAD(contiki_ng_br, ev, data)
{
  PROCESS_BEGIN();

	#if BORDER_ROUTER_CONF_WEBSERVER
	  PROCESS_NAME(webserver_nogui_process);
	  process_start(&webserver_nogui_process, NULL);
	#endif /* BORDER_ROUTER_CONF_WEBSERVER */

	  printf("Contiki-NG Border Router started\n");

  PROCESS_END();
}
/*------------------------------------------------------------------------------------------------------*/
