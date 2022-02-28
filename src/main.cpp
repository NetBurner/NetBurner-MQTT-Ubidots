#include <init.h>
#include <constants.h>
#include <nbrtos.h>
#include <utils.h>
#include <string.h>
#include <fdprintf.h>
#include <system.h>
#include <nettypes.h>
#include <netinterface.h>

#include <MQTTClient.h>
#include <NBMQTTSocket.h>
#include <NBMQTTTLSSocket.h>
#include <NBMQTTCountdown.h>

#include <ubidots.h>

/*---------------------  Globals ---------------------*/
const char *AppName = "NetBurner-Ubidots-MQTT";

/**
 * @brief Ubidots Token. Used to authenticate MQTT with the broker.
 *
 */
#define UBIDOTS_TOKEN "Add you token here"

/**
 * @brief Ubidots device name. Used to create MQTT topics.
 *
 */
#define UBIDOTS_DEVICE_NAME "Add the device name here"

/**
 * @brief Global object to create Ubidots instance.
 *
 */
/*               Token,         Device,Name,         SSL, Console log */
Ubidots ubidots(UBIDOTS_TOKEN, UBIDOTS_DEVICE_NAME, false, true); // Init ubidots object
/*------------------------------------------------*/

/*---------------------  Callbacks ---------------------*/
/**
 * @brief Callback to handle MQTT message received.
 *
 * @param md
 */
void ubidotsSubscribeHandler(MQTT::MessageData &md)
{
  static int arrivedcount = 0;

  MQTT::Message &message = md.message;
  MQTTString &topic = md.topicName;
  reinterpret_cast<char *>(message.payload)[message.payloadlen] = 0;

  iprintf("Message %d arrived: qos %d, retained %d, dup %d, packetid %d\r\nPayload: %s\r\n",
          ++arrivedcount, message.qos, message.retained, message.dup, message.id, (char *)message.payload);

  iprintf("Topic %.*s\r\n", topic.lenstring.len, (char *)topic.lenstring.data);
}

/**
 * @brief Callback to handle MQTT connection
 *
 * @param pvParameter
 */
void cb_connected(void *pvParameter)
{
  iprintf("  ---> Ubidots broker connected <--- \n");

  // NOTE: Subscribtions MUST be performed after sucessfull conection
  ubidots.subscribe("leds", ubidotsSubscribeHandler);
}

/**
 * @brief Callback to handle MQTT error
 *
 * @param pvParameter Error estatus
 */
void cb_error(void *pvParameter)
{
  ubidots_state_t *state = (ubidots_state_t *)pvParameter;
  iprintf("  ---> Ubidots broker error[%d] <--- \n", state);
}

/*------------------------------------------------*/

void UserMain(void *pd)
{
  init();
  WaitForActiveNetwork(TICKS_PER_SECOND * 10); // Wait for DHCP address
  iprintf("Application started\n");

  ubidots.registerCallback(UBIDOTS_EVENT_CONNECTED, cb_connected); // Register connected callback
  ubidots.registerCallback(UBIDOTS_EVENT_ERROR, cb_error);         // Register error callback

  float demo = 0;

  while (1)
  {
    if (ubidots.isConnected())
    { // Is Ubidots MQTT connected?
      demo += 2;
      ubidots.publish("demo", demo); // Send data to Ubidots
      ubidots.keepAlive();           // Issue keep alive and receive data
    }
    else
    {
      ubidots.connect(); // Try to connect
    }

    OSTimeDly(TICKS_PER_SECOND * 2);
  }
}
