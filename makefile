NAME    = NetburnerBase
PLATFORM = MODM7AE70

CPP_SRC     += \
            src/main.cpp 

CREATEDTARGS += src/htmldata.cpp
CPP_SRC += src/htmldata.cpp
src/htmldata.cpp : $(wildcard html/*.*)
	comphtml html -osrc/htmldata.cpp


# Include mqtt-paho library 
NBINCLUDE += \
						-I src/mqtt-paho  
						
# Source file smqtt-paho library
C_SRC	+= \
		src/mqtt-paho/MQTTConnectClient.c \
		src/mqtt-paho/MQTTConnectServer.c \
		src/mqtt-paho/MQTTDeserializePublish.c \
		src/mqtt-paho/MQTTFormat.c \
		src/mqtt-paho/MQTTPacket.c \
		src/mqtt-paho/MQTTSerializePublish.c \
		src/mqtt-paho/MQTTSubscribeClient.c \
		src/mqtt-paho/MQTTSubscribeServer.c \
		src/mqtt-paho/MQTTUnsubscribeClient.c \
		src/mqtt-paho/MQTTUnsubscribeServer.c \


# Include and Source file ubidots
NBINCLUDE += \
		-I src/ubidots 

CPP_SRC		+= \
        src/ubidots/ubidots.cpp \

include $(NNDK_ROOT)/make/boilerplate.mk
