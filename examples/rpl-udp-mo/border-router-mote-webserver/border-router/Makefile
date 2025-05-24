CONTIKI_PROJECT = border-router
all: $(CONTIKI_PROJECT)
CONTIKI = ../../../..

# The BR is either native or embedded, and in the latter case must support SLIP
PLATFORMS_EXCLUDE = nrf52dk z1

# Include RPL BR module
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_SERVICES_DIR)/rpl-border-router

# Include webserver module
MODULES_REL += webserver

# Include optional target-specific module
include $(CONTIKI)/Makefile.identify-target
MODULES_REL += $(TARGET)

#To enable TSCH
MAKE_MAC = MAKE_MAC_TSCH
#MODULES += $(CONTIKI_NG_SERVICES_DIR)/shell

include $(CONTIKI)/Makefile.include

#To enable RPL CLASSIC
#MAKE_ROUTING=MAKE_ROUTING_RPL_CLASSIC
