/**
 * @file ubidots.h
 *
 * @brief Connection using MQTT to Ubidots and NetBurner
 *
 * @author Jordan Garcia
 *
 */

#ifndef HTTP_CLIENT_APP_H_
#define HTTP_CLIENT_APP_H_

#include <nbrtos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <MQTTClient.h>
#include <NBMQTTSocket.h>
#include <NBMQTTTLSSocket.h>
#include <NBMQTTCountdown.h>

/*---------------------  Definitions ---------------------*/
#define UBIDOTS_MQTT_HOST "industrial.api.ubidots.com" /*!< Ubidots MQTT host */
#define UBIDOTS_MQTT_PASS ""                           /*!< Default password for MQTT connection */
#define UBIDOTS_MQTT_PORT 1883                         /*!< Ubidots MQTT port */
#define UBIDOTS_SSL_PORT 8883                          /*!< Ubidots SSL MQTT port */
#define UBIDOTS_MSG_MAX_LEN 1000                       /*!< Max Len for MQTT message */
#define UBIDOTS_TOPIC_MAX_LEN 100                      /*!< Max Len for MQTT topic */
#define UBIDOTS_BROKER_PATH "/v1.6/devices/"           /*!< MQTT broker path */
#define UBIDOTS_KEEP_ALIVE_MS 60                       /*!< Keep alive for MQTT config */
#define UBIDOTS_DEFAULT_CLIENT_ID "NETBURNER"          /*!< Default name for MQTT client ID */
#define UBIDOTS_CONNECT_RETRIES 3                      /*!< Retries to connect */
#define UBIDOTS_SUBSCRIBE_MAX_TOPICS 20                /*!< Max subscribe topics */

/**
 * @brief Ubidots events
 */
typedef enum
{
  UBIDOTS_EVENT_CONNECTED,    /*!< Event of mqtt succesfull conection */
  UBIDOTS_EVENT_DISCONNECTED, /*!< Event of mqtt disconection */
  UBIDOTS_EVENT_SUBSCRIBED,   /*!< Event of mqtt topic subcribed */
  UBIDOTS_EVENT_PUBLISHED,    /*!< Event of mqtt message published */
  UBIDOTS_EVENT_ERROR,        /*!< Event of mqtt error */
  UBIDOTS_MESSAGE_CODE_COUNT  /*!< End of events */
} ubidots_events_t;

/**
 * @brief Ubidots status
 *
 */
typedef enum
{
  UBIDOTS_NONE,                /*!< No estatus  */
  UBIDOTS_OK,                  /*!< Everything ok  */
  UBIDOTS_ALREADY_CONNECTED,   /*!< Socket/MQTT already connected */
  UBIDOTS_SOCKET_NO_AVAILABLE, /*!< TCP/SSL socker not available  */
  UBIDOTS_NO_CONNECTED,        /*!< Socket no connected */
  UBIDOTS_SOCKET_ERROR,        /*!< Socket error */
  UBIDOTS_MQTT_SOCKET_ERROR,   /*!< MQTT socket error */
  UBIDOTS_SUBSCRIBE_ERROR,     /*!< Error during MQTT subscribe */
  UBIDOTS_PUBLISH_ERROR,       /*!< Error during MQTT publish */
  UBIDOTS_NOT_AUTHORIZED       /*!< Error during MQTT authenticate */
} ubidots_state_t;

/**
 * @brief Handler to subscribe message format.
 *
 */
typedef void (*subscribe_handler_t)(MQTT::MessageData &md);

/*------------------------------------------------*/

/*---------------------  Classes ---------------------*/
class Ubidots
{
private:
  /*---------------------  Attributes ---------------------*/
  bool log;                                                                      /*!< Serial log */
  bool ssl;                                                                      /*!< Enable SSL */
  bool connected;                                                                /*!< MQTT connected flag */
  uint8_t retries;                                                               /*!< Number of connection retries */
  const char *token;                                                             /*!< Platform token */
  const char *device;                                                            /*!< Device name */
  char baseTopic[UBIDOTS_TOPIC_MAX_LEN];                                         /*!< MQTT base topic */
  uint8_t subTopicsUsed;                                                         /*!< Number of subscriptions */
  char subTopics[UBIDOTS_SUBSCRIBE_MAX_TOPICS][UBIDOTS_TOPIC_MAX_LEN];           /*!< MQTT base topic */
  NBMQTTSocket mqttSocket;                                                       /*!< MQTT Socket TCP */
  NBMQTTTLSSocket mqttSSLSocket;                                                 /*!< MQTT Socket SSL */
  MQTT::Client<NBMQTTSocket, NBMQTTCountdown, UBIDOTS_MSG_MAX_LEN> client;       /*!< MQTT TCP object  */
  MQTT::Client<NBMQTTTLSSocket, NBMQTTCountdown, UBIDOTS_MSG_MAX_LEN> clientSSL; /*!< MQTT SSL object  */
  MQTTPacket_connectData mqttOptions;                                            /*!< MQTT options object */
  void (*cbPtrArr[UBIDOTS_MESSAGE_CODE_COUNT])(void *);                          /*!< Array of function pointers for callbacks  */
  /*------------------------------------------------*/

  /*---------------------  Methods ---------------------*/
  /**
   * @brief Prints to the UART terminal
   *
   * @param format String format
   * @param ... Elipsis
   */
  void consoleLog(const char *format, ...);
  /*------------------------------------------------*/

public:
  /*---------------------  Constructor/Destructor ---------------------*/
  /**
   * @brief Construct a new Ubidots object
   *
   * @param token Ubidots token to authenticate.
   * @param device_name Ubidots device name.
   * @param ssl Choose SSL or TCP. True for SSL.
   * @param log True for print to the console.
   */
  Ubidots(const char *token = nullptr, const char *device_name = nullptr, bool ssl = false, bool log = true);

  /**
   * @brief Destroy the Ubidots object
   *
   */
  ~Ubidots();
  /*------------------------------------------------*/

  /*--------------------- Methods  ---------------------*/
  /**
   * @brief Connect to Ubidots MQTT.
   *
   * @retval true All set.
   * @retval false Error
   */
  bool connect();

  /**
   * @brief Ubidots MQTT Subcribe
   *
   * @param variable Variable name to subscribe
   * @param handler Callback when receive a message
   * @retval true Subscribed succesfully
   * @retval false Error
   */
  bool subscribe(const char *variable = nullptr, subscribe_handler_t handler = nullptr);

  /**
   * @brief Ubidots MQTT Publish
   *
   * @param variable Variable name to subscribe
   * @param value Value of variable
   * @retval true Published succesfully
   * @retval false Error
   */
  bool publish(const char *variable, float value);

  /**
   * @brief MQTT keep alive and receive data
   *
   * @retval true All good
   * @retval false Error
   */
  bool keepAlive();

  /**
   * @brief Register a callback to an event
   *
   * @param event Event to register
   * @param func_ptr Callback
   */
  void registerCallback(ubidots_events_t event, void (*func_ptr)(void *));

  /**
   * @brief Get MQTT is connected status
   *
   * @retval true Connected
   * @retval false Disconnected
   */
  bool isConnected() const;

  /**
   * @brief Get the Device Name object
   *
   * @retval const char* Device name
   */
  const char *getDeviceName() const;
  /*------------------------------------------------*/
};

/*------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HTTP_CLIENT_APP_H_ */