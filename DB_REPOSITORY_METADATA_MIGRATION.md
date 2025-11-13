# db_repository Metadata System Migration

**Datum:** 2025-11-10
**Status:** In Progress (5/6 Tests bestehen)

---

## Zusammenfassung der Änderungen

Das Metadata-System für `db_repository` wurde grundlegend überarbeitet:

### Altes System (❌ veraltet)
```cpp
// Properties hatten "table" metadata
flxp_model_list(orders, Order, {
    {"table", "orders"},           // ❌ Entfernt
    {"foreign_key", "user_id"}     // ❌ Wert war Spaltenname
});

// primary_key hatte boolean Wert
flxp_int(id, {
    {"column", "id"},
    {"primary_key", true}          // ❌ Boolean-Wert
});

// Fallback auf property name wenn column fehlte
```

### Neues System (✅ aktuell)
```cpp
// Properties OHNE Metadata (Relations werden durch Child-Models erkannt)
flxp_model_list(orders, Order);   // ✅ Keine Metadata am Parent!

// Foreign Key im CHILD Model
class Order : public flx_model {
    flxp_int(id, {{"column", "id"}, {"primary_key", "orders"}});  // ✅ Tabellenname
    flxp_int(user_id, {
        {"column", "user_id"},
        {"foreign_key", "users"}   // ✅ Zieltabellenname
    });
};

// column Metadata ist PFLICHT (kein Fallback!)
```

---

## Metadata-Regeln (Neues System)

| Metadata Key | Wert | Verwendung | Erforderlich |
|--------------|------|------------|--------------|
| `{"column", "db_name"}` | DB-Spaltenname | Mapping Property → DB | ✅ JA (kein Fallback!) |
| `{"primary_key", "table"}` | Tabellenname | Markiert PK, identifiziert Tabelle | ✅ JA (ein Feld pro Model) |
| `{"foreign_key", "table"}` | Zieltabellenname | Markiert FK-Beziehung | Optional |

**Wichtig:**
- ❌ Keine `{"table", ...}` Metadata mehr
- ❌ `flxp_model` und `flxp_model_list` haben **KEINE** Metadata
- ✅ FK-Metadata steht beim **tatsächlichen Feld** im Child-Model
- ✅ `column` ist **ZWINGEND** - ohne wird Feld ignoriert

---

## Implementierte Änderungen

### 1. Konstruktor ohne table_name Parameter

**Vorher:**
```cpp
db_repository<User> repo(&conn, "users");  // ❌ Expliziter Tabellenname
```

**Nachher:**
```cpp
db_repository<User> repo(&conn);           // ✅ Automatisch aus Metadata
```

**Implementation:**
```cpp
template<typename T>
db_repository<T>::db_repository(db_connection* conn)
  : connection_(conn)
  , table_name_(extract_table_name())  // Extrahiert aus primary_key
  , id_column_("id")
  , last_error_("")
  , embedder_(nullptr)
{}

template<typename T>
flx_string db_repository<T>::extract_table_name()
{
  T sample_model;
  const auto& properties = sample_model.get_properties();

  for (const auto& [prop_name, prop] : properties) {
    const flxv_map& meta = prop->get_meta();

    if (meta.find("primary_key") != meta.end()) {
      // Wert von primary_key IST der Tabellenname
      return meta.at("primary_key").string_value();
    }
  }

  return "";  // Fehler: Kein PK gefunden
}
```

---

### 2. ensure_structures() Methode

**Zweck:** Erstellt alle Tabellen (eigene + Child-Tabellen) idempotent.

```cpp
db_repository<test_company> repo(&conn);
repo.ensure_structures();  // Erstellt alle Tabellen in Hierarchie
```

**Implementation:**
```cpp
template<typename T>
bool db_repository<T>::ensure_structures()
{
  // 1. Eigene Tabelle migrieren (erstellt wenn nicht existiert)
  if (!migrate_table()) {
    return false;
  }

  // 2. Child-Tabellen rekursiv bearbeiten
  // TODO: Noch nicht vollständig implementiert

  return true;
}
```

**Status:** Grundfunktion vorhanden, rekursive Child-Behandlung offen.

---

### 3. scan_fields() - Neues Metadata-System

**Änderungen:**
- ✅ `column` Metadata ist **PFLICHT** (kein Fallback!)
- ✅ Prüft nicht mehr auf `{"table", ...}` zum Ausschließen von Relations
- ✅ `primary_key` Wert ist jetzt String (Tabellenname), nicht boolean

```cpp
template<typename T>
std::vector<typename db_repository<T>::field_metadata> db_repository<T>::scan_fields()
{
  std::vector<field_metadata> fields;
  T sample_model;
  const auto& properties = sample_model.get_properties();

  for (const auto& [prop_name, prop] : properties) {
    const flxv_map& meta = prop->get_meta();

    // REQUIRED: column metadata muss existieren (kein Fallback!)
    if (meta.find("column") == meta.end()) {
      continue;  // Feld wird ignoriert
    }

    field_metadata field;
    field.property_name = prop_name;
    field.cpp_name = prop_name;
    field.column_name = meta.at("column").string_value();

    // primary_key (Wert = Tabellenname)
    field.is_primary_key = (meta.find("primary_key") != meta.end());

    // foreign_key (Wert = Zieltabelle)
    if (meta.find("foreign_key") != meta.end()) {
      field.is_foreign_key = true;
      field.foreign_table = meta.at("foreign_key").string_value();
    } else {
      field.is_foreign_key = false;
    }

    field.type = prop->get_variant_type();
    fields.push_back(field);
  }

  return fields;
}
```

---

### 4. scan_relations() - Structure-Only Scanning

**Herausforderung:** Relations ohne Metadata am Parent erkennen.

**Lösung:** Nutzung von `get_children()` und `get_model_lists()` + `factory()`

```cpp
template<typename T>
std::vector<typename db_repository<T>::relation_metadata> db_repository<T>::scan_relations()
{
  std::vector<relation_metadata> relations;
  T sample_model;  // Non-const für Struktur-Initialisierung

  const auto& children = sample_model.get_children();
  const auto& model_lists = sample_model.get_model_lists();

  // Process flxp_model (1:1 relations)
  for (const auto& [child_name, child_ptr] : children) {
    if (child_ptr == nullptr) continue;

    relation_metadata rel;
    rel.property_name = child_name;

    // Scanne Child-Properties für Metadata
    const auto& child_properties = child_ptr->get_properties();
    for (const auto& [prop_name, prop] : child_properties) {
      const flxv_map& meta = prop->get_meta();

      // Tabelle aus primary_key extrahieren
      if (meta.find("primary_key") != meta.end()) {
        rel.related_table = meta.at("primary_key").string_value();
      }

      // FK zurück zum Parent finden
      if (meta.find("foreign_key") != meta.end()) {
        flx_string fk_table = meta.at("foreign_key").string_value();
        if (fk_table == table_name_ && meta.find("column") != meta.end()) {
          rel.foreign_key_column = meta.at("column").string_value();
        }
      }
    }

    if (!rel.related_table.empty() && !rel.foreign_key_column.empty()) {
      relations.push_back(rel);
    }
  }

  // Process flxp_model_list (1:N relations)
  for (const auto& [list_name, list_ptr] : model_lists) {
    if (list_ptr == nullptr) continue;

    relation_metadata rel;
    rel.property_name = list_name;

    // WICHTIG: factory() für Sample-Element nutzen (KEIN Datenzugriff!)
    auto sample_elem = list_ptr->factory();
    if (sample_elem.is_null()) continue;

    // Scanne Element-Properties für Metadata
    const auto& elem_properties = sample_elem->get_properties();
    for (const auto& [prop_name, prop] : elem_properties) {
      const flxv_map& meta = prop->get_meta();

      if (meta.find("primary_key") != meta.end()) {
        rel.related_table = meta.at("primary_key").string_value();
      }

      if (meta.find("foreign_key") != meta.end()) {
        flx_string fk_table = meta.at("foreign_key").string_value();
        if (fk_table == table_name_ && meta.find("column") != meta.end()) {
          rel.foreign_key_column = meta.at("column").string_value();
        }
      }
    }

    if (!rel.related_table.empty() && !rel.foreign_key_column.empty()) {
      relations.push_back(rel);
    }
  }

  return relations;
}
```

**Kritische Erkenntnisse:**
- ✅ Properties sind über Namen in `children` / `model_lists` registriert
- ✅ `factory()` muss pure virtual (`= 0`) sein in `flx_list`
- ✅ Kein Datenzugriff - nur Struktur-Scanning!
- ✅ Non-const Model für Struktur-Initialisierung erlaubt

---

### 5. migrate_table() mit auto_configure()

**Problem:** `create_table()` benötigt `columns_`, aber `migrate_table()` rief `auto_configure()` nicht auf.

**Lösung:**
```cpp
template<typename T>
bool db_repository<T>::migrate_table()
{
  if (!table_exists()) {
    // Auto-configure BEVOR create_table()
    if (!auto_configure()) {
      last_error_ = "Failed to auto-configure schema";
      return false;
    }
    return create_table();
  }

  // Migration bestehender Tabellen...
}
```

---

### 6. find_primary_key_column() Update

**Änderung:** `primary_key` ist jetzt String (Tabellenname), nicht boolean.

```cpp
template<typename T>
flx_string db_repository<T>::find_primary_key_column(flx_model& model)
{
  const auto& properties = model.get_properties();

  for (const auto& [prop_name, prop] : properties) {
    const flxv_map& meta = prop->get_meta();

    // primary_key existiert = ist PK (Wert ist Tabellenname)
    if (meta.find("primary_key") != meta.end()) {
      if (meta.find("column") != meta.end()) {
        return meta.at("column").string_value();
      }
    }
  }

  return "id";  // Fallback
}
```

---

### 7. collect_all_table_names() Update

**Änderung:** Tabellennamen aus Child-Models extrahieren statt aus Parent-Metadata.

```cpp
template<typename T>
std::set<flx_string> db_repository<T>::collect_all_table_names(flx_model& model, const flx_string& root_table)
{
  std::set<flx_string> tables;
  tables.insert(root_table);

  // Children (flxp_model - 1:1)
  const auto& children = model.get_children();
  for (const auto& [child_name, child_ptr] : children) {
    if (child_ptr == nullptr) continue;

    // Tabellenname aus Child's primary_key extrahieren
    flx_string child_table;
    const auto& child_properties = child_ptr->get_properties();
    for (const auto& [prop_name, prop] : child_properties) {
      const flxv_map& meta = prop->get_meta();
      if (meta.find("primary_key") != meta.end()) {
        child_table = meta.at("primary_key").string_value();
        break;
      }
    }

    if (!child_table.empty()) {
      auto child_tables = collect_all_table_names(*child_ptr, child_table);
      tables.insert(child_tables.begin(), child_tables.end());
    }
  }

  // Model Lists (flxp_model_list - 1:N)
  const auto& model_lists = model.get_model_lists();
  for (const auto& [list_name, list_ptr] : model_lists) {
    if (list_ptr == nullptr) continue;

    // factory() für Sample-Element
    auto sample_elem = list_ptr->factory();
    if (sample_elem.is_null()) continue;

    flx_string child_table;
    const auto& elem_properties = sample_elem->get_properties();
    for (const auto& [prop_name, prop] : elem_properties) {
      const flxv_map& meta = prop->get_meta();
      if (meta.find("primary_key") != meta.end()) {
        child_table = meta.at("primary_key").string_value();
        break;
      }
    }

    if (!child_table.empty()) {
      tables.insert(child_table);
      auto child_tables = collect_all_table_names(*sample_elem, child_table);
      tables.insert(child_tables.begin(), child_tables.end());
    }
  }

  return tables;
}
```

---

## factory() - Pure Virtual Interface

**Notwendigkeit:** Um Typ-Information von `flxp_model_list` Elementen zu bekommen **ohne** Daten zu erstellen.

**Hinzugefügt in:** `utils/flx_model.h`

```cpp
class flx_list {
public:
  virtual ~flx_list() = default;
  virtual size_t list_size() = 0;
  virtual flx_model* get_model_at(size_t index) = 0;
  virtual void resync() = 0;
  virtual flx_lazy_ptr<flx_model> factory() const = 0;  // ✅ Pure virtual
};
```

**Wichtig:**
- ✅ **Pure virtual** (`= 0`) - jede abgeleitete Klasse MUSS implementieren
- ✅ Gibt `flx_lazy_ptr<flx_model>` zurück (managed pointer)
- ✅ `const` - kein Datenzugriff, nur Struktur
- ✅ Wird von `flx_model_list<T>` implementiert

---

## Test-Status

**Tatsächliche Ergebnisse:**
- **Test Scenarios:** 6 gesamt
- **Bestanden:** 5 Scenarios ✅
- **Fehlgeschlagen:** 1 Scenario ❌
- **Assertions:** 49 gesamt (45 bestanden, 4 fehlgeschlagen)

### ✅ Bestandene Scenarios (5/6):
1. Simple flat model CRUD
2. NULL value handling
3. Properties without column metadata are ignored
4. Fieldname vs column metadata robustness
5. Full CRUD lifecycle round-trip

### ❌ Fehlgeschlagenes Scenario (1/6):
**Scenario:** One-level nested model (1:1)
**Assertions fehlgeschlagen:** 4
**Grund:** `scan_relations()` findet keine Relations

**Fehler-Output:**
```
[scan_relations] Scanning structure...
[scan_relations] children.size()=0 model_lists.size()=0
[scan_relations] Total relations found: 0
```

**Root Cause:** `get_children()` und `get_model_lists()` liefern leere Maps zurück bei frisch erstellten Models.

### 📊 Test-Datei Übersicht:

**Datei:** `tests/test_db_repository.cpp` (2998 lines, unveränderlich)

**Test-Kategorien laut Kommentar (20 Scenarios geplant):**
- `[unit]` - Fast tests, no dependencies
- `[nested]` - Hierarchical structures (1:1, 1:N, multi-level)
- `[integration]` - Full round-trip tests
- `[db]` - Database-specific operations
- `[slow]` - Performance tests (1000+ items)

**Aktuell getestet:** Nur erste 6 Scenarios (ohne `[slow]` Tag)

**Status unbekannt für:**
- Wide hierarchy (10+ sibling lists)
- Large datasets tests
- Multi-level deep hierarchies (5 levels)
- Alle anderen nested structure tests

---

## Offene Arbeiten

### 1. save_nested_objects() & load_nested_objects() anpassen
**Status:** In Progress
**Problem:** Alte Implementation nutzt noch Metadata am Parent

### 2. build_joins_recursive() & build_id_selects_recursive() anpassen
**Status:** Pending
**Grund:** Benötigt für hierarchical search

### 3. ensure_structures() vervollständigen
**Status:** Pending
**Aufgabe:** Rekursiv alle Child-Tabellen erstellen

---

## Wichtige Erkenntnisse / Lessons Learned

### 1. Const-Correctness ist entscheidend
- ✅ Structure-Scanning MUSS ohne Datenzugriff funktionieren
- ✅ `const` Models verwenden wo möglich
- ✅ Non-const nur für Struktur-Initialisierung (nicht Daten)

### 2. Property-System Internals
- Properties registrieren sich automatisch in `children` / `model_lists`
- `flxp_model` → `get_children()`
- `flxp_model_list` → `get_model_lists()`
- Namen sind Property-Namen (C++ Identifier)

### 3. factory() ist essentiell
- Ohne `factory()` keine Typ-Information für Listen
- Pure virtual (`= 0`) zwingt Implementation
- Returned managed pointer (`flx_lazy_ptr`), keine raw pointer

### 4. Metadata-Philosophie
- **Parent:** Keine Metadata (Relations sind implizit durch Typen)
- **Child:** Alle Metadata (Tabelle, FK, Spalten)
- **Separation of Concerns:** Child weiß wohin er gehört, Parent nicht

### 5. Migration Strategie
- ✅ `auto_configure()` VOR `create_table()` aufrufen
- ✅ `columns_` wird von `auto_configure()` gefüllt
- ✅ Idempotent: `migrate_table()` prüft Existenz

### 6. Debugging-Ansatz
- Debug-Ausgaben hinzufügen: `std::cerr << "[scan_relations] ..."`
- Größe von Collections prüfen: `children.size()`, `model_lists.size()`
- Nie Annahmen machen - immer prüfen ob Pointer `nullptr` sind

---

## Nächste Schritte (Priorität)

1. **HOCH:** Warum `children.size() == 0`?
   - Prüfen ob Properties korrekt registriert werden
   - `flxp_model` Makro-Expansion überprüfen
   - Test mit einzelnem 1:1 Child

2. **HOCH:** `save_nested_objects()` / `load_nested_objects()` debuggen
   - Alte "table" Metadata-Checks entfernen
   - Auf neue `scan_relations()` umstellen

3. **MITTEL:** `build_joins_recursive()` anpassen
   - Alte Metadata-Logik entfernen
   - `factory()` nutzen

4. **NIEDRIG:** `ensure_structures()` vervollständigen
   - Rekursion durch alle Child-Tabellen
   - Idempotente Tabellen-Erstellung

---

## Referenzen

**Geänderte Dateien:**
- `api/db/db_repository.h` - Hauptänderungen
- `utils/flx_model.h` - `factory()` Interface
- `CLAUDE.md` - API-Dokumentation aktualisiert

**Test-Datei:**
- `tests/test_db_repository.cpp` - 20 Test-Scenarios (unveränderlich)

**Commit Message (geplant):**
```
Implement new db_repository metadata system

Breaking Changes:
- Removed table_name parameter from constructor
- Removed {"table", ...} metadata from properties
- Changed {"primary_key", true} to {"primary_key", "table_name"}
- Changed {"foreign_key", "column"} to {"foreign_key", "table"}
- Made {"column", ...} metadata REQUIRED (no fallback)

New Features:
- Added ensure_structures() for idempotent table creation
- Added extract_table_name() static method
- Added factory() pure virtual to flx_list interface
- scan_relations() now scans child models instead of parent metadata

Implementation:
- Updated scan_fields() for new metadata rules
- Updated scan_relations() to use get_children()/get_model_lists()
- Updated find_primary_key_column() for string-valued primary_key
- Updated collect_all_table_names() to scan child structures
- migrate_table() now calls auto_configure() before create_table()

Status: 5/6 tests passing (nested objects work in progress)

🤖 Generated with Claude Code
Co-Authored-By: Claude <noreply@anthropic.com>
```

---

**Letztes Update:** 2025-11-10
**Autor:** Claude Code
**Review-Status:** Pending User Review
