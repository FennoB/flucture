#ifndef flx_LAZY_PTR_H
#define flx_LAZY_PTR_H

#include <map>
#include <stdexcept>

/*
 * Lazy pointer with reference counting and on-demand creation.
 * Can hold a managed or unmanaged pointer.
 * Creates the object on first access if null.
 * Supports both const-correct and lazy-creation access patterns.
 */

// Exception for null access in const context
class flx_null_access_exception : public std::exception {
public:
    const char* what() const noexcept override {
        return "Access to null lazy pointer in const context";
    }
};

template <typename object_type>
class flx_lazy_ptr
{
  static std::map<object_type*, size_t> *refcounters;
  object_type *ptr;
  bool managed;
public:
  flx_lazy_ptr() : ptr(nullptr), managed(false){}
  flx_lazy_ptr(object_type* ptr, bool managed=false) : ptr(ptr), managed(managed)
  {
    if (managed)
    {
      manage();
    }
  }
  flx_lazy_ptr(const flx_lazy_ptr &other) : ptr(other.ptr), managed(false)
  {
    if (other.managed)
    {
      manage();
    }
  }
  void reset()
  {
    if (managed)
    {
      unmanage();
      if ((*refcounters)[ptr] == 0)
      {
        refcounters->erase(ptr);
        delete ptr;
      }
    }
    ptr = nullptr;
  }
  void manage()
  {
    if (!managed)
    {
      managed = true;
      if (!refcounters)
      {
        refcounters = new std::map<object_type*, size_t>;
      }
      ++(*refcounters)[ptr];
    }
  }
  void unmanage()
  {
    if (managed)
    {
      if (!refcounters)
      {
        refcounters = new std::map<object_type*, size_t>;
      }
      --(*refcounters)[ptr];
      managed = false;
    }
  }
  void set(object_type *ptr, bool managed=false)
  {
    if (ptr == this->ptr)
    {
      return;
    }
    reset();
    this->ptr = ptr;
    if (managed)
    {
      manage();
    }
  }
  ~flx_lazy_ptr()
  {
    reset();
  }
  
  // Non-const access - creates object if null
  virtual object_type &operator*()
  {
    if (ptr == nullptr)
    {
      ptr = new object_type;
      manage();
      on_create();
    }
    return *ptr;
  }
  
  // Const access - throws exception if null
  virtual const object_type &operator*() const
  {
    if (ptr == nullptr)
    {
      throw flx_null_access_exception();
    }
    return *ptr;
  }
  
  // Non-const pointer access
  object_type* operator->()
  {
    return &*(*this);
  }
  
  // Const pointer access - throws exception if null
  const object_type* operator->() const
  {
    return &*(*this);
  }
  
  flx_lazy_ptr &operator=(const flx_lazy_ptr &other)
  {
    reset();
    this->ptr = other.ptr;
    if (other.managed)
    {
      manage();
    }
    return *this;
  }
  flx_lazy_ptr &operator=(object_type *ptr)
  {
    set(ptr);
    return *this;
  }
  
  bool operator==(const flx_lazy_ptr &other) const
  {
    return ptr == other.ptr;
  }

  // Safe null check - doesn't throw, doesn't create
  bool is_null() const { return ptr == nullptr; }
  
  // This is nullptr if never accessed
  object_type* getptr() const { return ptr; }

  virtual void on_create() {}
};


// Allowed because its a template
template <typename object_type>
std::map<object_type*, size_t> *flx_lazy_ptr<object_type>::refcounters = 0;

#endif // flx_LAZY_PTR_H
