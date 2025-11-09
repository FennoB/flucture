#include "flx_model.h"

#ifndef flx_variant_models
#else
flx_property_i::flx_property_i(flx_model *parent, const flx_string &name, const flxv_map& meta)
  : parent(parent), name(name), metadata(meta)
{
  // Override name if "fieldname" exists in metadata
  if (metadata.find("fieldname") != metadata.end()) {
    this->name = metadata.at("fieldname").string_value();
  }
  parent->add_prop(this, this->name);
}

flx_variant &flx_property_i::access()
{
  // Check if name contains "/" for nested access
  if (name.contains("/")) {
    std::vector<flx_string> parts = name.split("/");

    // Navigate through nested maps
    flx_variant* current_var = &(**parent)[parts[0]];

    for (size_t i = 1; i < parts.size(); ++i) {
      // Ensure current level is a map
      if (current_var->in_state() != flx_variant::map_state) {
        *current_var = flxv_map(); // Create empty map if needed
      }

      flxv_map& current_map = current_var->to_map();
      current_var = &current_map[parts[i]];
    }

    return *current_var;
  }

  return (**parent)[name];
}

const flx_variant &flx_property_i::const_access() const
{
  // Check if name contains "/" for nested access
  if (name.contains("/")) {
    std::vector<flx_string> parts = name.split("/");

    // Navigate through nested maps (const version)
    const flxv_map& root_map = **parent;
    if (root_map.find(parts[0]) == root_map.end()) {
      throw flx_null_field_exception(name);
    }

    const flx_variant* current_var = &root_map.at(parts[0]);

    for (size_t i = 1; i < parts.size(); ++i) {
      if (current_var->in_state() != flx_variant::map_state) {
        throw flx_null_field_exception(name);
      }

      const flxv_map& current_map = current_var->map_value();

      if (current_map.find(parts[i]) == current_map.end()) {
        throw flx_null_field_exception(name);
      }

      current_var = &current_map.at(parts[i]);
    }

    return *current_var;
  }

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

void flx_model::add_child(const flx_string& name, flx_model* child)
{
  children[name] = child;
}

void flx_model::add_model_list(const flx_string& name, flx_list* list)
{
  model_lists[name] = list;
}

void flx_model::set_parent(flx_property<flxv_map>* parent_prop)
{
  parent_property = parent_prop;
}

void flx_model::update_child_registration(const flx_string& name, flx_model* child)
{
  children[name] = child;
}

void flx_model::update_model_list_registration(const flx_string& name, flx_list* list)
{
  model_lists[name] = list;
}

void flx_model::remove_child_registration(const flx_string& name)
{
  children.erase(name);
}

void flx_model::remove_model_list_registration(const flx_string& name)
{
  model_lists.erase(name);
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

void flx_model::resync()
{
  // Iterate over children map and resync each child model
  for (auto& child_pair : children) {
    flx_model* child = child_pair.second;
    const flx_string& fieldname = child_pair.first;

    // Check if this fieldname exists in our data
    if ((**this).find(fieldname) != (**this).end()) {
      flx_variant& child_data = (**this)[fieldname];

      if (child_data.is_map()) {
        // Resync the child to point to this map
        child->set(&child_data.to_map());
        // Recursively resync the child's children
        child->resync();
      }
    }
  }

  // Iterate over model_lists and resync each list
  for (auto& list_pair : model_lists) {
    flx_list* list = list_pair.second;
    const flx_string& fieldname = list_pair.first;

    // Check if this fieldname exists in our data
    if ((**this).find(fieldname) != (**this).end()) {
      flx_variant& list_data = (**this)[fieldname];

      if (list_data.is_vector()) {
        // Call resync on the list interface
        list->resync();
      }
    }
  }
}

void flx_model::read_row(const flxv_map& row)
{
  // Iterate through all properties and pull data for those with "column" metadata
  for (const auto& prop_pair : props) {
    flx_property_i* prop = prop_pair.second;
    const flxv_map& meta = prop->get_meta();

    // Check if property has "column" metadata
    if (meta.find("column") != meta.end()) {
      flx_string column_name = meta.at("column").string_value();

      // Check if row has this column
      if (row.find(column_name) != row.end()) {
        const flx_variant& value = row.at(column_name);

        // Property name is the fieldname (or C++ name if no fieldname)
        const flx_string& prop_name = prop->prop_name();

        // If fieldname contains "/", create nested structure for path navigation
        if (prop_name.contains("/")) {
          std::vector<flx_string> parts = prop_name.split("/");

          // Navigate/create nested maps to the parent of the final key
          flxv_map* current_map = &(**this);

          for (size_t i = 0; i < parts.size() - 1; ++i) {
            flx_variant& var = (*current_map)[parts[i]];

            // Ensure this level is a map
            if (var.in_state() != flx_variant::map_state) {
              var = flxv_map();
            }

            current_map = &var.to_map();
          }

          // Set the final value at the last key
          (*current_map)[parts[parts.size() - 1]] = value;
        } else {
          // Simple flat access for properties without path
          (**this)[prop_name] = value;
        }
      }
    }
  }
}

#endif
