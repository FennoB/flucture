#include "api/server/flx_rest_api.h"
#include <fstream>
#include <iostream>
#include <thread>

int main(int argc, char *argv[])
{
  flxv_vector args;
  for (int i = 0; i < argc; i++)
  {
    args.push_back(flx_string(argv[i]));
  }

  flx_rest_api api(args);
  std::ifstream f;
  f.open("privkey.pem");
  if (f.is_open())
  {
    // read all bytes from the file
    std::string privkey((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();
    f.open("cert.pem");
    if (f.is_open())
    {
      // read all bytes from the file
      std::string cert((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
      f.close();
      std::cout << "Found certificates for https!" << std::endl;
      api.activate_ssl(privkey, cert);
    }
  }

  api.activate_thread_pool(16);
  if (!api.exec(12345))
  {
    return -1;
  }
  while (api.is_running())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return 0;
}
