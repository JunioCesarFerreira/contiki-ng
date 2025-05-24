/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#ifndef WEBSERVER_CONF_CFS_CONNS
#define WEBSERVER_CONF_CFS_CONNS 2
#endif

#ifndef BORDER_ROUTER_CONF_WEBSERVER
#define BORDER_ROUTER_CONF_WEBSERVER 1
#endif

#if BORDER_ROUTER_CONF_WEBSERVER
#define UIP_CONF_TCP 1
#endif

/*The Energest module*/
#define ENERGEST_CONF_ON 1

/*Platform configuration for Energest */
#define ENERGEST_CONF_CURRENT_TIME clock_time
#define ENERGEST_CONF_TIME_T clock_time_t
#define ENERGEST_CONF_SECOND CLOCK_SECOND


/*The Energest module 2*/
//#define BUILD_WITH_SIMPLE_ENERGEST 1

#endif /* PROJECT_CONF_H_ */


//------------------------------------------------------------------------------------------------------
//------------------------ Configure TSCH --------------------------------------------------------------
//------------------------------------------------------------------------------------------------------

#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

// Enable TCP 
#define UIP_CONF_TCP 1

// IEEE802.15.4 PANID 
#define IEEE802154_CONF_PANID 0x81a5

// Do not start TSCH at init, wait for NETSTACK_MAC.on() 
#define TSCH_CONF_AUTOSTART 0

// 6TiSCH minimal schedule length.
// Larger values result in less frequent active slots: reduces capacity and saves energy. 
// #define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 3

//------------------------------------------------------------------------------------------------------
//------------------------ Other system configuration --------------------------------------------------
//------------------------------------------------------------------------------------------------------

// Logging 
#define LOG_CONF_LEVEL_RPL LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_IPV6 LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_6LOWPAN LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_MAC LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_FRAMER LOG_LEVEL_INFO
#define TSCH_LOG_CONF_PER_SLOT 0

//------------------------------------------------------------------------------------------------------
//------------------------ RPL STUFF -------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------

#ifndef RPL_CONF_STATS
#define RPL_CONF_STATS 1
#endif

// Reduce the EB period in order to update the network nodes with more agility 
#define TSCH_CONF_EB_PERIOD (60 * CLOCK_SECOND)
#define TSCH_CONF_MAX_EB_PERIOD (60 * CLOCK_SECOND)
#endif // __PROJECT_CONF_H__ 

//------------------------------------------------------------------------------------------------------
