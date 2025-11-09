#ifndef FLX_XML_H
#define FLX_XML_H

#include "../../utils/flx_variant.h"

class flx_xml {
public:
  /**
   * @brief Konstruktor für flx_xml.
   * @param map_ptr Ein Zeiger auf eine flx_variant_map, die von dieser Instanz
   * manipuliert wird. Die Lebensdauer der Map muss extern verwaltet werden.
   */
  explicit flx_xml(flxv_map* map_ptr);

  /**
   * @brief Parst einen XML-String und füllt die assoziierte flx_variant_map.
   * @param xml_string Der zu parsende XML-String.
   * @return true bei Erfolg, false bei einem Fehler.
   * @note Die assoziierte flx_variant_map wird geleert, bevor neue Daten eingefügt werden.
   * @note Attribute werden mit "@" Prefix gespeichert, Text-Content als "#text"
   */
  bool parse(const flx_string& xml_string);

  /**
   * @brief Erstellt einen XML-String aus der assoziierten flx_variant_map.
   * @return Ein flx_string, der die XML-Repräsentation der Map enthält.
   * @note Gibt einen leeren String zurück, wenn ein Fehler auftritt.
   */
  flx_string create() const;

private:
  flxv_map* data_map;
};

#endif // FLX_XML_H
