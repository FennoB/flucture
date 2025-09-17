#ifndef FLX_JSON_H
#define FLX_JSON_H

#include "../../utils/flx_variant.h" // Enthält flx_string und flx_variant_map Definitionen

// Vorwärtsdeklaration ist nicht notwendig, da nlohmann::json nicht im Header verwendet wird.

class flx_json {
public:
  /**
   * @brief Konstruktor für flx_json.
   * @param map_ptr Ein Zeiger auf eine flx_variant_map, die von dieser Instanz
   * manipuliert wird. Die Lebensdauer der Map muss extern verwaltet werden.
   */
  explicit flx_json(flxv_map* map_ptr);

  /**
   * @brief Parst einen JSON-String und füllt die assoziierte flx_variant_map.
   * @param json_string Der zu parsende JSON-String.
   * @return true bei Erfolg, false bei einem Fehler. Fehlerdetails könnten über eine
   * separate Methode oder einen Rückgabewert mit Fehlerinformationen behandelt werden (hier vereinfacht).
   * @note Die assoziierte flx_variant_map wird geleert, bevor neue Daten eingefügt werden.
   */
  bool parse(const flx_string& json_string);

  /**
   * @brief Erstellt einen JSON-String aus der assoziierten flx_variant_map.
   * @return Ein flx_string, der die JSON-Repräsentation der Map enthält.
   * Gibt einen leeren String zurück oder löst eine Ausnahme aus, wenn ein Fehler auftritt
   * (hier vereinfacht als leerer String).
   */
  flx_string create() const;

private:
  flxv_map* data_map; // Zeiger auf die externe Map
};

#endif // FLX_JSON_H
