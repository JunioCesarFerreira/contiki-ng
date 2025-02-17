#ifndef PROJECT_CONF_H
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
#endif /* PROJECT_CONF_H */
