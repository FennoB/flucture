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

  /**
   * @brief Liest einen Wert an einem absoluten Pfad im XML.
   * @param path Absoluter Pfad z.B. "team/member[0]/name" oder "team/member[]"
   * @return Pointer auf flx_variant oder nullptr wenn Pfad nicht existiert
   * @note Unterstützt Index-Notation [0], [1] für Array-Zugriff
   */
  const flx_variant* read_path(const flx_string& path) const;

  /**
   * @brief Prüft ob ein Pfad Index-Platzhalter [] enthält.
   * @param path Zu prüfender Pfad
   * @return true wenn [] gefunden, sonst false
   */
  static bool has_placeholder(const flx_string& path);

  /**
   * @brief Ersetzt ersten [] Platzhalter durch konkreten Index.
   * @param path Pfad mit Platzhalter, z.B. "team/member[]"
   * @param index Index zum Einsetzen
   * @return Neuer Pfad, z.B. "team/member[0]"
   */
  static flx_string replace_first_placeholder(const flx_string& path, size_t index);

  /**
   * @brief Entfernt ersten [] Platzhalter aus Pfad.
   * @param path Pfad mit Platzhalter, z.B. "team/member[]"
   * @return Pfad ohne Platzhalter, z.B. "team/member"
   */
  static flx_string remove_first_placeholder(const flx_string& path);

private:
  flxv_map* data_map;
};

#endif // FLX_XML_H
