#include "flx_doc_sio.h"
#include "fstream"

bool flx_doc_sio::read(flx_string filename)
{
  std::fstream f;
  f.open(filename.c_str(), std::ios::in | std::ios::binary);
  if (!f.is_open())
  {
    return false;
  }
  // read all bytes from the file
  std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  f.close();

  flx_string str = data;
  // parse the data
  if (!parse(str))
  {
    return false;
  }
  return true;
}

bool flx_doc_sio::write(flx_string filename)
{
  std::fstream f;
  f.open(filename.c_str(), std::ios::out | std::ios::binary);
  if (!f.is_open())
  {
    return false;
  }
  // serialize the data
  flx_string data;
  if (!serialize(data))
  {
    return false;
  }
  f.write(data.c_str(), data.size());
  f.close();
  return true;
}

flx_layout_geometry& flx_doc_sio::add_page() {
  flx_layout_geometry new_page;
  pages.push_back(new_page);
  return pages[pages.size() - 1];
}

size_t flx_doc_sio::page_count() const {
  return pages.size();
}
