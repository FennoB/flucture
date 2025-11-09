#include "flx_xml.h"
#include <pugixml.hpp>
#include <iostream>
#include <sstream>

namespace {

  // Konvertiert einen pugixml Node in ein flx_variant
  flx_variant pugi_to_flx(const pugi::xml_node& node) {
    // Leerer Node → null variant
    if (!node) {
      return flx_variant();
    }

    // Prüfe ob Node Element-Children hat
    bool has_element_children = false;
    for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
      if (child.type() == pugi::node_element) {
        has_element_children = true;
        break;
      }
    }

    // Wenn der Node nur Text enthält (keine Element-Children und keine Attribute)
    if (!has_element_children && !node.first_attribute()) {
      flx_string text = node.text().as_string();
      if (text.empty()) {
        return flx_variant();
      }

      // Versuche Typ-Erkennung
      if (text == "true" || text == "false") {
        return flx_variant(text == "true");
      }

      // Check for date/time patterns before numeric conversion
      // ISO 8601 dates: "2025-01-17", "2025-01-17+01:00", "2025-01-17T14:30:00"
      // Times: "11:00:00", "11:00:00+01:00"
      if (text.contains("-") || text.contains(":")) {
        // Keep as string - could be date/time
        return flx_variant(text);
      }

      // Erst auf Double prüfen (enthält "."), dann auf Integer
      if (text.contains(".") && text.is_double()) {
        return flx_variant(text.to_double(0.0));
      }
      if (text.is_integer()) {
        return flx_variant(static_cast<long long>(text.to_int(0)));
      }
      return flx_variant(text);
    }

    // Node mit Kindern oder Attributen → Map
    flxv_map element_map;

    // Attribute mit "@" Prefix speichern
    for (pugi::xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute()) {
      flx_string key = flx_string("@") + attr.name();
      flx_string value = attr.value();

      // Typ-Erkennung für Attributwerte
      if (value == "true" || value == "false") {
        element_map[key] = flx_variant(value == "true");
      } else if (value.is_integer()) {
        element_map[key] = flx_variant(static_cast<long long>(value.to_int(0)));
      } else if (value.is_double()) {
        element_map[key] = flx_variant(value.to_double(0.0));
      } else {
        element_map[key] = flx_variant(value);
      }
    }

    // Text-Content speichern
    flx_string text_content = node.text().as_string();
    text_content = text_content.trim();  // Trim whitespace from text content
    if (!text_content.empty()) {
      element_map[flx_string("#text")] = flx_variant(text_content);
    }

    // Child-Elemente verarbeiten
    for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
      // Nur Element-Nodes verarbeiten, keine Text-Nodes
      if (child.type() != pugi::node_element) {
        continue;
      }

      flx_string child_name = child.name();
      flx_variant child_value = pugi_to_flx(child);

      // Wenn bereits ein Element mit diesem Namen existiert, zu einem Array konvertieren
      if (element_map.find(child_name) != element_map.end()) {
        // Bereits existiert - prüfen ob schon Array
        if (element_map[child_name].in_state() == flx_variant::vector_state) {
          // Schon ein Array - hinzufügen
          flxv_vector& vec = const_cast<flxv_vector&>(element_map[child_name].vector_value());
          vec.push_back(child_value);
        } else {
          // Noch kein Array - in Array konvertieren
          flx_variant existing = element_map[child_name];
          flxv_vector vec;
          vec.push_back(existing);
          vec.push_back(child_value);
          element_map[child_name] = flx_variant(vec);
        }
      } else {
        // Erstes Element mit diesem Namen
        element_map[child_name] = child_value;
      }
    }

    return flx_variant(element_map);
  }

  // Konvertiert ein flx_variant in einen pugixml Node
  void flx_to_pugi(const flx_variant& var, pugi::xml_node& node) {
    switch (var.in_state()) {
      case flx_variant::string_state:
        node.text().set(var.string_value().c_str());
        break;

      case flx_variant::int_state:
        node.text().set(var.int_value());
        break;

      case flx_variant::bool_state:
        node.text().set(var.bool_value());
        break;

      case flx_variant::double_state:
        node.text().set(var.double_value());
        break;

      case flx_variant::vector_state: {
        // Arrays als mehrere gleichnamige Child-Elemente
        const flxv_vector& vec = var.vector_value();
        for (const auto& item : vec) {
          pugi::xml_node child = node.append_child("item");
          flx_to_pugi(item, child);
        }
        break;
      }

      case flx_variant::map_state: {
        const flxv_map& map_val = var.map_value();

        for (const auto& pair : map_val) {
          flx_string key = pair.first;

          // Attribut (beginnt mit @)
          if (key.starts_with("@")) {
            flx_string attr_name = key.to_std().substr(1);

            if (pair.second.in_state() == flx_variant::string_state) {
              node.append_attribute(attr_name.c_str()).set_value(pair.second.string_value().c_str());
            } else if (pair.second.in_state() == flx_variant::int_state) {
              node.append_attribute(attr_name.c_str()).set_value(pair.second.int_value());
            } else if (pair.second.in_state() == flx_variant::bool_state) {
              node.append_attribute(attr_name.c_str()).set_value(pair.second.bool_value());
            } else if (pair.second.in_state() == flx_variant::double_state) {
              node.append_attribute(attr_name.c_str()).set_value(pair.second.double_value());
            }
          }
          // Text-Content
          else if (key == "#text") {
            if (pair.second.in_state() == flx_variant::string_state) {
              node.text().set(pair.second.string_value().c_str());
            }
          }
          // Array von Child-Elementen
          else if (pair.second.in_state() == flx_variant::vector_state) {
            const flxv_vector& vec = pair.second.vector_value();
            for (const auto& item : vec) {
              pugi::xml_node child = node.append_child(key.c_str());
              flx_to_pugi(item, child);
            }
          }
          // Child-Element
          else {
            pugi::xml_node child = node.append_child(key.c_str());
            flx_to_pugi(pair.second, child);
          }
        }
        break;
      }

      case flx_variant::none:
      default:
        // Null-Werte werden als leere Nodes belassen
        break;
    }
  }

} // Ende anonymer Namespace

flx_xml::flx_xml(flxv_map* map_ptr) : data_map(map_ptr) {
  if (!data_map) {
    std::cerr << "Error: flx_xml constructor received a nullptr for data_map." << std::endl;
  }
}

bool flx_xml::parse(const flx_string& xml_string) {
  if (!data_map) {
    std::cerr << "Error: flx_xml::parse called on a null data_map." << std::endl;
    return false;
  }

  data_map->clear();

  try {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml_string.c_str());

    if (!result) {
      std::cerr << "XML parse error: " << result.description()
                << " at offset " << result.offset << std::endl;
      return false;
    }

    // Root-Element verarbeiten
    pugi::xml_node root = doc.first_child();
    if (!root) {
      std::cerr << "Error: No root element found in XML." << std::endl;
      return false;
    }

    // Root-Element in die Map konvertieren
    flx_string root_name = root.name();
    (*data_map)[root_name] = pugi_to_flx(root);

    return true;

  } catch (const std::exception& e) {
    std::cerr << "An unexpected error occurred during XML parsing: " << e.what() << std::endl;
    return false;
  }
}

flx_string flx_xml::create() const {
  if (!data_map) {
    std::cerr << "Error: flx_xml::create called on a null data_map." << std::endl;
    return flx_string("");
  }

  if (data_map->empty()) {
    std::cerr << "Error: Cannot create XML from empty map." << std::endl;
    return flx_string("");
  }

  try {
    pugi::xml_document doc;

    // Erwarten genau ein Root-Element in der Map
    if (data_map->size() != 1) {
      std::cerr << "Warning: XML map should contain exactly one root element. Found "
                << data_map->size() << " elements." << std::endl;
    }

    // Erstes Element als Root verwenden
    const auto& root_pair = *data_map->begin();
    pugi::xml_node root = doc.append_child(root_pair.first.c_str());
    flx_to_pugi(root_pair.second, root);

    // XML in String serialisieren
    std::ostringstream oss;
    doc.save(oss, "  ", pugi::format_default, pugi::encoding_utf8);

    return flx_string(oss.str());

  } catch (const std::exception& e) {
    std::cerr << "An unexpected error occurred during XML creation: " << e.what() << std::endl;
    return flx_string("");
  }
}

namespace {
  // Helper: Navigate one step in path (map[key])
  const flx_variant* navigate_map_key(const flx_variant* current, const flx_string& key) {
    if (!current || current->in_state() != flx_variant::map_state) {
      return nullptr;
    }
    const flxv_map& map = current->map_value();
    if (map.find(key) == map.end()) {
      return nullptr;
    }
    return &map.at(key);
  }

  // Helper: Navigate array access (vec[index])
  const flx_variant* navigate_array_index(const flx_variant* current, size_t index) {
    if (!current || current->in_state() != flx_variant::vector_state) {
      return nullptr;
    }
    const flxv_vector& vec = current->vector_value();
    if (index >= vec.size()) {
      return nullptr;
    }
    return &vec[index];
  }
}

const flx_variant* flx_xml::read_path(const flx_string& path) const {
  if (!data_map || path.empty()) {
    return nullptr;
  }

  std::vector<flx_string> parts = path.split("/");
  const flx_variant* current = nullptr;

  for (const auto& part : parts) {
    // Erste Iteration: Von root map starten
    if (current == nullptr) {
      if (data_map->find(part) == data_map->end()) {
        return nullptr;
      }
      current = &data_map->at(part);
      continue;
    }

    // Check für Array-Index: "member[0]"
    if (part.contains("[") && part.contains("]")) {
      size_t bracket_pos = part.find("[");
      flx_string name = part.substr(0, bracket_pos);
      flx_string index_str = part.substr(bracket_pos + 1, part.find("]") - bracket_pos - 1);
      size_t index = static_cast<size_t>(index_str.to_int(0));

      current = navigate_map_key(current, name);
      if (!current) return nullptr;

      current = navigate_array_index(current, index);
      if (!current) return nullptr;
    } else {
      current = navigate_map_key(current, part);
      if (!current) return nullptr;
    }
  }

  return current;
}

bool flx_xml::has_placeholder(const flx_string& path) {
  return path.contains("[]");
}

flx_string flx_xml::replace_first_placeholder(const flx_string& path, size_t index) {
  size_t pos = path.find("[]");
  if (pos == flx_string::npos) {
    return path;
  }

  flx_string result = path;
  flx_string index_str = flx_string("[") + flx_string(static_cast<long>(index)) + "]";
  result = result.substr(0, pos) + index_str + result.substr(pos + 2);
  return result;
}

flx_string flx_xml::remove_first_placeholder(const flx_string& path) {
  size_t pos = path.find("[]");
  if (pos == flx_string::npos) {
    return path;
  }

  return path.substr(0, pos) + path.substr(pos + 2);
}
