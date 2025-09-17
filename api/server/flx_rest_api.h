#ifndef flx_flx_rest_api_H
#define flx_flx_rest_api_H

#include "../../utils/flx_variant.h"
#include "flx_httpdaemon.h"
#include "../../utils/flx_string.h"

class flx_rest_api : public flx_http_daemon
{
  flxv_vector args;
public:
  flx_rest_api(flxv_vector &args) : args(args)
  {
    this->args = args;
  }
  response handle(request req);
  response dispatch(request req);
};

#endif // flx_flx_rest_api_H
