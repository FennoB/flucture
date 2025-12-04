#include "flx_variant.h"

void flx_variant::copy_from(const flx_variant &other)
{
  reset(other.is);
  if (other.is == string_state)
  {
    *cast_content<flx_string>() = other.string_value();
  }
  if (other.is == int_state)
  {
    *cast_content<long>() = other.int_value();
  }
  if (other.is == double_state)
  {
    *cast_content<double>() = other.double_value();
  }
  if (other.is == bool_state)
  {
    *cast_content<bool>() = other.bool_value();
  }
  if (other.is == vector_state)
  {
    *cast_content<flxv_vector>() = other.vector_value();
  }
  if (other.is == map_state)
  {
    *cast_content<flxv_map>() = other.map_value();
  }
}

void flx_variant::clear()
{
  if (is == string_state)
  {
    delete cast_content<flx_string>();
  }
  if (is == int_state)
  {
    delete cast_content<long>();
  }
  if (is == double_state)
  {
    delete cast_content<double>();
  }
  if (is == bool_state)
  {
    delete cast_content<bool>();
  }
  if (is == vector_state)
  {
    delete cast_content<flxv_vector>();
  }
  if (is == map_state)
  {
    delete cast_content<flxv_map>();
  }
  content = nullptr;
#ifdef E_DEBUG
  string_debug = nullptr;
  int_debug = nullptr;
  bool_debug = nullptr;
  double_debug = nullptr;
  map_debug = nullptr;
  vec_debug = nullptr;
#endif
  is = none;
}

void flx_variant::reset(flx_variant::state to)
{
  clear();
  is = to;
  if (is == string_state)
  {
    content = new flx_string;
#ifdef E_DEBUG
    string_debug = cast_content<flx_string>();
#endif
  }
  if (is == int_state)
  {
    content = new long(0);
#ifdef E_DEBUG
    int_debug = cast_content<long>();
#endif
  }
  if (is == double_state)
  {
    content = new double(0.0);
#ifdef E_DEBUG
    double_debug = cast_content<double>();
#endif
  }
  if (is == bool_state)
  {
    content = new bool(false);
#ifdef E_DEBUG
    bool_debug = cast_content<bool>();
#endif
  }
  if (is == vector_state)
  {
    content = new flxv_vector;
#ifdef E_DEBUG
    vec_debug = cast_content<flxv_vector>();
#endif
  }
  if (is == map_state)
  {
    content = new flxv_map;
#ifdef QT_DEBUG
    map_debug = cast_content<flxv_map>();
#endif
  }
}

flx_variant::~flx_variant()
{
  clear();
}

flx_variant::flx_variant()
{
  is = none;
  content = nullptr;
}

flx_variant::flx_variant(const char *from_string)
{
  is = string_state;
  content = new flx_string(from_string);
}

flx_variant::flx_variant(const flx_string &from_string)
{
  is = string_state;
  content = new flx_string(from_string);
}

flx_variant::flx_variant(int from_int)
{
  is = int_state;
  content = new long(from_int);
}

flx_variant::flx_variant(bool from_bool)
{
  is = bool_state;
  content = new bool(from_bool);
}

flx_variant::flx_variant(long long from_int)
{
  is = int_state;
  content = new long long(from_int);
}

flx_variant::flx_variant(double from_double)
{
  is = double_state;
  content = new double(from_double);
}

flx_variant::flx_variant(const flxv_vector &from_vector)
{
  is = vector_state;
  content = new flxv_vector(from_vector);
}

flx_variant::flx_variant(const flxv_map &from_map)
{
  is = map_state;
  content = new flxv_map(from_map);
}

flx_variant::flx_variant(const flx_variant &other)
{
  is = none;
  content = nullptr;
  copy_from(other);
}

flx_variant::state flx_variant::in_state() const
{
  return is;
}

bool flx_variant::is_null() const
{
  return is == none;
}

bool flx_variant::is_string() const
{
  return is == string_state;
}

bool flx_variant::is_int() const
{
  return is == int_state;
}

bool flx_variant::is_bool() const
{
  return is == bool_state;
}

bool flx_variant::is_double() const
{
  return is == double_state;
}

bool flx_variant::is_vector() const
{
  return is == vector_state;
}

bool flx_variant::is_map() const
{
  return is == map_state;
}

flx_string &flx_variant::to_string()
{
  if (is != string_state)
  {
    *this = convert(string_state);
  }
  return *cast_content<flx_string>();
}

long long &flx_variant::to_int()
{
  if (is != int_state)
  {
    *this = convert(int_state);
  }
  return *cast_content<long long>();
}

bool &flx_variant::to_bool()
{
  if (is != bool_state)
  {
    *this = convert(bool_state);
  }
  return *cast_content<bool>();
}

double &flx_variant::to_double()
{
  if (is != double_state)
  {
    *this = convert(double_state);
  }
  return *cast_content<double>();
}

flxv_vector &flx_variant::to_vector()
{
  if (is != vector_state)
  {
    *this = convert(vector_state);
  }
  return *cast_content<flxv_vector>();
}

flxv_map &flx_variant::to_map()
{
  if (is != map_state)
  {
    *this = convert(map_state);
  }
  return *cast_content<flxv_map>();
}

const flx_string &flx_variant::string_value() const
{
  return *cast_content<flx_string>();
}

const long long &flx_variant::int_value() const
{
  return *cast_content<long long>();
}

const bool &flx_variant::bool_value() const
{
  return *cast_content<bool>();
}

const double &flx_variant::double_value() const
{
  return *cast_content<double>();
}

const flxv_vector &flx_variant::vector_value() const
{
  return *cast_content<flxv_vector>();
}

const flxv_map &flx_variant::map_value() const
{
  return *cast_content<flxv_map>();
}

bool flx_variant::converts_to(flx_variant::state s) const
{
  if (is == s)
  {
    return true;
  }
  if (is == string_state)
  {
    return s == string_state ||
           (s == bool_state && (string_value().lower() == "true" ||
                                string_value().lower() == "false" ||
                                string_value().is_integer())) ||
           (s == int_state && string_value().is_integer()) ||
           (s == double_state && string_value().is_double());
  }
  else if (is == bool_state)
  {
    return s == int_state || s == string_state;
  }
  else if (is == int_state)
  {
    return s == int_state || s == double_state || s == string_state;
  }
  else if (is == double_state)
  {
    return s == int_state || s == double_state || s == string_state;
  }
  return false;
}

flx_variant flx_variant::convert(flx_variant::state to) const
{
  flx_variant res;
  res.reset(to);

  if (is == to)
  {
    res = *this;
  }
  if (is == string_state)
  {
    if (to == int_state)
    {
      res = (long long)string_value().to_int(0);
    }
    else if (to == bool_state)
    {
      flx_string lower = string_value().lower();
      // Support: "true", "t" (PostgreSQL), "1", or any non-zero number
      if (lower == "true" || lower == "t" || string_value().to_int(0))
      {
        res = true;
      }
      else
      {
        res = false;
      }
    }
    else if (to == double_state)
    {
      res = string_value().to_double(0);
    }

  }
  else if (is == bool_state)
  {
    if (to == int_state)
    {
      res = (int)bool_value();
    }
    else if (to == string_state)
    {
      res = bool_value() ? "true" : "false";
    }
  }
  else if (is == int_state)
  {
    if (to == double_state)
    {
      res = (double)int_value();
    }
    else if (to == bool_state)
    {
      res = (bool)int_value();
    }
    else if (to == string_state)
    {
      res = flx_string(int_value());
    }
  }
  else if (is == double_state)
  {
    if (to == int_state)
    {
      res = (long long)double_value();
    }
    else if (to == string_state)
    {
      res = flx_string(double_value());
    }
  }
  return res;
}

flx_variant &flx_variant::operator=(const flx_variant &other)
{
  copy_from(other);
  return *this;
}

flx_variant &flx_variant::operator=(const flxv_vector &v)
{
  return *this = flx_variant(v);
}

flx_variant &flx_variant::operator=(const flxv_map &m)
{
  return *this = flx_variant(m);
}

flx_variant &flx_variant::operator=(double d)
{
  return *this = flx_variant(d);
}

flx_variant &flx_variant::operator=(long long i)
{
  return *this = flx_variant(i);
}

flx_variant &flx_variant::operator=(int i)
{
  return *this = flx_variant(i);
}

flx_variant &flx_variant::operator=(bool b)
{
  return *this = flx_variant(b);
}

flx_variant &flx_variant::operator=(const char *s)
{
  return *this = flx_variant(s);
}

flx_variant &flx_variant::operator=(flx_string s)
{
  return *this = flx_variant(s);
}

bool flx_variant::operator==(const flx_variant &other) const
{
  if (is_string())
  {
    return string_value() == other.convert(string_state).string_value();
  }
  else if (is_int())
  {
    return int_value() == other.convert(int_state).int_value();
  }
  else if (is_bool())
  {
    return bool_value() == other.convert(bool_state).bool_value();
  }
  else if (is_double())
  {
    flx_variant o = other.convert(double_state);
    return convert(string_state) == o.convert(string_state);
  }
  if (is_vector())
  {
    return vector_value() == other.convert(vector_state).vector_value();
  }
  if (is_map())
  {
    const flxv_map& a = map_value();
    const flxv_map b = other.convert(map_state).map_value();
    if (a.size() != b.size())
    {
      return false;
    }
    flxv_map::const_iterator it(a.begin());
    while (it != a.end())
    {
      flxv_map::const_iterator it2 = b.find(it->first);
      if (it2 == b.end() || it->second != it2->second)
      {
        return false;
      }
      ++it;
    }
    return true;
  }
  return other.is == none;
}

bool flx_variant::operator<(const flx_variant &other) const
{
  if (is == string_state || is == map_state || is == vector_state ||
      other.is == string_state || other.is == map_state || other.is == vector_state)
  {
    return (const flx_string&)(*this) < (const flx_string&)(other);
  }
  else
  {
    return (const double&)(*this) < (const double&)(other);
  }
}

