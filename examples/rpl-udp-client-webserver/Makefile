CONTIKI_PROJECT = udp-client-monitor udp-server-monitor
all: $(CONTIKI_PROJECT)

CONTIKI=../..

#To enable TSCH
MAKE_MAC = MAKE_MAC_TSCH
include $(CONTIKI)/Makefile.dir-variables

MODULES_REL += webserver

include $(CONTIKI)/Makefile.include

CFLAGS += -Os
