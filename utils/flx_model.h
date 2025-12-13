#ifndef flx_MODEL_H
#define flx_MODEL_H

#include "flx_lazy_ptr.h"
#include "flx_variant.h"
#include <type_traits>

// Forward declarations
class flx_xml;

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
  flxv_map metadata;
public:
  flx_property_i(flx_model* parent, const flx_string &name, const flxv_map& meta = flxv_map());

  flx_string prop_name() const { return name; }

  // Parent access (for move constructors)
  flx_model* get_parent() const { return parent; }

  // Update parent pointer after move/copy
  virtual void update_parent(flx_model* new_parent) { parent = new_parent; }

  // Metadata access
  const flxv_map& get_meta() const { return metadata; }
  flxv_map& get_meta() { return metadata; }

  // Get the expected variant type for this property
  virtual flx_variant::state get_variant_type() const = 0;

  // Check if this property is a model_list (relation)
  virtual bool is_relation() const { return false; }

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
  flx_property(flx_model* parent, const flx_string &name, const flxv_map& meta = flxv_map())
    : flx_property_i(parent, name, meta)
  {
    flx_variant v = type{};
    variant_type = v.in_state();
  }

  // Copy constructor - needed for Model copy
  flx_property(const flx_property& other)
    : flx_property_i(other.parent, other.name, other.metadata)
    , variant_type(other.variant_type)
  {
  }

  // Move constructor - needed for from_vector() return
  flx_property(flx_property&& other) noexcept
    : flx_property_i(other.parent, other.name, other.metadata)
    , variant_type(other.variant_type)
  {
  }

  // Copy assignment - needed for Model assignment
  flx_property& operator=(const flx_property& other)
  {
    // Don't copy parent/name/metadata - those are structural
    variant_type = other.variant_type;
    return *this;
  }

  // Move assignment
  flx_property& operator=(flx_property&& other) noexcept
  {
    variant_type = other.variant_type;
    return *this;
  }

  // Return the expected variant type
  virtual flx_variant::state get_variant_type() const override {
    return variant_type;
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

// Forward declaration
class flx_model;

// Base interface for model lists - enforces resync implementation
class flx_list {
public:
  virtual ~flx_list() = default;
  virtual size_t list_size() = 0;
  virtual flx_model* get_model_at(size_t index) = 0;  // Returns pointer to model at index
  virtual void resync() = 0;  // Must be implemented by derived classes
  virtual flx_lazy_ptr<flx_model> factory() = 0;  // Create empty model for metadata inspection
  virtual void clear() = 0;  // Clear all elements
  virtual void add_element() = 0;  // Add new empty element
  virtual flx_model& back() = 0;  // Access last element
};

// Abstract base class for models - enforces resync implementation
class flx_model : public flx_lazy_ptr<flxv_map>
{
  std::map<flx_string, flx_property_i*> props;
  std::map<flx_string, flx_model*> children;
  std::map<flx_string, flx_list*> model_lists;
  flx_property<flxv_map>* parent_property;
public:
  flx_model();
  virtual ~flx_model() = default;

  void add_prop(flx_property_i* prop, const flx_string &name);
  void add_child(const flx_string& name, flx_model* child);
  void add_model_list(const flx_string& name, flx_list* list);
  void set_parent(flx_property<flxv_map>* parent_prop);

  // Update registration after move (called by move constructors)
  void update_child_registration(const flx_string& name, flx_model* child);
  void update_model_list_registration(const flx_string& name, flx_list* list);

  // Remove registration (called by move assignment operators)
  void remove_child_registration(const flx_string& name);
  void remove_model_list_registration(const flx_string& name);

  // Access to properties for metadata introspection
  const std::map<flx_string, flx_property_i*>& get_properties() const { return props; }

  // Access to child models for nested model introspection
  const std::map<flx_string, flx_model*>& get_children() const { return children; }

  // Access to model lists for nested model list introspection
  const std::map<flx_string, flx_list*>& get_model_lists() const { return model_lists; }

  // Resync implementation - can be overridden by derived classes
  virtual void resync();

  // Pull data from DB row - reads properties with {"column", "name"} metadata
  void read_row(const flxv_map& row);

  // Read XML data into model using absolute paths with [] placeholders
  void read_xml(flx_xml& xml, const flx_string& base_path);

private:
  // Single-responsibility helpers for read_xml (each <20 lines)
  void read_property(flx_xml& xml, const flx_string& cpp_name,
                     const flx_string& xml_path, const flx_string& base_path);
  bool try_read_property(flx_xml& xml, const flx_string& cpp_name,
                         const flx_string& full_path);
  void read_primitive_property(const flx_string& cpp_name, const flx_variant* value);
  bool read_list_property(flx_xml& xml, const flx_string& cpp_name,
                          flx_list* list_ptr, const flx_string& full_path);

public:
  // Override to sync with parent before access
  virtual flxv_map &operator*() override;
  virtual const flxv_map &operator*() const override;
  flx_variant& operator[](const flx_string &key)
  {
    return (**this)[key];
  }

  // Clear the model
  void clear()
  {
    (**this).clear();
  }
  // copy constructor
  flx_model(flx_model &other) : flx_lazy_ptr<flxv_map>(other)
  {
    **this = *other;
  }
  // const copy constructor - Properties copied, need parent pointer update
  flx_model(const flx_model &other)
    : flx_lazy_ptr<flxv_map>()
    , props()  // Properties will be copied via default, then we update parents
    , children()
    , model_lists()
    , parent_property(nullptr)
  {
    // Copy underlying data map
    try {
      **this = *other;
    } catch (...) {
    }

    // CRITICAL: Properties have been copied and their parent pointers point to OTHER
    // Update all property parent pointers to THIS
    for (auto& prop_pair : props) {
      if (prop_pair.second) {
        prop_pair.second->update_parent(this);
      }
    }
  }

  // Move constructor - Properties moved, need parent pointer update
  flx_model(flx_model&& other) noexcept
    : flx_lazy_ptr<flxv_map>(other)
    , props()  // Properties will be moved via default, then we update parents
    , children(std::move(other.children))
    , model_lists(std::move(other.model_lists))
    , parent_property(other.parent_property)
  {
    // Update parent's children map to point to NEW location
    if (parent_property && parent_property->get_parent()) {
      parent_property->get_parent()->update_child_registration(parent_property->prop_name(), this);
    }

    // CRITICAL: Properties have been moved and their parent pointers point to OLD address
    // Update all property parent pointers to THIS
    for (auto& prop_pair : props) {
      if (prop_pair.second) {
        prop_pair.second->update_parent(this);
      }
    }

    other.parent_property = nullptr;
  }

  // Copy assignment
  flx_model& operator=(flx_model &other)
  {
    **this = *other;
    return *this;
  }

  // Move assignment - deregisters old, registers new
  flx_model& operator=(flx_model&& other) noexcept
  {
    if (this != &other) {
      // Deregister from OLD parent (this might be under different name)
      if (parent_property && parent_property->get_parent()) {
        parent_property->get_parent()->remove_child_registration(parent_property->prop_name());
      }

      // Move data and children, but NOT props (props are structural members)
      flx_lazy_ptr<flxv_map>::operator=(other);
      // props stays unchanged - each instance keeps its own properties
      children = std::move(other.children);
      model_lists = std::move(other.model_lists);
      parent_property = other.parent_property;

      // Register at NEW parent
      if (parent_property && parent_property->get_parent()) {
        parent_property->get_parent()->update_child_registration(parent_property->prop_name(), this);
      }

      other.parent_property = nullptr;
    }
    return *this;
  }
  template<typename T = flx_model>
  static T from_map(flx_property<flxv_map>& map_prop, flx_model* parent = nullptr, const flx_string& child_name = flx_string())
  {
    T model;
    model.set(&map_prop.value());
    model.set_parent(&map_prop);

    // Register as child if parent specified
    if (parent != nullptr && !child_name.empty()) {
      parent->add_child(child_name, &model);
    }

    return model;
  }
};

template <typename model>
class flx_model_list : public flx_lazy_ptr<flxv_vector>, public flx_list
{
  std::map<size_t, model> cache;
  flx_property<flxv_vector>* parent_property;
public:
  flx_model_list() : flx_lazy_ptr<flxv_vector>(), parent_property(nullptr)
  {
  }

  // Copy constructor - needed for from_vector() return
  flx_model_list(const flx_model_list& other)
    : flx_lazy_ptr<flxv_vector>()
    , parent_property(nullptr)
  {
    try {
      **this = *other;
    } catch (...) {
    }
  }

  // Move constructor - updates parent registration
  flx_model_list(flx_model_list&& other) noexcept
    : flx_lazy_ptr<flxv_vector>(other)
    , cache(std::move(other.cache))
    , parent_property(other.parent_property)
  {
    if (parent_property && parent_property->get_parent()) {
      parent_property->get_parent()->update_model_list_registration(parent_property->prop_name(), this);
    }
    other.parent_property = nullptr;
  }

  // Move assignment
  flx_model_list& operator=(flx_model_list&& other) noexcept
  {
    if (this != &other) {
      if (parent_property && parent_property->get_parent()) {
        parent_property->get_parent()->remove_model_list_registration(parent_property->prop_name());
      }
      flx_lazy_ptr<flxv_vector>::operator=(other);
      cache = std::move(other.cache);
      parent_property = other.parent_property;
      if (parent_property && parent_property->get_parent()) {
        parent_property->get_parent()->update_model_list_registration(parent_property->prop_name(), this);
      }
      other.parent_property = nullptr;
    }
    return *this;
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
  void add_element() override
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

  model& back() override
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

  // Interface implementation for reflection
  virtual size_t list_size() override
  {
    return size();
  }

  virtual flx_model* get_model_at(size_t index) override
  {
    return &at(index);  // Returns pointer to cached model
  }

  // Resync implementation (implements flx_list)
  virtual void resync() override
  {
    // Clear cache to force reload from underlying vector
    cache.clear();

    // Resync each cached model
    for (size_t i = 0; i < size(); i++) {
      model& m = at(i);  // This will populate cache
      m.resync();  // Recursively resync the model
    }
  }

  // Factory implementation (implements flx_list)
  virtual flx_lazy_ptr<flx_model> factory() override
  {
    // Create managed model (allocated on heap, owned by lazy_ptr)
    flx_lazy_ptr<flx_model> ptr;
    ptr.set(new model(), true);  // managed=true
    return ptr;
  }

  // Pull data from DB row - adds new element and reads its data
  void read_row(const flxv_map& row)
  {
    add_element();
    back().read_row(row);
  }

  model& operator[](size_t index)
  {
    return at(index);
  }

  // Clear the list
  void clear() override
  {
    (**this).clear();
    cache.clear();
  }

  // Remove last element
  void pop_back()
  {
    if (size() > 0) {
      size_t last_index = size() - 1;
      (**this).pop_back();
      cache.erase(last_index);
    }
  }

  // Copy assignment operator
  flx_model_list& operator=(const flx_model_list& other)
  {
    if (this != &other) {
      try {
        **this = *other;
      } catch (const flx_null_access_exception&) {
        // Clear our own vector if other is null
        (**this).clear();
      }
      cache.clear();
    }
    return *this;
  }

  static flx_model_list from_vector(flx_property<flxv_vector>& vec_prop, flx_model* parent = nullptr, const flx_string& name = flx_string())
  {
    flx_model_list<model> list;
    list.set(&vec_prop.value());
    list.set_parent(&vec_prop);

    // Register with parent if provided
    if (parent != nullptr && !name.empty()) {
      parent->add_model_list(name, &list);
    }

    return list;
  }
};

// Property macros with optional metadata
#define flxp_int(name, ...) flx_property<flxv_int> name = flx_property<flxv_int>(this, #name, ##__VA_ARGS__)
#define flxp_string(name, ...) flx_property<flxv_string> name = flx_property<flxv_string>(this, #name, ##__VA_ARGS__)
#define flxp_bool(name, ...) flx_property<flxv_bool> name = flx_property<flxv_bool>(this, #name, ##__VA_ARGS__)
#define flxp_double(name, ...) flx_property<flxv_double> name = flx_property<flxv_double>(this, #name, ##__VA_ARGS__)
#define flxp_vector(name, ...) flx_property<flxv_vector> name = flx_property<flxv_vector>(this, #name, ##__VA_ARGS__)
#define flxp_map(name, ...) flx_property<flxv_map> name = flx_property<flxv_map>(this, #name, ##__VA_ARGS__)
#define flxp_model(name, model_type, ...) \
  flx_property<flxv_map> name##_map = flx_property<flxv_map>(this, #name, ##__VA_ARGS__); \
  model_type name = flx_model::from_map<model_type>(name##_map, this, name##_map.prop_name())
#define flxp_model_list(name, model_type, ...) \
  flx_property<flxv_vector> name##_vec = flx_property<flxv_vector>(this, #name, ##__VA_ARGS__); \
  flx_model_list<model_type> name = flx_model_list<model_type>::from_vector(name##_vec, this, name##_vec.prop_name())

#endif

#endif // flx_MODEL_H
