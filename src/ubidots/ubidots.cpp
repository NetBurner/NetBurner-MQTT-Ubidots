/**
 * @file ubidots.cpp
 *
 * @brief Connection using MQTT to Ubidots and NetBurner
 *
 * @author Jordan Garcia
 *
 */

#include <nettypes.h>

#include <ubidots.h>

#define TMPT_SIMPLE_PUBLISH_VAR "{\"%s\": %.2f}"

/*---------------------  Globals ---------------------*/
static const char TAG[] = "UBIDOTS";
static const char UBIDOTS_STATES[][30] = {
    "none",
    "ok",
    "already_connected",
    "socket_no_available",
    "no_connected",
    "socket_error",
    "mqtt_socket_error",
    "no_link_active",
    "timer_error",
    "subscribe_error",
    "publish_error",
    "not_authorized"};

/*------------------------------------------------*/

/*---------------------  Prototipos Callbacks ---------------------*/
/*------------------------------------------------*/

/*---------------------  Prototipos funciones privadas ---------------------*/
static void printSocketErrors(int fd_print);
/*------------------------------------------------*/

/*---------------------  Callbacks ---------------------*/
/*------------------------------------------------*/

/*---------------------  Private fuctions ---------------------*/
static void printSocketErrors(int fd_print)
{
  iprintf("Conection failed with error %d, ", fd_print);

  switch (fd_print)
  {
  case TCP_ERR_TIMEOUT:
    iprintf(" TCP_ERR_TIMEOUT       \r\n");
    break;
  case TCP_ERR_NOCON:
    iprintf(" TCP_ERR_NOCON        \r\n");
    break;
  case TCP_ERR_CLOSING:
    iprintf(" TCP_ERR_CLOSING        \r\n");
    break;
  case TCP_ERR_NOSUCH_SOCKET:
    iprintf(" TCP_ERR_NOSUCH_SOCKET        \r\n");
    break;
  case TCP_ERR_NONE_AVAIL:
    iprintf(" TCP_ERR_NONE_AVAIL        \r\n");
    break;
  case TCP_ERR_CON_RESET:
    iprintf(" TCP_ERR_CON_RESET        \r\n");
    break;
  case TCP_ERR_CON_ABORT:
    iprintf(" TCP_ERR_CON_ABORT        \r\n");
    break;
  case SSL_ERROR_FAILED_NEGOTIATION:
    iprintf(" SSL_ERROR_FAILED_NEGOTIATION        \r\n");
    break;
  case SSL_ERROR_CERTIFICATE_UNKNOWN:
    iprintf(" SSL_ERROR_CERTIFICATE_UNKNOWN       \r\n");
    break;
  case SSL_ERROR_CERTIFICATE_NAME_FAILED:
    iprintf(" SSL_ERROR_CERTIFICATE_NAME_FAILED   \r\n");
    break;
  case SSL_ERROR_CERTIFICATE_VERIFY_FAILED:
    iprintf(" SSL_ERROR_CERTIFICATE_VERIFY_FAILED \r\n");
    break;

  default:
    iprintf(" Error desconocido %d \r\n", fd_print);
    break;
  }
}
/*------------------------------------------------*/

/*---------------------  Public fuctions ---------------------*/
/*------------------------------------------------*/

/*---------------------  Tareas ---------------------*/

/*------------------------------------------------*/

/*---------------------  Private Methods  ---------------------*/
void Ubidots::consoleLog(const char *format, ...)
{
  if (this->log)
  { // If able log
    va_list vl;
    va_start(vl, format);
    viprintf(format, vl);
    va_end(vl);
  }
}
/*------------------------------------------------*/

/*---------------------  Constructor/Destructor Methods  ---------------------*/
Ubidots::Ubidots(const char *token, const char *device_name, bool ssl, bool log) : client(this->mqttSocket), clientSSL(this->mqttSSLSocket)
{
  this->log = log;            // Init log
  this->ssl = ssl;            // Init ssl
  this->token = token;        // Init token
  this->connected = false;    // Init connected
  this->device = device_name; // Init device name
  this->subTopicsUsed = 0;    // Number de subscribe topics used

  memset(this->subTopics, 0, sizeof(this->subTopics)); // Initialize subs topics array

  // Init with null array of callbacks
  for (size_t i = 0; i < UBIDOTS_MESSAGE_CODE_COUNT; i++)
  {
    this->cbPtrArr[i] = nullptr;
  }

  // Init Ubidots MQTT configuration
  this->mqttOptions = MQTTPacket_connectData_initializer;
  this->mqttOptions.MQTTVersion = 3;
  this->mqttOptions.keepAliveInterval = UBIDOTS_KEEP_ALIVE_MS;
  this->mqttOptions.cleansession = 1;
  this->mqttOptions.username.cstring = (char *)token;
  this->mqttOptions.password.cstring = (char *)UBIDOTS_MQTT_PASS;

  // Init device name
  if (device_name)
  {
    this->mqttOptions.clientID.cstring = (char *)this->device;
  }
  else
  {
    this->mqttOptions.clientID.cstring = (char *)UBIDOTS_DEFAULT_CLIENT_ID;
  }

  // Create base topic
  memset(this->baseTopic, '\0', UBIDOTS_TOPIC_MAX_LEN);
  strcpy(this->baseTopic, UBIDOTS_BROKER_PATH);
  strcat(this->baseTopic, this->device);

  this->consoleLog("-- %s: APP iniciada --\r\n", "Ubidots");
}

Ubidots::~Ubidots() {}
/*------------------------------------------------*/

/*---------------------  Public Methods  ---------------------*/

bool Ubidots::connect()
{
  ubidots_state_t state = UBIDOTS_NONE; // Ubidots state

  if (!this->connected)
  {                       // If no connected before
    int stateSocket = -1; // TCP socket state
    int stateMQTT = -1;   // MQTT socket state

    stateSocket = (this->ssl) ? this->mqttSSLSocket.mysock : this->mqttSocket.mysock; // Get socket status

    if (stateSocket < 0)
    { // If TCP socket available

      // --- TCP socket connection logic --- //
      do
      {
        // Get socket connecting status
        stateSocket = (this->ssl) ? this->mqttSSLSocket.connect((char *)UBIDOTS_MQTT_HOST, UBIDOTS_SSL_PORT)
                                  : this->mqttSocket.connect((char *)UBIDOTS_MQTT_HOST, UBIDOTS_MQTT_PORT);

        this->retries++; // Add a retry

        if (stateSocket != 0)
        {                                                                                       // if error
          this->consoleLog("%s\r\n", "Error connecting socket, retrying within 5 seconds\r\n"); // print message
          if (this->log)
          {
            printSocketErrors(stateSocket); // Print socket error
          }
          OSTimeDly(TICKS_PER_SECOND * 5); // Block task for 5 seconds
        }

      } while (this->retries < UBIDOTS_CONNECT_RETRIES && stateSocket != 0);

      this->retries = 0; // Reset retries

      if (stateSocket != 0)
      {                               // if socket error
        state = UBIDOTS_SOCKET_ERROR; // set state to socket error
        if (this->cbPtrArr[UBIDOTS_EVENT_ERROR])
        {
          this->cbPtrArr[UBIDOTS_EVENT_ERROR]((void *)state); // Event error callback
        }
        return false; // return false
      }

      this->consoleLog("Ubidots socket %s connected successfully\r\n", (this->ssl) ? "SSL" : "TCP");

      // --- MQTT socket connection logic --- //
      bool mqttLoop = true;
      do
      {
        // Get MQTT connection status
        stateMQTT = (this->ssl) ? this->clientSSL.connect(this->mqttOptions)
                                : this->client.connect(this->mqttOptions);

        this->retries++; // Add a retry

        if (stateMQTT != 0)
        {                                                                                      // Broker connection error
          this->consoleLog("%s\r\n", "Error connecting to broker, retrying within 5 seconds"); // print message
          if (stateMQTT == 5)
          {                                 // If 5 (no authorized)
            state = UBIDOTS_NOT_AUTHORIZED; // Set state to no authorized
            mqttLoop = false;               // Break loop
          }
          else
          {
            state = UBIDOTS_MQTT_SOCKET_ERROR; // Set state mqtt socket error
          }

          OSTimeDly(TICKS_PER_SECOND * 5); // Block task for 5 seconds
        }
        else
        {
          state = UBIDOTS_OK; // Set state to OK
          mqttLoop = false;   // Break loop
        }

      } while (this->retries < UBIDOTS_CONNECT_RETRIES && mqttLoop);

      this->retries = 0; // Reset retries

      if (state != UBIDOTS_OK)
      { // if mqtt error

        // Get MQTT connection status
        int stateDisconnect = (this->ssl) ? this->mqttSSLSocket.disconnect()
                                          : this->mqttSocket.disconnect();

        if (this->cbPtrArr[UBIDOTS_EVENT_ERROR])
        {
          this->cbPtrArr[UBIDOTS_EVENT_ERROR]((void *)state); // Event error callback
        }
        OSTimeDly(TICKS_PER_SECOND * 5); // Block task for 5 seconds

        return false; // return false
      }

      this->connected = true; // Set connected to true
      this->consoleLog("Ubidots MQTT socket connected successfully\r\n");

      if (this->cbPtrArr[UBIDOTS_EVENT_CONNECTED])
      {
        this->cbPtrArr[UBIDOTS_EVENT_CONNECTED](nullptr); // Connected callback
      }

      return true; // return true
    }
    else
    {                                      // No socket available
      state = UBIDOTS_SOCKET_NO_AVAILABLE; // Set state to socket_no_available
      if (this->cbPtrArr[UBIDOTS_EVENT_ERROR])
      {
        this->cbPtrArr[UBIDOTS_EVENT_ERROR]((void *)state); // Error event
      }
    }
  }
  else
  {
    state = UBIDOTS_ALREADY_CONNECTED; // Set state to socket_no_available
    if (this->cbPtrArr[UBIDOTS_EVENT_ERROR])
    {
      this->cbPtrArr[UBIDOTS_EVENT_ERROR]((void *)state); // Error event
    }
  }

  return false;
}

bool Ubidots::subscribe(const char *variable, subscribe_handler_t handler)
{
  if (variable == nullptr && handler == nullptr)
    return false; // No data
  if (this->subTopicsUsed >= UBIDOTS_SUBSCRIBE_MAX_TOPICS)
    return false; // No topics available

  // Create subscribe topic using the base topic
  strcpy(this->subTopics[this->subTopicsUsed], this->baseTopic);
  strcat(this->subTopics[this->subTopicsUsed], "/");
  strcat(this->subTopics[this->subTopicsUsed], variable);
  strcat(this->subTopics[this->subTopicsUsed], "/lv");

  int subState = -1; // Subscription state

  if (this->client.isConnected() || this->clientSSL.isConnected())
  { // If mqtt connected
    subState = (this->ssl) ? this->clientSSL.subscribe(this->subTopics[this->subTopicsUsed], MQTT::QOS0, handler)
                           : this->client.subscribe(this->subTopics[this->subTopicsUsed], MQTT::QOS0, handler);
  }

  if (subState != 0)
    return false; // Subscribe error

  this->subTopicsUsed++; // Adds one topic used

  if (this->cbPtrArr[UBIDOTS_EVENT_SUBSCRIBED])
  {
    this->cbPtrArr[UBIDOTS_EVENT_SUBSCRIBED]((void *)nullptr); // Event subscribed callback
  }

  return true;
}

bool Ubidots::keepAlive()
{
  if (this->ssl)
  {
    if (this->clientSSL.isConnected())
    {
      clientSSL.yield(100);
    }
  }
  else
  {
    if (this->client.isConnected())
    {
      client.yield(100);
    }
  }

  if (this->client.isConnected() || this->clientSSL.isConnected())
  { // If client connected

    // Get MQTT yield status
    int yieldState = (this->ssl) ? this->clientSSL.yield(10)
                                 : this->client.yield(10);

    if (yieldState < 0)
    {                          // If error
      this->connected = false; // It means that socket is disconected
      if (this->cbPtrArr[UBIDOTS_EVENT_DISCONNECTED])
      {
        this->cbPtrArr[UBIDOTS_EVENT_DISCONNECTED]((void *)nullptr); // Event disconected callback
      }

      if (this->ssl)
      {
        this->clientSSL.disconnect();     // Disconnect to reset
        this->mqttSSLSocket.disconnect(); // Disconnect TCP socket
      }
      else
      {
        this->client.disconnect();     // Disconnect to reset
        this->mqttSocket.disconnect(); // Disconnect TCP socket
      }

      return false;
    }
    return true;
  }
  else
  {
    this->connected = false; // Not connected
    return false;
  }

  return false;
}

bool Ubidots::publish(const char *variable, float value)
{
  if (variable == nullptr)
    return false; // No variable name
  if (!this->connected)
    return false; // No mqtt connection active

  MQTT::Message message;              // message to send
  int pState = -1;                    // Publish state
  char buf[UBIDOTS_MSG_MAX_LEN] = {}; // Buffer for message

  siprintf(buf, TMPT_SIMPLE_PUBLISH_VAR, variable, value); // Format the data

  message.qos = MQTT::QOS0;         // Quality of service
  message.retained = false;         // Message retained on broker false
  message.dup = false;              // No duplicate message
  message.payload = (void *)buf;    // Data buffer
  message.payloadlen = strlen(buf); // Data buffer len

  pState = (this->ssl) ? this->clientSSL.publish(this->baseTopic, message) : this->client.publish(this->baseTopic, message);

  if (pState < 0)
  {                                                // If publish error
    ubidots_state_t state = UBIDOTS_PUBLISH_ERROR; // Ubidots state
    this->consoleLog("Publish Error [%d] \r\n", pState);
    if (this->cbPtrArr[UBIDOTS_EVENT_ERROR])
    {
      this->cbPtrArr[UBIDOTS_EVENT_ERROR]((void *)state); // Event error callback
    }
    return false;
  }

  if (this->cbPtrArr[UBIDOTS_EVENT_PUBLISHED])
  {
    this->cbPtrArr[UBIDOTS_EVENT_PUBLISHED]((void *)nullptr); // Event published callback
  }

  this->consoleLog("Message published %s to %s\r\n", buf, this->baseTopic);

  return true;
}

void Ubidots::registerCallback(ubidots_events_t event, void (*func_ptr)(void *))
{
  if (event < UBIDOTS_MESSAGE_CODE_COUNT)
  {
    this->cbPtrArr[event] = func_ptr;
  }
}

bool Ubidots::isConnected() const
{
  return this->connected;
}

const char *Ubidots::getDeviceName() const
{
  return this->device;
}
/*------------------------------------------------*/
