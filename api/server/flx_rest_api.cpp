#include "flx_rest_api.h"

flx_http_daemon::response flx_rest_api::handle(flx_http_daemon::request req)
{
  response r;
  flx_string data = req.body;
  
  flx_string origin = req.headers["Origin"];
  r.headers["Access-Control-Allow-Origin"] = origin;
  if (req.method == "GET" || req.method == "POST" || req.method == "PUT" || req.method == "DELETE")
  {
    r = dispatch(req);
  }
  else if (req.method == "OPTIONS")
  {
    r.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    r.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
    r.statuscode = 204;
  }
  return r;
}

flx_http_daemon::response flx_rest_api::dispatch(request req)
{
  response r;
  flx_string token;
  if (req.headers.find("Authorization") != req.headers.end())
  {
    std::vector<flx_string> t;
    req.headers["Authorization"].split(" ", t);
    token = t[1];
  }
  r.statuscode = 200;
  return r;
}
