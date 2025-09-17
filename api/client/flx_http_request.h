#ifndef FLX_HTTP_REQUEST_H
#define FLX_HTTP_REQUEST_H

#include "../../utils/flx_variant.h" // Enthält flx_string und flx_variant_map Definitionen

// Vorwärtsdeklarationen sind nicht notwendig, da die HTTPRequest-Bibliothek
// nur in der .cpp-Datei verwendet wird.

class flx_http_request {
public:
  /**
   * @brief Standardkonstruktor.
   */
  flx_http_request();

  /**
   * @brief Konstruktor mit einer initialen URL.
   * @param url Die URL für die HTTP-Anfrage.
   */
  explicit flx_http_request(const flx_string& url);

  /**
   * @brief Setzt die URL für die Anfrage.
   * @param url Die zu setzende URL.
   */
  void set_url(const flx_string& url);

  /**
   * @brief Gibt die aktuelle URL zurück.
   * @return Die aktuelle URL.
   */
  flx_string get_url() const;

  /**
   * @brief Setzt die HTTP-Methode (z.B. "GET", "POST").
   * @param method Die zu setzende HTTP-Methode.
   */
  void set_method(const flx_string& method);

  /**
   * @brief Gibt die aktuelle HTTP-Methode zurück.
   * @return Die aktuelle HTTP-Methode.
   */
  flx_string get_method() const;

  /**
   * @brief Fügt einen Header hinzu oder aktualisiert ihn.
   * @param key Der Schlüssel des Headers.
   * @param value Der Wert des Headers.
   */
  void set_header(const flx_string& key, const flx_string& value);

  /**
   * @brief Gibt den Wert eines Headers zurück.
   * @param key Der Schlüssel des Headers.
   * @return Der Wert des Headers oder ein leerer flx_string, wenn nicht gefunden.
   */
  flx_string get_header(const flx_string& key) const;

  /**
   * @brief Gibt die Map aller Header zurück.
   * @return Eine Referenz auf die Header-Map.
   */
  flxv_map& get_headers();
  const flxv_map& get_headers() const;


  /**
   * @brief Fügt einen URL-Parameter hinzu oder aktualisiert ihn (für GET-Requests).
   * @param key Der Schlüssel des Parameters.
   * @param value Der Wert des Parameters.
   */
  void set_param(const flx_string& key, const flx_string& value);

  /**
   * @brief Gibt den Wert eines URL-Parameters zurück.
   * @param key Der Schlüssel des Parameters.
   * @return Der Wert des Parameters oder ein leerer flx_string, wenn nicht gefunden.
   */
  flx_string get_param(const flx_string& key) const;

  /**
   * @brief Gibt die Map aller URL-Parameter zurück.
   * @return Eine Referenz auf die Parameter-Map.
   */
  flxv_map& get_params();
  const flxv_map& get_params() const;

  /**
   * @brief Setzt den Body der Anfrage (z.B. für POST-Requests).
   * @param body Der zu setzende Body.
   */
  void set_body(const flx_string& body);

  /**
   * @brief Gibt den aktuellen Body der Anfrage zurück.
   * @return Der aktuelle Body.
   */
  flx_string get_body() const;

  /**
   * @brief Sendet die HTTP-Anfrage synchron.
   * @return true bei Erfolg (Statuscode 2xx), false bei einem Fehler.
   */
  bool send();

  /**
   * @brief Gibt den HTTP-Statuscode der letzten Antwort zurück.
   * @return Der Statuscode (z.B. 200, 404). Gibt 0 zurück, wenn keine Anfrage gesendet wurde oder ein Fehler auftrat.
   */
  int get_status_code() const;

  /**
   * @brief Gibt den Body der letzten Antwort zurück.
   * @return Der Antwort-Body.
   */
  flx_string get_response_body() const;

  /**
   * @brief Gibt die Header der letzten Antwort zurück.
   * @return Eine Map der Antwort-Header.
   */
  const flxv_map& get_response_headers() const;

  /**
   * @brief Gibt die Fehlermeldung der letzten Anfrage zurück, falls vorhanden.
   * @return Die Fehlermeldung oder ein leerer String.
   */
  flx_string get_error_message() const;

private:
  flx_string url_;
  flx_string method_; // z.B. "GET", "POST", "PUT", "DELETE"
  flxv_map params_; // Für URL-Parameter (GET)
  flxv_map headers_;
  flx_string body_; // Für Request-Body (POST, PUT)

         // Antwortspezifische Daten
  int status_code_;
  flx_string response_body_;
  flxv_map response_headers_;
  flx_string error_message_;

         // Interne Hilfsmethoden (optional, falls benötigt)
         // void parse_url(); // Zum Extrahieren von Host, Pfad etc. aus der URL
};

#endif // FLX_HTTP_REQUEST_H
