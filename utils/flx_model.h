#ifndef flx_MODEL_H
#define flx_MODEL_H

#include "flx_lazy_ptr.h"
#include "flx_variant.h"
#include <type_traits>

// Enable the advanced variant-based model system
#define flx_variant_models

// Exception for null field access in properties
class flx_null_field_exception : public std::exception {
    flx_string field_name;
    mutable flx_string message;
public:
    flx_null_field_exception(const flx_string& name) : field_name(name) {}
    const char* what() const noexcept override {
        message = flx_string("Access to null field: ") + field_name;
        return message.c_str();
    }
    const flx_string& get_field_name() const { return field_name; }
};

#ifndef flx_variant_models
class flx_model {};
#define flxp_int(name) long long name = 0
#define flxp_string(name) flx_string name = flx_string()
#define flxp_bool(name) bool name = false
#define flxp_double(name) double name = 0.0
#define flxp_vector(name) flxv_vector name = flxv_vector()
#define flxp_map(name) flxv_map name = flxv_map()
#define flx_model_list std::vector
#else
class flx_model;

class flx_property_i
{
protected:
  flx_model* parent;
  flx_string name;
public:
  flx_property_i(flx_model* parent, const flx_string &name);
  
  flx_string prop_name() const { return name; }
  
  // Non-const access - can create new values
  virtual flx_variant& access();
  
  // Const access - throws exception if null
  virtual const flx_variant& const_access() const;
  
  // Null check - handles exception internally
  bool is_null() const;
};

template <typename type>
class flx_property : public flx_property_i
{
  flx_variant::state variant_type;
public:
  flx_property(flx_model* parent, const flx_string &name) : flx_property_i(parent, name)
  {
    flx_variant v = type{};
    variant_type = v.in_state();
  }

  // Non-const access - creates default value if null
  type &value()
  {
    flx_variant& data = this->access();
    if (data.in_state() == flx_variant::none) {
      data = type{};  // Create default value
    }
    if (data.in_state() != variant_type) {
      data = data.convert(variant_type);
    }
    return data.template to<type>();
  }
  
  // Const access - throws exception if null
  const type &value() const
  {
    const flx_variant& data = this->const_access();
    if (data.in_state() == flx_variant::none) {
      throw flx_null_field_exception(prop_name());
    }
    // Note: In const context, we can't convert, so we assume correct type
    // We need to cast away const to access the data - this is safe because
    // we're only reading, not modifying the underlying value
    return const_cast<flx_variant&>(data).template to<type>();
  }

  // Non-const operators
  type &operator*() { return value(); }
  type *operator->() { return &value(); }
  
  // Const operators
  const type &operator*() const { return value(); }
  const type *operator->() const { return &value(); }

  // Assignment operator
  flx_property& operator=(const type &new_value)
  {
    value() = new_value;  // Use non-const value() which creates if needed
    return *this;
  }

  // Comparison operators
  bool operator==(const type &other) const 
  {
    if (is_null()) return false;
    return value() == other;
  }
  
  // Special int comparison for properties holding long long
  template<typename T = type>
  typename std::enable_if<std::is_same<T, long long>::value, bool>::type
  operator==(int other) const 
  {
    if (is_null()) return false;
    return value() == static_cast<long long>(other);
  }
  
  bool operator!=(const type &other) const 
  {
    return !(*this == other);
  }
  
  // Special int comparison for properties holding long long
  template<typename T = type>
  typename std::enable_if<std::is_same<T, long long>::value, bool>::type
  operator!=(int other) const 
  {
    return !(*this == other);
  }

  // Conversion operators
  operator type&() { return value(); }
  operator const type&() const { return value(); }
  
  // Reference assignment support: int& ref = model.val; ref = 10;
  // This allows: model.val returns type& which can be assigned to
};

class flx_model : public flx_lazy_ptr<flxv_map>
{
  std::map<flx_string, flx_property_i*> props;
  flx_property<flxv_map>* parent_property;
public:
  flx_model();
  void add_prop(flx_property_i* prop, const flx_string &name);
  void set_parent(flx_property<flxv_map>* parent_prop);
  
  // Override to sync with parent before access
  virtual flxv_map &operator*() override;
  virtual const flxv_map &operator*() const override;
  flx_variant& operator[](const flx_string &key)
  {
    return (**this)[key];
  }
  // copy constructor
  flx_model(flx_model &other) : flx_lazy_ptr<flxv_map>(other)
  {
    **this = *other;
  }
  // const copy constructor
  flx_model(const flx_model &other) : flx_lazy_ptr<flxv_map>()
  {
    try {
      **this = *other;
    } catch (...) {
      // Ignore exceptions during const copy
    }
  }
  // operator=
  flx_model& operator=(flx_model &other)
  {
    **this = *other;
    return *this;
  }
  template<typename T = flx_model>
  static T from_map(flx_property<flxv_map>& map_prop)
  {
    T model;
    model.set(&map_prop.value());
    model.set_parent(&map_prop);
    return model;
  }
};

template <typename model>
class flx_model_list : public flx_lazy_ptr<flxv_vector>
{
  std::map<size_t, model> cache;
  flx_property<flxv_vector>* parent_property;
public:
  flx_model_list() : flx_lazy_ptr<flxv_vector>(), parent_property(nullptr)
  {
  }
  
  void set_parent(flx_property<flxv_vector>* parent_prop)
  {
    parent_property = parent_prop;
  }
  
  // Override to sync with parent before access
  virtual flxv_vector &operator*() override
  {
    if (parent_property != nullptr)
    {
      this->set(&parent_property->value());
    }
    return flx_lazy_ptr<flxv_vector>::operator*();
  }
  
  // Const version - can't sync, so just delegate to base
  virtual const flxv_vector &operator*() const override
  {
    return flx_lazy_ptr<flxv_vector>::operator*();
  }

  // Add an empty element to the list
  void add_element()
  {
    flxv_map empty_map;
    (**this).push_back(empty_map);
    cache[this->size() - 1].set(&(**this)[this->size() - 1].to_map());
  }
  
  // Add a model to the list (const parameter)
  void push_back(const model &m)
  {
    flxv_map map_copy;
    try {
      map_copy = *m;
    } catch (...) {
      // Use empty map if copy fails
    }
    (**this).push_back(map_copy);
    cache[this->size() - 1].set(&(**this)[this->size() - 1].to_map());
  }

  model& back()
  {
    return at(this->size() - 1);
  }

  // Get a model by index
  model& at(size_t index)
  {
    if (index >= this->size())
    {
      throw std::out_of_range("Index out of range");
    }
    cache[index].set(&(**this)[index].to_map());
    return cache[index];
  }

  // at const
  const model& at(size_t index) const
  {
    if (index >= this->size())
    {
      throw std::out_of_range("Index out of range");
    }
    if (cache.find(index) == cache.end())
    {
      throw flx_null_access_exception();
    }
    return cache.at(index);
  }

  // Get the size of the list
  size_t size()
  {
    return (**this).size();
  }

  // const
  size_t size() const
  {
    return (**this).size();
  }

  model& operator[](size_t index)
  {
    return at(index);
  }

  static flx_model_list from_vector(flx_property<flxv_vector>& vec_prop)
  {
    flx_model_list<model> list;
    list.set(&vec_prop.value());
    list.set_parent(&vec_prop);
    return list;
  }
};

#define flxp_int(name) flx_property<flxv_int> name = flx_property<flxv_int>(this, #name)
#define flxp_string(name) flx_property<flxv_string> name = flx_property<flxv_string>(this, #name)
#define flxp_bool(name) flx_property<flxv_bool> name = flx_property<flxv_bool>(this, #name)
#define flxp_double(name) flx_property<flxv_double> name = flx_property<flxv_double>(this, #name)
#define flxp_vector(name) flx_property<flxv_vector> name = flx_property<flxv_vector>(this, #name)
#define flxp_map(name) flx_property<flxv_map> name = flx_property<flxv_map>(this, #name)
#define flxp_model(name, model_type) flxp_map(name##_map); model_type name = flx_model::from_map<model_type>(name##_map)
#define flxp_model_list(name, model_type) flxp_vector(name##_vec); flx_model_list<model_type> name = flx_model_list<model_type>::from_vector(name##_vec)

#endif

#endif // flx_MODEL_H
