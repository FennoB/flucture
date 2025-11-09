#include "flx_http_request.h"
#include <curl/curl.h>
#include <string>
#include <vector>
#include <iostream>
#include <memory> // F�r std::unique_ptr

// Anonymer Namespace f�r interne Hilfsfunktionen und Callbacks
namespace {

         // Callback-Funktion f�r libcurl, um den Response Body zu schreiben.
         // Schreibt die empfangenen Daten in den user-data-Pointer (hier ein flx_string).
  size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t real_size = size * nmemb;
    flx_string* mem = static_cast<flx_string*>(userp);
    mem->append(static_cast<char*>(contents), real_size);
    return real_size;
  }

         // Callback-Funktion f�r libcurl, um die Response Header zu schreiben.
         // Analysiert jede Header-Zeile und f�gt sie der user-data-Map (flxv_map) hinzu.
  size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t numbytes = size * nitems;
    flxv_map* headers = static_cast<flxv_map*>(userdata);
    flx_string header_line(buffer, numbytes);

           // Header-Zeile aufteilen
    size_t colon_pos = header_line.find(":");
    if (colon_pos != flx_string::npos) {
      flx_string key = header_line.substr(0, colon_pos);
      flx_string value = header_line.substr(colon_pos + 2); // +2, um ": " zu �berspringen
      // Zeilenumbr�che am Ende des Werts entfernen
      size_t crlf_pos = value.find("\r\n");
      if (crlf_pos != flx_string::npos) {
        value.erase(crlf_pos);
      }
      (*headers)[key] = value;
    }
    return numbytes;
  }

  // Erstellt einen URL-kodierten String aus Parametern
  flx_string encode_params(CURL* curl, const flxv_map& params) {
    flx_string encoded_params;
    bool first = true;
    for (const auto& pair : params) {
      if (!first) {
        encoded_params += "&";
      }
      first = false;

             // Schl�ssel kodieren
      char* encoded_key = curl_easy_escape(curl, pair.first.to_std_const().c_str(), 0);
      if(encoded_key) {
        encoded_params += encoded_key;
        curl_free(encoded_key);
      }

      encoded_params += "=";

             // Wert kodieren
      flx_string val_str; // Leerer String als Default
      if(pair.second.is_string()) val_str = pair.second.string_value();
      else if (pair.second.is_int()) val_str = flx_string(pair.second.int_value());
      else if (pair.second.is_double()) val_str = flx_string(pair.second.double_value());
      else if (pair.second.is_bool()) val_str = (pair.second.bool_value() ? "true" : "false");

      char* encoded_value = curl_easy_escape(curl, val_str.to_std_const().c_str(), 0);
      if(encoded_value) {
        encoded_params += encoded_value;
        curl_free(encoded_value);
      }
    }
    return encoded_params;
  }

  size_t file_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t real_size = size * nmemb;
    FILE* file = static_cast<FILE*>(userp);
    return fwrite(contents, size, nmemb, file);
  }

} // Ende anonymer Namespace

// --- Implementierung der Klassenmethoden ---

flx_http_request::flx_http_request()
    : url_(), method_("GET"), status_code_(0) {}

flx_http_request::flx_http_request(const flx_string& url)
    : url_(url), method_("GET"), status_code_(0) {}

// Setter und Getter bleiben unver�ndert
void flx_http_request::set_url(const flx_string& url) { url_ = url; }
flx_string flx_http_request::get_url() const { return url_; }
void flx_http_request::set_method(const flx_string& method) { method_ = method; }
flx_string flx_http_request::get_method() const { return method_; }
void flx_http_request::set_header(const flx_string& key, const flx_string& value) { headers_[key] = value; }
flx_string flx_http_request::get_header(const flx_string& key) const {
  auto it = headers_.find(key);
  return (it != headers_.end() && it->second.is_string()) ? it->second.string_value() : flx_string();
}
flxv_map& flx_http_request::get_headers() { return headers_; }
const flxv_map& flx_http_request::get_headers() const { return headers_; }
void flx_http_request::set_param(const flx_string& key, const flx_string& value) { params_[key] = value; }
flx_string flx_http_request::get_param(const flx_string& key) const {
  auto it = params_.find(key);
  return (it != params_.end() && it->second.is_string()) ? it->second.string_value() : flx_string();
}
flxv_map& flx_http_request::get_params() { return params_; }
const flxv_map& flx_http_request::get_params() const { return params_; }
void flx_http_request::set_body(const flx_string& body) { body_ = body; }
flx_string flx_http_request::get_body() const { return body_; }

// Die zentrale 'send'-Methode, neu implementiert mit libcurl
bool flx_http_request::send() {
  // 1. Reset der Antwortdaten
  status_code_ = 0;
  response_body_.clear();
  response_headers_.clear();
  error_message_.clear();

  if (url_.empty()) {
    error_message_ = "URL is empty.";
    return false;
  }

         // RAII-Wrapper f�r CURL und curl_slist, um die Freigabe zu garantieren
  auto curl_deleter = [](CURL* c) { if (c) curl_easy_cleanup(c); };
  auto slist_deleter = [](struct curl_slist* s) { if (s) curl_slist_free_all(s); };

  std::unique_ptr<CURL, decltype(curl_deleter)> curl(curl_easy_init(), curl_deleter);
  if (!curl) {
    error_message_ = "Failed to initialize libcurl.";
    return false;
  }

  std::unique_ptr<struct curl_slist, decltype(slist_deleter)> header_list(nullptr, slist_deleter);
  char errbuf[CURL_ERROR_SIZE] = {0};

  try {
    // 2. URL und Parameter zusammenbauen
    flx_string final_url = url_;
    flx_string method_upper = method_.upper();
    if (method_upper == "GET" && !params_.empty()) {
      flx_string query_string = encode_params(curl.get(), params_);
      if (!query_string.empty()) {
        final_url += flx_string(final_url.find("?") == flx_string::npos ? "?" : "&") + query_string;
      }
    }

    // 3. libcurl-Optionen setzen
    curl_easy_setopt(curl.get(), CURLOPT_URL, final_url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, errbuf);

    // Automatisch Redirects folgen (�blich und n�tzlich)
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);

           // Callbacks f�r Response Body und Header setzen
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body_);
    curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA, &response_headers_);

           // 4. Request-Header setzen
    for (const auto& pair : headers_) {
      if (pair.second.is_string()) {
        flx_string header_string = pair.first + ": " + pair.second.string_value();
        header_list.reset(curl_slist_append(header_list.release(), header_string.c_str()));
      }
    }
    if (header_list) {
      curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, header_list.get());
    }

           // 5. HTTP-Methode und Body setzen
    if (method_upper == "POST") {
      curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
      curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body_.c_str());
      curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, body_.length());
    } else if (method_upper == "PUT") {
      curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, "PUT");
      curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body_.c_str());
      curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, body_.length());
    } else if (method_upper == "DELETE") {
      curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (method_upper != "GET") { // F�r andere Methoden
      curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, method_upper.c_str());
    }

           // 6. Anfrage senden
    CURLcode res = curl_easy_perform(curl.get());

           // 7. Ergebnis auswerten
    if (res != CURLE_OK) {
      error_message_ = flx_string("libcurl error: ") + curl_easy_strerror(res) + " - " + errbuf;
      return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
    status_code_ = static_cast<int>(http_code);

    if (http_code > 300)
    {
      error_message_ = flx_string("HTTP error: ") + std::to_string(status_code_) + " - " + response_body_;
    }

    return status_code_ >= 200 && status_code_ < 300;

  } catch (const std::exception& e) {
    error_message_ = flx_string("Standard Exception: ") + e.what();
    return false;
  } catch (...) {
    error_message_ = "An unknown error occurred during HTTP request.";
    return false;
  }
}

// Antwort-Getter bleiben unver�ndert
int flx_http_request::get_status_code() const { return status_code_; }
flx_string flx_http_request::get_response_body() const { return response_body_; }
const flxv_map& flx_http_request::get_response_headers() const { return response_headers_; }
flx_string flx_http_request::get_error_message() const { return error_message_; }

bool flx_http_request::download_to_file(const flx_string& output_path) {
  // 1. Reset der Antwortdaten
  status_code_ = 0;
  response_body_.clear();
  response_headers_.clear();
  error_message_.clear();

  if (url_.empty()) {
    error_message_ = "URL is empty.";
    return false;
  }

  if (output_path.empty()) {
    error_message_ = "Output path is empty.";
    return false;
  }

  // 2. Datei zum Schreiben öffnen
  FILE* file = fopen(output_path.c_str(), "wb");
  if (!file) {
    error_message_ = flx_string("Failed to open output file: ") + output_path;
    return false;
  }

  // RAII-Wrapper für CURL und curl_slist
  auto curl_deleter = [](CURL* c) { if (c) curl_easy_cleanup(c); };
  auto slist_deleter = [](struct curl_slist* s) { if (s) curl_slist_free_all(s); };

  std::unique_ptr<CURL, decltype(curl_deleter)> curl(curl_easy_init(), curl_deleter);
  if (!curl) {
    error_message_ = "Failed to initialize libcurl.";
    fclose(file);
    return false;
  }

  std::unique_ptr<struct curl_slist, decltype(slist_deleter)> header_list(nullptr, slist_deleter);
  char errbuf[CURL_ERROR_SIZE] = {0};

  try {
    // 3. URL und Parameter zusammenbauen
    flx_string final_url = url_;
    flx_string method_upper = method_.upper();
    if (method_upper == "GET" && !params_.empty()) {
      flx_string query_string = encode_params(curl.get(), params_);
      if (!query_string.empty()) {
        final_url += flx_string(final_url.find("?") == flx_string::npos ? "?" : "&") + query_string;
      }
    }

    // 4. libcurl-Optionen setzen
    curl_easy_setopt(curl.get(), CURLOPT_URL, final_url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);

    // Callbacks für direktes Datei-Schreiben
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, file_write_callback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, file);

    // Response Header tracking
    curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA, &response_headers_);

    // 5. Request-Header setzen
    for (const auto& pair : headers_) {
      if (pair.second.is_string()) {
        flx_string header_string = pair.first + ": " + pair.second.string_value();
        header_list.reset(curl_slist_append(header_list.release(), header_string.c_str()));
      }
    }
    if (header_list) {
      curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, header_list.get());
    }

    // 6. HTTP-Methode und Body setzen
    if (method_upper == "POST") {
      curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
      curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body_.c_str());
      curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, body_.length());
    } else if (method_upper == "PUT") {
      curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, "PUT");
      curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body_.c_str());
      curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, body_.length());
    } else if (method_upper == "DELETE") {
      curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (method_upper != "GET") {
      curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, method_upper.c_str());
    }

    // 7. Anfrage senden
    CURLcode res = curl_easy_perform(curl.get());

    // 8. Datei schließen
    fclose(file);

    // 9. Ergebnis auswerten
    if (res != CURLE_OK) {
      error_message_ = flx_string("libcurl error: ") + curl_easy_strerror(res) + " - " + errbuf;
      return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
    status_code_ = static_cast<int>(http_code);

    if (http_code > 300) {
      error_message_ = flx_string("HTTP error: ") + std::to_string(status_code_);
    }

    return status_code_ >= 200 && status_code_ < 300;

  } catch (const std::exception& e) {
    fclose(file);
    error_message_ = flx_string("Standard Exception: ") + e.what();
    return false;
  } catch (...) {
    fclose(file);
    error_message_ = "An unknown error occurred during file download.";
    return false;
  }
}
