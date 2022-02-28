#pragma once

#include <tcp.h>
#include <dns.h>
#include <iosys.h>
#include <crypto/ssl.h>
// #include <crypto/SSLContext.h>

class NBMQTTTLSSocket
{
public:
  int mysock;

  NBMQTTTLSSocket()
  {
    mysock = -1;
  }

  // void initSSL() {
  //   // initialize client certs
  //   RegisterClientCertVerification();
  //   SSLContext::getClientContext()->initialize();
  // }

  int connect(char *hostname, int port, int timeout = 10)
  {
    IPADDR4 addr;
    GetHostByName4(hostname, &addr, 0, 0);
    return connect(addr, port, timeout);
  }

  int connect(IPADDR addr, int port, int timeout = 10)
  {
    // Use :: to call from global namespace, not the class function
    mysock = ::SSL_connect(addr, 0, port, timeout * TICKS_PER_SECOND, NULL);

    return (mysock > 0) ? 0 : mysock;
  }

  int read(unsigned char *buffer, int len, int timeout)
  {
    if (mysock > 0)
    {
      return ReadWithTimeout(mysock, reinterpret_cast<char *>(buffer), len, timeout);
    }
    return 0;
  }

  int write(unsigned char *buffer, int len, int timeout)
  {
    if (mysock > 0)
    {
      // Use :: to call from global namespace, not the class function
      return ::write(mysock, reinterpret_cast<char *>(buffer), len);
    }
    return 0;
  }

  int disconnect()
  {
    return (mysock > 0) ? close(mysock) : 0;
  }
};
