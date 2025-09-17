#ifndef flx_STRING_H
#define flx_STRING_H

#include <stdexcept>
#include <string>
#include <vector>

// Wrapper around std::string to provide additional functionality
class flx_string
{
  std::string str;
public:
  // npos
  static const size_t npos = std::string::npos;
  flx_string() : str() {}
  flx_string(const char* s) : str(s) {}
  flx_string(const char* s, size_t len) : str(s, len) {}
  flx_string(const std::string& s) : str(s) {}

  // from long and double:
  flx_string(long i) : str(std::to_string(i)) {}
  flx_string(long long i) : str(std::to_string(i)) {}
  flx_string(double d) : str(std::to_string(d)) {}
  // single char
  flx_string(char c) : str(1, c) {}

  std::string& to_std() { return str; }
  const std::string& to_std_const() const { return str; }
  const char* c_str() const { return str.c_str(); }

  flx_string operator+(const flx_string& s) const { return str + s.str; }
  flx_string operator+(const char* s) const { return str + s; }
  flx_string& operator+=(const flx_string& s) { str += s.str; return *this; }
  bool operator==(const flx_string& s) const { return str == s.str; }
  bool operator!=(const flx_string& s) const { return str != s.str; }
  bool operator<(const flx_string& s) const { return str < s.str; }

  bool empty() const { return str.empty(); }
  size_t size() const { return str.size(); }

  char& operator[](size_t i) { return str[i]; }

  flx_string substr(size_t pos, size_t len = npos) const { return str.substr(pos, len); }

  // clear
  void clear() { str.clear(); }
  // length
  size_t length() const { return str.length(); }

  long to_int(long def = 0) const
  {
    try
    {
      return std::stol(str);
    }
    catch (const std::invalid_argument&)
    {
      return def;
    }
    catch (const std::out_of_range&)
    {
      return def;
    }
  }

  double to_double(double def = 0) const
  {
    try
    {
      return std::stod(str);
    }
    catch (const std::invalid_argument&)
    {
      return def;
    }
    catch (const std::out_of_range&)
    {
      return def;
    }
  }

  bool is_integer() const
  {
    try
    {
      std::stol(str);
      return true;
    }
    catch (const std::invalid_argument&)
    {
      return false;
    }
    catch (const std::out_of_range&)
    {
      return false;
    }
  }

  bool is_double() const
  {
    try
    {
      std::stod(str);
      return true;
    }
    catch (const std::invalid_argument&)
    {
      return false;
    }
    catch (const std::out_of_range&)
    {
      return false;
    }
  }

  size_t find(const flx_string& s) const { return str.find(s.str); }
  size_t rfind(const flx_string& s) const { return str.rfind(s.str); }

  bool contains(const flx_string& s) const { return str.find(s.str) != std::string::npos; }

  // to_lower, to_upper
  flx_string to_lower() const
  {
    flx_string res = *this;
    for (size_t i = 0; i < res.size(); ++i)
    {
      res[i] = tolower(res[i]);
    }
    return res;
  }
  flx_string to_upper() const
  {
    flx_string res = *this;
    for (size_t i = 0; i < res.size(); ++i)
    {
      res[i] = toupper(res[i]);
    }
    return res;
  }

  flx_string& replace(size_t pos, size_t len, const flx_string& s) { str.replace(pos, len, s.str); return *this; }
  flx_string& replace(const flx_string& from, const flx_string& to) { for (size_t pos = 0; (pos = str.find(from.str, pos)) != std::string::npos; pos += to.size()) str.replace(pos, from.size(), to.str); return *this; }

  // erase
  flx_string& erase(size_t pos = 0, size_t len = std::string::npos)
  {
    str.erase(pos, len);
    return *this;
  }

  // append
  flx_string& append(const flx_string& s)
  {
    str.append(s.str);
    return *this;
  }

  // append with two parameters
  flx_string& append(const char* s, size_t len)
  {
    str.append(s, len);
    return *this;
  }

  size_t split(const flx_string& delim, std::vector<flx_string>& out) const
  {
    size_t pos = 0;
    size_t lastPos = 0;
    while ((pos = str.find(delim.str, lastPos)) != std::string::npos)
    {
      out.push_back(str.substr(lastPos, pos - lastPos));
      lastPos = pos + delim.size();
    }
    out.push_back(str.substr(lastPos));
    return out.size();
  }

  // split with return
  std::vector<flx_string> split(const flx_string& delim) const
  {
    std::vector<flx_string> out;
    split(delim, out);
    return out;
  }

  // lower and upper
  flx_string lower() const
  {
    flx_string res = *this;
    for (size_t i = 0; i < res.size(); ++i)
    {
      res[i] = tolower(res[i]);
    }
    return res;
  }

  flx_string upper() const
  {
    flx_string res = *this;
    for (size_t i = 0; i < res.size(); ++i)
    {
      res[i] = toupper(str[i]);
    }
    return res;
  }
};

// Global operators for const char* + flx_string
inline flx_string operator+(const char* lhs, const flx_string& rhs) {
    return flx_string(lhs) + rhs;
}

#endif // flx_STRING_H
