#include "flx_json.h"
#include "json.hpp" // Die nlohmann/json Bibliothek
#include <iostream> // F�r Fehler-Logging (optional)

// Anonymer Namespace f�r interne Hilfsfunktionen
namespace {

  // Konvertiert ein nlohmann::json Objekt in ein flx_variant
  flx_variant nlohmann_to_flx(const nlohmann::json& j_val) {
    if (j_val.is_null()) {
      return flx_variant(); // Erzeugt einen 'none' state flx_variant
    }
    if (j_val.is_boolean()) {
      return flx_variant(j_val.get<bool>());
    }
    if (j_val.is_number_integer()) {
      // nlohmann::json kann int64_t speichern. flx_variant verwendet long.
      // Dies ist normalerweise sicher auf den meisten modernen Systemen (long oft 64-bit).
      return flx_variant((long long)j_val.get<long>());
    }
    if (j_val.is_number_unsigned()) {
      // Konvertierung von unsigned zu signed long.
      // Vorsicht: Potenzieller �berlauf, wenn der unsigned Wert zu gro� f�r long ist.
      // Eine robustere L�sung k�nnte hier pr�fen oder eine Ausnahme ausl�sen.
      unsigned long u_val = j_val.get<unsigned long>();
      if (u_val > static_cast<unsigned long>(std::numeric_limits<long>::max())) {
        // Fehlerbehandlung f�r �berlauf, z.B. als double speichern oder Fehler werfen
        // Hier vereinfacht: Konvertierung zu double, wenn zu gro� f�r long
        // Alternativ: throw std::overflow_error("Unsigned value too large for long");
        std::cerr << "Warning: Unsigned JSON number " << u_val << " too large for long, converting to double." << std::endl;
        return flx_variant(static_cast<double>(u_val));
      }
      return flx_variant(static_cast<long long>(u_val));
    }
    if (j_val.is_number_float()) {
      return flx_variant(j_val.get<double>());
    }
    if (j_val.is_string()) {
      return flx_variant(flx_string(j_val.get<std::string>()));
    }
    if (j_val.is_array()) {
      flxv_vector vec;
      vec.reserve(j_val.size());
      for (const auto& el : j_val) {
        vec.push_back(nlohmann_to_flx(el));
      }
      return flx_variant(vec);
    }
    if (j_val.is_object()) {
      flxv_map map_val;
      for (auto it = j_val.items().begin(); it != j_val.items().end(); ++it) {
        map_val[flx_string(it.key())] = nlohmann_to_flx(it.value());
      }
      return flx_variant(map_val);
    }
    // Fallback f�r unbekannte oder nicht unterst�tzte Typen
    std::cerr << "Warning: Unknown nlohmann::json type encountered during conversion." << std::endl;
    return flx_variant();
  }

  // Konvertiert ein flx_variant Objekt in ein nlohmann::json Objekt
  nlohmann::json flx_to_nlohmann(const flx_variant& var) {
    switch (var.in_state()) {
      case flx_variant::string_state:
        // Annahme: flx_string::string_value() gibt const flx_string& zur�ck,
        // und flx_string hat eine Methode to_std(), die std::string zur�ckgibt.
        return var.string_value().to_std_const();
      case flx_variant::int_state:
        return var.int_value();
      case flx_variant::bool_state:
        return var.bool_value();
      case flx_variant::double_state:
        return var.double_value();
      case flx_variant::vector_state: {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& el : var.vector_value()) {
          arr.push_back(flx_to_nlohmann(el));
        }
        return arr;
      }
      case flx_variant::map_state: {
        nlohmann::json obj = nlohmann::json::object();
        for (const auto& pair : var.map_value()) {
          // Annahme: pair.first ist flx_string und hat to_std()
          obj[pair.first.to_std_const()] = flx_to_nlohmann(pair.second);
        }
        return obj;
      }
      case flx_variant::none:
      default:
        return nullptr; // nlohmann::json behandelt nullptr als JSON null
    }
  }

} // Ende anonymer Namespace

flx_json::flx_json(flxv_map* map_ptr) : data_map(map_ptr) {
  if (!data_map) {
    // Es wird dringend empfohlen, hier einen Fehler zu behandeln,
    // z.B. eine std::invalid_argument Ausnahme auszul�sen.
    // throw std::invalid_argument("flx_json constructor received a nullptr for data_map");
    std::cerr << "Error: flx_json constructor received a nullptr for data_map." << std::endl;
  }
}

bool flx_json::parse(const flx_string& json_string) {
  if (!data_map) {
    std::cerr << "Error: flx_json::parse called on a null data_map." << std::endl;
    return false;
  }

  data_map->clear(); // Bestehende Daten in der Map l�schen

  try {
    // Parsen des JSON-Strings mit nlohmann::json
    nlohmann::json parsed_json = nlohmann::json::parse(json_string.to_std_const());

           // �berpr�fen, ob das oberste Element ein Objekt ist, da data_map eine Map ist.
    if (parsed_json.is_object()) {
      for (auto it = parsed_json.items().begin(); it != parsed_json.items().end(); ++it) {
        // Konvertieren jedes Elements des nlohmann::json-Objekts in flx_variant
        // und Einf�gen in die data_map.
        (*data_map)[flx_string(it.key())] = nlohmann_to_flx(it.value());
      }
      return true;
    }

    // Falls das oberste JSON-Element kein Objekt ist, ist dies ein Fehler f�r diese Implementierung.
    std::cerr << "Error: JSON string does not represent an object at the top level." << std::endl;
    return false;

  } catch (const nlohmann::json::parse_error& e) {
    // Fehler beim Parsen des JSON-Strings (z.B. ung�ltiges Format).
    std::cerr << "JSON parse error: " << e.what()
              << " at byte " << e.byte << std::endl;
    return false;
  } catch (const std::exception& e) {
    // Andere m�gliche Fehler w�hrend der Konvertierung oder des Parsens.
    std::cerr << "An unexpected error occurred during JSON parsing: " << e.what() << std::endl;
    return false;
  }
}

flx_string flx_json::create() const {
  if (!data_map) {
    std::cerr << "Error: flx_json::create called on a null data_map." << std::endl;
    return flx_string(""); // Keine Daten zum Serialisieren
  }

         // Erstellen eines nlohmann::json Objekts aus der data_map.
  nlohmann::json j_obj = nlohmann::json::object();
  for (const auto& pair : *data_map) {
    j_obj[pair.first.to_std_const()] = flx_to_nlohmann(pair.second);
  }

  try {
    // Serialisieren des nlohmann::json Objekts in einen String.
    return flx_string(j_obj.dump());
  } catch (const nlohmann::json::type_error& e) {
    // Fehler bei der Serialisierung aufgrund von Typ-Problemen.
    std::cerr << "JSON dump type error: " << e.what() << std::endl;
    return flx_string("");
  }
  catch (const std::exception& e) {
    // Andere m�gliche Fehler w�hrend der Serialisierung.
    std::cerr << "An unexpected error occurred during JSON creation: " << e.what() << std::endl;
    return flx_string("");
  }
}
