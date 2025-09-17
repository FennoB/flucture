#include "flx_httpdaemon.h"
#include <errno.h>
#include <string.h>
#include <iostream>

flx_http_daemon::flx_http_daemon()
{
  threads = 1;
  running = false;
  ssl = false;
}

flx_http_daemon::~flx_http_daemon()
{
  stop();
}

MHD_Result flx_http_daemon::fill_request(void *r, MHD_ValueKind kind, const char *key, const char *value)
{
  request *req = static_cast<request*>(r);
  if (key == nullptr || value == nullptr)
  {
    return MHD_NO;
  }

  if (kind == MHD_GET_ARGUMENT_KIND)
  {
    //std::cout << "Parameter: '" << key << "': '" << value << "'" << std::endl;
    req->params[key] = value;
  }
  else if (kind == MHD_HEADER_KIND)
  {
    std::cout << "Header: '" << key << "': '" << value << "'" << std::endl;
    req->headers[key] = value;
  }
  return MHD_YES;
}

void request_completed (void *, struct MHD_Connection *,
                        void **con_cls,
                        enum MHD_RequestTerminationCode )
{
    flx_http_daemon::request *req = static_cast<flx_http_daemon::request*>(*con_cls);
    std::cout << "request completed" << std::endl << std::endl;

    if (!req)
    {
      std::cout << "request_completed: req was nullptr!" << std::endl;
      return;
    }
    delete req;
    *con_cls = NULL;
}

MHD_Result flx_http_daemon::echo(void *cls, MHD_Connection *connection,
                             const char *url,
                             const char *method,
                             const char *,
                             const char *upload_data,
                             size_t *upload_data_size,
                             void **con_cls)
{
  flx_http_daemon* daemon = static_cast<flx_http_daemon*>(cls);
  MHD_Response * resp;
  MHD_Result ret;

  request *req = static_cast<request*>(*con_cls);

  if (req == nullptr)
  {
    // This is the beginning of a new request
    std::cout << "Incoming request: " << method << ": " << url << std::endl;
    req = new request();
    req->path = url;
    req->method = method;

    // The headers are already valid. Lets use them
    MHD_get_connection_values(connection, MHD_HEADER_KIND, &fill_request, req);

    *con_cls = req;
    return MHD_YES;
  }

  if (*upload_data_size)
  {
    // Just add the data to the body!
    req->body += flx_string(upload_data, *upload_data_size);
    // Tell MHD that we have consumed the chunk
    *upload_data_size = 0;
    return MHD_YES;
  }

  // Get url parameters
  MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, &fill_request, req);

  if (req->body.size())
  {
    //std::cout << "Request Body: \n" << req->body.toStdString() << std::endl;
  }

  // Process the constructed request
  response result(daemon->handle(*req));

  // Construct response
  flx_string data = result.body;
  resp = MHD_create_response_from_buffer (data.size(), (void*) data.c_str(), MHD_RESPMEM_MUST_COPY);
  for (auto i = result.headers.begin(); i != result.headers.end(); ++i)
  {
    MHD_add_response_header (resp, i->first.c_str(), i->second.c_str());
  }
  if (result.body.size())
  {
    if (result.body.size() < 1000)
    {
      std::cout << "Response body: \n" << result.body.to_std() << std::endl;
    }
    else
    {
      std::cout << "Body too long..." << std::endl;
    }
  }
  else
  {
    std::cout << "Response has empty body!" << std::endl;
  }
  ret = MHD_queue_response(connection,
                           result.statuscode,
                           resp);
  MHD_destroy_response(resp);
  std::cout << "Finished with statuscode " << result.statuscode << std::endl;
  return ret;
}

bool flx_http_daemon::exec(int port)
{
  mutex.lock();
  if (is_running())
  {
    stop();
  }
  std::cout << "Starting daemon on port " << port << " using " << threads << " threads." << std::endl;
  if (ssl)
  {
    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_SSL,
                              port,
                              nullptr,
                              nullptr,
                              &echo,
                              this,
                              MHD_OPTION_THREAD_POOL_SIZE, threads,
                              MHD_OPTION_HTTPS_MEM_KEY, privatekey.c_str(),
                              MHD_OPTION_HTTPS_MEM_CERT, certificate.c_str(),
                              MHD_OPTION_NOTIFY_COMPLETED, &request_completed, nullptr,
                              MHD_OPTION_END
                              );
  }
  else
  {
    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD,
                              port,
                              nullptr,
                              nullptr,
                              &echo,
                              this,
                              MHD_OPTION_THREAD_POOL_SIZE, threads,
                              MHD_OPTION_NOTIFY_COMPLETED, &request_completed, nullptr,
                              MHD_OPTION_END
                              );
  }
  if (daemon == nullptr)
  {
    std::cout << "Error starting daemon." << strerror(errno) << std::endl;
    mutex.unlock();
    return false;
  }

  running = true;
  mutex.unlock();
  return true;
}

void flx_http_daemon::stop()
{
  mutex.lock();
  if (running)
  {
    MHD_stop_daemon(daemon);
  }
  running = false;
  mutex.unlock();
}

bool flx_http_daemon::is_running()
{
  return running;
}

bool flx_http_daemon::check_ssl_supported()
{
  return MHD_YES == MHD_is_feature_supported(MHD_FEATURE_SSL);
}

void flx_http_daemon::activate_ssl(flx_string privatekey, flx_string certificate)
{
  this->privatekey = privatekey;
  this->certificate = certificate;
  ssl = true;
}

void flx_http_daemon::activate_thread_pool(size_t threads)
{
  this->threads = threads;
}

flx_http_daemon::response flx_http_daemon::handle(flx_http_daemon::request req)
{
  response r;
  r.body = req.body;
  r.statuscode = 200;
  return r;
}
