#ifndef flx_STRING_H
#define flx_STRING_H

#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

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

  flx_string trim() const
  {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return flx_string();
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
  }

  flx_string trim_left() const
  {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return flx_string();
    return str.substr(start);
  }

  flx_string trim_right() const
  {
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    if (end == std::string::npos) return flx_string();
    return str.substr(0, end + 1);
  }

  bool starts_with(const flx_string& prefix) const
  {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix.str;
  }

  bool ends_with(const flx_string& suffix) const
  {
    return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix.str;
  }

  flx_string left(size_t count) const
  {
    return str.substr(0, count);
  }

  flx_string right(size_t count) const
  {
    if (count >= str.size()) return *this;
    return str.substr(str.size() - count);
  }

  flx_string mid(size_t start, size_t count = npos) const
  {
    return str.substr(start, count);
  }

  flx_string reverse() const
  {
    flx_string result = *this;
    std::reverse(result.str.begin(), result.str.end());
    return result;
  }

  size_t count(const flx_string& substring) const
  {
    size_t count = 0;
    size_t pos = 0;
    while ((pos = str.find(substring.str, pos)) != std::string::npos) {
      count++;
      pos += substring.size();
    }
    return count;
  }

  size_t count(char ch) const
  {
    return std::count(str.begin(), str.end(), ch);
  }

  flx_string repeat(size_t times) const
  {
    flx_string result;
    for (size_t i = 0; i < times; ++i) {
      result += *this;
    }
    return result;
  }

  flx_string pad_left(size_t total_width, char pad_char = ' ') const
  {
    if (str.size() >= total_width) return *this;
    return flx_string(pad_char).repeat(total_width - str.size()) + *this;
  }

  flx_string pad_right(size_t total_width, char pad_char = ' ') const
  {
    if (str.size() >= total_width) return *this;
    return *this + flx_string(pad_char).repeat(total_width - str.size());
  }

  flx_string pad_center(size_t total_width, char pad_char = ' ') const
  {
    if (str.size() >= total_width) return *this;
    size_t pad_total = total_width - str.size();
    size_t pad_left = pad_total / 2;
    size_t pad_right = pad_total - pad_left;
    return flx_string(pad_char).repeat(pad_left) + *this + flx_string(pad_char).repeat(pad_right);
  }

  flx_string join(const std::vector<flx_string>& parts) const
  {
    if (parts.empty()) return flx_string();
    if (parts.size() == 1) return parts[0];
    
    flx_string result = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
      result += *this + parts[i];
    }
    return result;
  }

  bool is_numeric() const
  {
    if (str.empty()) return false;
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') start = 1;
    if (start >= str.size()) return false;
    
    bool has_dot = false;
    for (size_t i = start; i < str.size(); ++i) {
      if (str[i] == '.') {
        if (has_dot) return false;
        has_dot = true;
      } else if (!std::isdigit(str[i])) {
        return false;
      }
    }
    return true;
  }

  flx_string remove(const flx_string& substring) const
  {
    flx_string result = *this;
    result.replace(substring, "");
    return result;
  }

  flx_string remove_all(const flx_string& substring) const
  {
    flx_string result = *this;
    while (result.contains(substring)) {
      result.replace(substring, "");
    }
    return result;
  }

  std::vector<flx_string> lines() const
  {
    return split("\n");
  }

  flx_string normalize_whitespace() const
  {
    flx_string result;
    bool in_whitespace = false;
    
    for (char c : str) {
      if (std::isspace(c)) {
        if (!in_whitespace) {
          result += ' ';
          in_whitespace = true;
        }
      } else {
        result += c;
        in_whitespace = false;
      }
    }
    
    return result.trim();
  }

  flx_string capitalize() const
  {
    if (str.empty()) return *this;
    flx_string result = to_lower();
    result[0] = std::toupper(result[0]);
    return result;
  }

  flx_string title_case() const
  {
    flx_string result = to_lower();
    bool capitalize_next = true;
    
    for (size_t i = 0; i < result.size(); ++i) {
      if (std::isalpha(result[i])) {
        if (capitalize_next) {
          result[i] = std::toupper(result[i]);
          capitalize_next = false;
        }
      } else {
        capitalize_next = true;
      }
    }
    
    return result;
  }
};

// Global operators for const char* + flx_string
inline flx_string operator+(const char* lhs, const flx_string& rhs) {
    return flx_string(lhs) + rhs;
}

#endif // flx_STRING_H
