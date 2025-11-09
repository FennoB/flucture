# XML Integration Migration Guide

## Migration von `apply_xml_map()` zu `read_xml()`

Dieses Dokument beschreibt die Migration vom alten relativen XML-Mapping-System zum neuen absoluten Pfad-basierten System.

---

## Alte API (deprecated)

### Konzept
- Methode: `model.apply_xml_map(xml_map)`
- Übergabe: `flxv_map` (bereits geparste XML-Daten)
- Pfade: Relativ zum aktuellen Element
- Problem: Komplexe Navigation bei verschachtelten Listen

### Beispiel (ALT)
```cpp
// XML parsen
flxv_map data;
flx_xml xml(&data);
xml.parse(xml_string);

// Model laden
Team team;
team.apply_xml_map(data["team"].to_map());

// Property-Deklarationen
class Team : public flx_model {
public:
    flxp_string(name, {{"xml_path", "teamname"}});
    flxp_model_list(members, Person, {{"xml_path", "member"}});
};

class Person : public flx_model {
public:
    flxp_string(name, {{"xml_path", "fullname"}});
    flxp_int(age, {{"xml_path", "age"}});
    flxp_model(address, Address, {{"xml_path", "address"}});
};
```

---

## Neue API (aktuell)

### Konzept
- Methode: `model.read_xml(xml, root_path)`
- Übergabe: `flx_xml&` (Parser-Objekt mit Hilfsmethoden)
- Pfade: **Absolut** vom XML-Root
- Index-Platzhalter: `[]` für Array-Elemente
- Vorteil: Eindeutige Pfade, tiefe Verschachtelung möglich

### Pfad-Syntax

#### Einfache Pfade
```
"team/teamname"           → <team><teamname>...</teamname></team>
"person/address/city"     → <person><address><city>...</city></address></person>
```

#### Listen mit Index-Platzhalter `[]`
```
"team/member[]"           → Alle <member> Elemente (wird zu [0], [1], [2]...)
"team/member[]/name"      → Name jedes Members
"team/member[]/address/city"  → Stadt jedes Members
```

#### Verschachtelte Listen
```
"company/departments[]/teams[]/members[]"
→ Für jedes Department, jedes Team, jeden Member
```

### Platzhalter-Verhalten

**Bei mehreren Elementen (Vector):**
- `"team/member[]"` → Iteration: `[0]`, `[1]`, `[2]`
- Jedes Element wird separat verarbeitet

**Bei einzelnem Element (Map):**
- `"team/member[]"` → Platzhalter entfernen: `"team/member"`
- Wird als 1-Element-Array behandelt

---

## Migrations-Schritte

### 1. Property-Deklarationen anpassen

**VORHER (relativ):**
```cpp
class Person : public flx_model {
public:
    flxp_string(name, {{"xml_path", "fullname"}});
    flxp_int(age, {{"xml_path", "age"}});
    flxp_model(address, Address, {{"xml_path", "address"}});
};
```

**NACHHER (absolut mit Root-Pfad):**
```cpp
class Person : public flx_model {
public:
    flxp_string(name, {{"xml_path", "name"}});  // Relativ zum Person-Kontext
    flxp_int(age, {{"xml_path", "age"}});
    flxp_model(address, Address, {{"xml_path", "address"}});
};
```

**WICHTIG:** Properties bleiben relativ zum Model-Kontext! Der Root-Pfad wird beim Aufruf übergeben.

### 2. Listen mit Index-Platzhalter

**VORHER:**
```cpp
class Team : public flx_model {
public:
    flxp_model_list(members, Person, {{"xml_path", "member"}});
};
```

**NACHHER:**
```cpp
class Team : public flx_model {
public:
    flxp_model_list(members, Person, {{"xml_path", "member[]"}});  // [] hinzugefügt
};
```

### 3. Aufruf-Syntax ändern

**VORHER:**
```cpp
flxv_map data;
flx_xml xml(&data);
xml.parse(xml_string);

Team team;
team.apply_xml_map(data["team"].to_map());  // Map übergeben
```

**NACHHER:**
```cpp
flxv_map data;
flx_xml xml(&data);
xml.parse(xml_string);

Team team;
team.read_xml(xml, "team");  // Parser + Root-Pfad übergeben
```

---

## Vollständiges Beispiel

### XML-Struktur
```xml
<team>
    <teamname>Development</teamname>
    <member>
        <fullname>Alice</fullname>
        <age>30</age>
        <address>
            <city>Boston</city>
            <street>Oak Ave 1</street>
        </address>
    </member>
    <member>
        <fullname>Bob</fullname>
        <age>28</age>
        <address>
            <city>Seattle</city>
            <street>Pine Rd 2</street>
        </address>
    </member>
</team>
```

### Model-Definitionen (NEU)
```cpp
class Address : public flx_model {
public:
    flxp_string(city, {{"xml_path", "city"}});
    flxp_string(street, {{"xml_path", "street"}});
};

class Person : public flx_model {
public:
    flxp_string(name, {{"xml_path", "fullname"}});
    flxp_int(age, {{"xml_path", "age"}});
    flxp_model(address, Address, {{"xml_path", "address"}});
};

class Team : public flx_model {
public:
    flxp_string(name, {{"xml_path", "teamname"}});
    flxp_model_list(members, Person, {{"xml_path", "member[]"}});  // [] !
};
```

### Usage (NEU)
```cpp
// Parse XML
flxv_map data;
flx_xml xml(&data);
xml.parse(xml_string);

// Load model
Team team;
team.read_xml(xml, "team");

// Access data
assert(team.name.value() == "Development");
assert(team.members.size() == 2);
assert(team.members[0].name.value() == "Alice");
assert(team.members[0].address.city.value() == "Boston");
assert(team.members[1].name.value() == "Bob");
```

---

## Interne Pfad-Auflösung (Beispiel)

Für `team.read_xml(xml, "team")`:

1. **team.name** → Pfad: `"team/teamname"` → `xml.read_path("team/teamname")`
2. **team.members** → Pfad: `"team/member[]"` → Erkennt Array mit 2 Elementen
   - Element 0: `team.members[0].read_xml(xml, "team/member[0]")`
     - name → `"team/member[0]/fullname"`
     - age → `"team/member[0]/age"`
     - address → `team.members[0].address.read_xml(xml, "team/member[0]/address")`
       - city → `"team/member[0]/address/city"`
   - Element 1: `team.members[1].read_xml(xml, "team/member[1]")`
     - ...

---

## Checkliste für Migration

- [ ] Alle `xml_path` in model_lists um `[]` erweitern
- [ ] `apply_xml_map(map)` → `read_xml(xml, root_path)` ändern
- [ ] Übergabe von `data[root].to_map()` → `xml, "root"` ändern
- [ ] Tests anpassen und ausführen
- [ ] Bei verschachtelten Listen: Mehrere `[]` Platzhalter verwenden

---

## Vorteile des neuen Systems

1. **Eindeutige Pfade**: Keine Mehrdeutigkeit bei verschachtelten Strukturen
2. **Tiefe Verschachtelung**: `departments[]/teams[]/members[]` problemlos möglich
3. **Debugging**: Voller Pfad zeigt exakt, wo im XML gelesen wird
4. **Konsistenz**: Gleiche Pfad-Syntax wie bei `read_row()` für DB
5. **Weniger Code**: Keine manuelle Navigation durch Maps nötig

---

## Support

Bei Fragen zur Migration:
- Siehe `tests/test_flx_model_xml_integration.cpp` für Beispiele
- Siehe `flx_xml.h` für verfügbare Hilfsmethoden
