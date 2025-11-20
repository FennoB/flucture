#ifndef flx_HTTPDAEMON_H
#define flx_HTTPDAEMON_H

#include "microhttpd.h"
#include "../../utils/flx_string.h"
#include <map>
#include <mutex>

// Compatibility with older libmicrohttpd versions
#if MHD_VERSION < 0x00097002
typedef int MHD_Result;
#endif

class flx_http_daemon
{
  static MHD_Result fill_request(void *req, enum MHD_ValueKind kind,
                                const char *key, const char *value);

  static MHD_Result echo(void * cls,
                         struct MHD_Connection * connection,
                         const char * url,
                         const char * method,
                         const char * version,
                         const char * upload_data,
                         size_t * upload_data_size,
                         void ** ptr);
  bool ssl;
  flx_string privatekey;
  flx_string certificate;
  volatile bool running;
  MHD_Daemon* daemon;
  size_t threads;
  std::mutex mutex;

public:
  flx_http_daemon();
  ~flx_http_daemon();

  bool exec(int port);
  void stop();
  bool is_running();

  bool check_ssl_supported();
  void activate_ssl(flx_string privatekey, flx_string certificate);
  void activate_thread_pool(size_t threads);

  struct request
  {
    flx_string path;
    flx_string method;
    flx_string body;
    std::map<flx_string, flx_string> headers;
    std::map<flx_string, flx_string> params;
  };

  struct response
  {
    flx_string body;
    std::map<flx_string, flx_string> headers;
    int statuscode = 0;
  };

  virtual response handle(request req);
};

#endif // flx_HTTPDAEMON_H
