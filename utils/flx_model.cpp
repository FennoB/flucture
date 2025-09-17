#include "flx_model.h"

#ifndef flx_variant_models
#else
flx_property_i::flx_property_i(flx_model *parent, const flx_string &name) : parent(parent), name(name)
{
  parent->add_prop(this, name);
}

flx_variant &flx_property_i::access()
{
  return (**parent)[name];
}

const flx_variant &flx_property_i::const_access() const
{
  return (**parent)[name];
}

bool flx_property_i::is_null() const
{
  try {
    const_access();
    return const_access().in_state() == flx_variant::none;
  } catch(const flx_null_access_exception&) {
    // Parent object itself is null
    return true;
  } catch(const flx_null_field_exception&) {
    // Field is null
    return true;
  }
}


flx_model::flx_model() : parent_property(nullptr)
{
}

void flx_model::add_prop(flx_property_i *prop, const flx_string &name)
{
  props[name] = prop;
}

void flx_model::set_parent(flx_property<flxv_map>* parent_prop)
{
  parent_property = parent_prop;
}

flxv_map &flx_model::operator*()
{
  if (parent_property != nullptr)
  {
    this->set(&parent_property->value());
  }
  return flx_lazy_ptr<flxv_map>::operator*();
}

const flxv_map &flx_model::operator*() const
{
  return flx_lazy_ptr<flxv_map>::operator*();
}

#endif
