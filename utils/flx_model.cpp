#include "flx_model.h"
#include <iostream>

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
  std::cout << "[RESYNC] Called on model with " << children.size() << " children, " << model_lists.size() << " lists" << std::endl;

  // Iterate over children map and resync each child model
  for (auto& child_pair : children) {
    flx_model* child = child_pair.second;
    const flx_string& fieldname = child_pair.first;

    std::cout << "[RESYNC] Processing child: " << fieldname.c_str() << std::endl;

    // Check if this fieldname exists in our data
    if ((**this).find(fieldname) != (**this).end()) {
      flx_variant& child_data = (**this)[fieldname];

      std::cout << "[RESYNC] Found data for " << fieldname.c_str() << ", is_map=" << child_data.is_map() << std::endl;

      if (child_data.is_map()) {
        // Resync the child to point to this map
        child->set(&child_data.to_map());
        std::cout << "[RESYNC] Set child pointer for " << fieldname.c_str() << std::endl;

        // Recursively resync the child's children
        child->resync();
      }
    } else {
      std::cout << "[RESYNC] Data not found for " << fieldname.c_str() << std::endl;
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

void flx_model::apply_xml_map(const flxv_map& xml_map)
{
  // Iterate through all properties and map from XML using xml_path metadata
  for (const auto& prop_pair : props) {
    flx_property_i* prop = prop_pair.second;
    const flxv_map& meta = prop->get_meta();

    // Check if property has "xml_path" metadata
    if (meta.find("xml_path") != meta.end()) {
      flx_string xml_path = meta.at("xml_path").string_value();

      // Navigate the XML map using the path
      if (xml_path.contains("/")) {
        std::vector<flx_string> parts = xml_path.split("/");

        const flx_variant* current_var = nullptr;
        const flxv_map* current_map = &xml_map;

        // Navigate through the path
        bool path_valid = true;
        for (size_t i = 0; i < parts.size(); ++i) {
          if (current_map->find(parts[i]) == current_map->end()) {
            path_valid = false;
            break;
          }

          current_var = &current_map->at(parts[i]);

          // If not last part, must be a map to continue
          if (i < parts.size() - 1) {
            if (current_var->in_state() != flx_variant::map_state) {
              path_valid = false;
              break;
            }
            current_map = &current_var->map_value();
          }
        }

        // If we found the value, write it to the C++ property name
        if (path_valid && current_var != nullptr) {
          const flx_string& cpp_name = prop_pair.first;  // C++ property name
          (**this)[cpp_name] = *current_var;
        }
      } else {
        // Simple path without "/"
        if (xml_map.find(xml_path) != xml_map.end()) {
          const flx_string& cpp_name = prop_pair.first;
          (**this)[cpp_name] = xml_map.at(xml_path);
        }
      }
    }
  }

  // Recursively apply to nested models
  for (auto& child_pair : children) {
    flx_model* child = child_pair.second;
    const flx_string& cpp_name = child_pair.first;

    // Get the property to check its metadata
    if (props.find(cpp_name) != props.end()) {
      flx_property_i* prop = props.at(cpp_name);
      const flxv_map& meta = prop->get_meta();

      if (meta.find("xml_path") != meta.end()) {
        flx_string xml_path = meta.at("xml_path").string_value();

        // Navigate to the child's XML data
        const flx_variant* child_xml = nullptr;
        const flxv_map* current_map = &xml_map;
        bool path_valid = true;

        if (xml_path.contains("/")) {
          std::vector<flx_string> parts = xml_path.split("/");
          for (size_t i = 0; i < parts.size(); ++i) {
            if (current_map->find(parts[i]) == current_map->end()) {
              path_valid = false;
              break;
            }
            child_xml = &current_map->at(parts[i]);
            if (i < parts.size() - 1) {
              if (child_xml->in_state() != flx_variant::map_state) {
                path_valid = false;
                break;
              }
              current_map = &child_xml->map_value();
            }
          }
        } else {
          if (xml_map.find(xml_path) != xml_map.end()) {
            child_xml = &xml_map.at(xml_path);
          } else {
            path_valid = false;
          }
        }

        if (path_valid && child_xml && child_xml->in_state() == flx_variant::map_state) {
          child->apply_xml_map(child_xml->map_value());
        }
      }
    }
  }

  // Handle model_lists
  for (auto& list_pair : model_lists) {
    const flx_string& cpp_name = list_pair.first;

    // Get the property to check its metadata
    if (props.find(cpp_name) != props.end()) {
      flx_property_i* prop = props.at(cpp_name);
      const flxv_map& meta = prop->get_meta();

      if (meta.find("xml_path") != meta.end()) {
        flx_string xml_path = meta.at("xml_path").string_value();

        // Navigate to the list's XML data
        const flx_variant* list_xml = nullptr;
        const flxv_map* current_map = &xml_map;
        bool path_valid = true;

        if (xml_path.contains("/")) {
          std::vector<flx_string> parts = xml_path.split("/");
          for (size_t i = 0; i < parts.size(); ++i) {
            if (current_map->find(parts[i]) == current_map->end()) {
              path_valid = false;
              break;
            }
            list_xml = &current_map->at(parts[i]);
            if (i < parts.size() - 1) {
              if (list_xml->in_state() != flx_variant::map_state) {
                path_valid = false;
                break;
              }
              current_map = &list_xml->map_value();
            }
          }
        } else {
          if (xml_map.find(xml_path) != xml_map.end()) {
            list_xml = &xml_map.at(xml_path);
          } else {
            path_valid = false;
          }
        }

        if (path_valid && list_xml) {
          std::cout << "[DEBUG] Processing list: " << cpp_name.c_str() << ", state=" << list_xml->in_state() << std::endl;

          // Handle single element vs array
          if (list_xml->in_state() == flx_variant::map_state) {
            std::cout << "[DEBUG] Single element detected, creating vector" << std::endl;

            // Single element - convert to vector
            flxv_vector vec;
            vec.push_back(*list_xml);
            (**this)[cpp_name] = flx_variant(vec);
            std::cout << "[DEBUG] Assigned vector to model, resyncing list" << std::endl;

            // CRITICAL: Resync the list to point to the new vector data
            flx_list* list_ptr = list_pair.second;
            list_ptr->resync();
            std::cout << "[DEBUG] List resynced, calling apply_xml_map_to_all" << std::endl;

            list_ptr->apply_xml_map_to_all();
            std::cout << "[DEBUG] apply_xml_map_to_all completed" << std::endl;
          } else if (list_xml->in_state() == flx_variant::vector_state) {
            std::cout << "[DEBUG] Vector state detected, using as-is" << std::endl;

            // Already a vector - use as-is
            (**this)[cpp_name] = *list_xml;

            flx_list* list_ptr = list_pair.second;
            list_ptr->resync();
            list_ptr->apply_xml_map_to_all();
          } else {
            std::cout << "[DEBUG] Unknown state, storing as-is" << std::endl;
            // Unknown state - store as-is but don't process
            (**this)[cpp_name] = *list_xml;
          }
        }
      }
    }
  }
}

#endif
