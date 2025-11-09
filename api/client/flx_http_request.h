#ifndef FLX_HTTP_REQUEST_H
#define FLX_HTTP_REQUEST_H

#include "../../utils/flx_variant.h" // Enth�lt flx_string und flx_variant_map Definitionen

// Vorw�rtsdeklarationen sind nicht notwendig, da die HTTPRequest-Bibliothek
// nur in der .cpp-Datei verwendet wird.

class flx_http_request {
public:
  /**
   * @brief Standardkonstruktor.
   */
  flx_http_request();

  /**
   * @brief Konstruktor mit einer initialen URL.
   * @param url Die URL f�r die HTTP-Anfrage.
   */
  explicit flx_http_request(const flx_string& url);

  /**
   * @brief Setzt die URL f�r die Anfrage.
   * @param url Die zu setzende URL.
   */
  void set_url(const flx_string& url);

  /**
   * @brief Gibt die aktuelle URL zur�ck.
   * @return Die aktuelle URL.
   */
  flx_string get_url() const;

  /**
   * @brief Setzt die HTTP-Methode (z.B. "GET", "POST").
   * @param method Die zu setzende HTTP-Methode.
   */
  void set_method(const flx_string& method);

  /**
   * @brief Gibt die aktuelle HTTP-Methode zur�ck.
   * @return Die aktuelle HTTP-Methode.
   */
  flx_string get_method() const;

  /**
   * @brief F�gt einen Header hinzu oder aktualisiert ihn.
   * @param key Der Schl�ssel des Headers.
   * @param value Der Wert des Headers.
   */
  void set_header(const flx_string& key, const flx_string& value);

  /**
   * @brief Gibt den Wert eines Headers zur�ck.
   * @param key Der Schl�ssel des Headers.
   * @return Der Wert des Headers oder ein leerer flx_string, wenn nicht gefunden.
   */
  flx_string get_header(const flx_string& key) const;

  /**
   * @brief Gibt die Map aller Header zur�ck.
   * @return Eine Referenz auf die Header-Map.
   */
  flxv_map& get_headers();
  const flxv_map& get_headers() const;


  /**
   * @brief F�gt einen URL-Parameter hinzu oder aktualisiert ihn (f�r GET-Requests).
   * @param key Der Schl�ssel des Parameters.
   * @param value Der Wert des Parameters.
   */
  void set_param(const flx_string& key, const flx_string& value);

  /**
   * @brief Gibt den Wert eines URL-Parameters zur�ck.
   * @param key Der Schl�ssel des Parameters.
   * @return Der Wert des Parameters oder ein leerer flx_string, wenn nicht gefunden.
   */
  flx_string get_param(const flx_string& key) const;

  /**
   * @brief Gibt die Map aller URL-Parameter zur�ck.
   * @return Eine Referenz auf die Parameter-Map.
   */
  flxv_map& get_params();
  const flxv_map& get_params() const;

  /**
   * @brief Setzt den Body der Anfrage (z.B. f�r POST-Requests).
   * @param body Der zu setzende Body.
   */
  void set_body(const flx_string& body);

  /**
   * @brief Gibt den aktuellen Body der Anfrage zur�ck.
   * @return Der aktuelle Body.
   */
  flx_string get_body() const;

  /**
   * @brief Sendet die HTTP-Anfrage synchron.
   * @return true bei Erfolg (Statuscode 2xx), false bei einem Fehler.
   */
  bool send();

  /**
   * @brief L�dt die Datei direkt in eine Datei herunter (ohne in Memory zu laden).
   * @param output_path Pfad zur Zieldatei.
   * @return true bei Erfolg (Statuscode 2xx), false bei einem Fehler.
   */
  bool download_to_file(const flx_string& output_path);

  /**
   * @brief Gibt den HTTP-Statuscode der letzten Antwort zur�ck.
   * @return Der Statuscode (z.B. 200, 404). Gibt 0 zur�ck, wenn keine Anfrage gesendet wurde oder ein Fehler auftrat.
   */
  int get_status_code() const;

  /**
   * @brief Gibt den Body der letzten Antwort zur�ck.
   * @return Der Antwort-Body.
   */
  flx_string get_response_body() const;

  /**
   * @brief Gibt die Header der letzten Antwort zur�ck.
   * @return Eine Map der Antwort-Header.
   */
  const flxv_map& get_response_headers() const;

  /**
   * @brief Gibt die Fehlermeldung der letzten Anfrage zur�ck, falls vorhanden.
   * @return Die Fehlermeldung oder ein leerer String.
   */
  flx_string get_error_message() const;

private:
  flx_string url_;
  flx_string method_; // z.B. "GET", "POST", "PUT", "DELETE"
  flxv_map params_; // F�r URL-Parameter (GET)
  flxv_map headers_;
  flx_string body_; // F�r Request-Body (POST, PUT)

         // Antwortspezifische Daten
  int status_code_;
  flx_string response_body_;
  flxv_map response_headers_;
  flx_string error_message_;

         // Interne Hilfsmethoden (optional, falls ben�tigt)
         // void parse_url(); // Zum Extrahieren von Host, Pfad etc. aus der URL
};

#endif // FLX_HTTP_REQUEST_H
