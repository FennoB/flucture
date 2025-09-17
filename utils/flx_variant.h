#ifndef flx_VARIANT_H
#define flx_VARIANT_H

#include "flx_string.h"
#include <map>

class flx_variant;

typedef std::vector<flx_variant> flxv_vector;
typedef std::map<flx_string, flx_variant> flxv_map;
typedef flxv_map::iterator flxvm_iterator;

#define flxv_string flx_string
#define flxv_int long long
#define flxv_bool bool
#define flxv_double double

#define flxv_string_state flx_variant::string_state
#define flxv_int_state flx_variant::int_state
#define flxv_bool_state flx_variant::bool_state
#define flxv_double_state flx_variant::double_state
#define flxv_vector_state flx_variant::vector_state
#define flxv_map_state flx_variant::map_state

#define flxv_detect_state(value) flx_variant(value).in_state()

class flx_variant
{
public:
  enum state
  {
    none,
    string_state,
    int_state,
    bool_state,
    double_state,
    vector_state,
    map_state
  };

private:
  void* content;
  state is;

  // Make value visible in debug mode.
#ifdef E_DEBUG
  flx_string* string_debug = nullptr;
  long* int_debug = nullptr;
  bool* bool_debug = nullptr;
  double* double_debug = nullptr;
  flxv_map* map_debug = nullptr;
  flxv_vector* vec_debug = nullptr;
#endif

  void copy_from(const flx_variant &other);

public:
  template<typename to>
  to* cast_content() const
  {
    return static_cast<to*>(content);
  }
  void clear();
  void reset(state to);
  ~flx_variant();
  flx_variant();
  flx_variant(const char* fromString);
  flx_variant(const flx_string &fromString);
  flx_variant(int from_int);
  flx_variant(bool from_bool);
  flx_variant(long long from_int);
  flx_variant(double fromDouble);
  flx_variant(const flxv_vector &from_vector);
  flx_variant(const flxv_map &from_map);
  flx_variant(const flx_variant &other);

  state in_state() const;
  bool is_null() const;
  bool is_string() const;
  bool is_int() const;
  bool is_bool() const;
  bool is_double() const;
  bool is_dateTime() const;
  bool is_vector() const;
  bool is_map() const;

  // These are safe to use without type check
  flx_string& to_string();
  long long& to_int();
  bool& to_bool();
  double& to_double();
  flxv_vector& to_vector();
  flxv_map& to_map();

  template<typename type>
  type &to()
  {
    state tstate = flxv_detect_state(type());
    if (is != tstate)
    {
      *this = convert(tstate);
    }
    return *cast_content<type>();
  }

  // These should only be used after type check
  const flx_string& string_value() const;
  const long long& int_value() const;
  const bool& bool_value() const;
  const double& double_value() const;
  const flxv_vector& vector_value() const;
  const flxv_map& map_value() const;

  bool converts_to(state s) const;
  flx_variant convert(state to) const;

  flx_variant& operator=(const flx_variant &other);
  flx_variant& operator=(const char *s);
  flx_variant& operator=(flx_string s);
  flx_variant& operator=(long long i);
  flx_variant& operator=(int i);
  flx_variant& operator=(bool b);
  flx_variant& operator=(double d);
  flx_variant& operator=(const flxv_vector &v);
  flx_variant& operator=(const flxv_map &m);
  
  operator flx_string() const
  {
    return convert(string_state).to_string();
  }

  operator double() const
  {
    return convert(double_state).to_double();
  }

  operator long long() const
  {
    return convert(int_state).to_int();
  }

  operator int() const
  {
    return (int)convert(int_state).to_int();
  }

  operator bool() const
  {
    return convert(bool_state).to_bool();
  }

  operator flxv_vector() const
  {
    return convert(vector_state).to_vector();
  }
  
  operator flxv_map()
  {
    return convert(map_state).to_map();
  }

  bool operator==(const flx_variant &other) const;

  bool operator!=(const flx_variant& other) const
  {
    return !(*this == other);
  }

  bool operator<(const flx_variant &other) const;
  bool operator>(const flx_variant &other) const
  {
    return other < *this;
  }
};

#endif // flx_VARIANT_H
