# Debug-Protokoll: pH-Wert Extraktionsproblem

**Datum:** 2025-10-16
**PDF:** BTS 5070 Safety Data Sheet
**Problem:** Text "ca. 1,5 (1%)" wird nicht extrahiert, obwohl pdftotext ihn findet

## Chronologie der Debugging-Session

### 1. Initiales Problem
- **Symptom:** CSV zeigt "ca. 12,0" statt "ca. 1,5 (1%)" für pH-Wert
- **pdftotext Ausgabe:** Zeile 304: `· pH-Wert bei 20 °C:                        ca. 1,5 (1%)`
- **Hypothese:** Positionierungsproblem im ASCII-Layout

### 2. Doppelter GetString() Crash (GELÖST)
- **Problem:** Segfault durch doppelten `GetString()` Aufruf in Debug-Code
- **Root Cause:** `GetString()` gibt `string_view` zurück, der invalidiert wird
- **Fix:** Debug-Ausgaben entfernt, die `GetString()` zweimal aufriefen

### 3. JSON Export zur Ursachenanalyse
- **Methode:** Exportiere raw flucture flx_layout_document als JSON
- **Ergebnis:** 155,031 Zeichen JSON mit 828 Texten
- **Kritische Erkenntnis:** Text "ca. 1,5" erscheint NIRGENDS im JSON

### 4. Seite 5 Analyse  
- **Befund:** Alle 69 Texte auf Seite 5 haben leeren Content
- **Schlussfolgerung:** Problem liegt in Text-Decodierung, nicht in Layout

### 5. Font-Encoding Analyse (DURCHBRUCH)
- **Debug-Code:** Vergleiche TryScanEncodedString vs GetString
- **Ergebnis:** TryScanEncodedString funktioniert PERFEKT!
  - CIDFont wird korrekt dekodiert
  - Fallback würde falsche Zeichen liefern
  - Font-Encoding ist NICHT das Problem

### 6. Finale Diagnose: XObject Problem
- **Beobachtung:** "ca. 1,5 (1%)" erscheint NIRGENDS in Debug-Logs
- **Code bei Zeile 245:** `args.Flags = PdfContentReaderFlags::SkipHandleNonFormXObjects`
- **ROOT CAUSE:** XObjects werden übersprungen!

## Lösung: XObject/FormXObject Unterstützung

**Fehlende Operatoren:**
- ❌ Do (XObject/FormXObject Execution)
- ❌ Recursive FormXObject Processing

**Implementation Plan:**
1. Do-Operator Handler hinzufügen
2. FormXObject Content Streams rekursiv verarbeiten  
3. Flag SkipHandleNonFormXObjects entfernen

## Status
- Font-Encoding: ✅ GELÖST
- XObject-Support: ✅ GELÖST

## ✅ FINALE LÖSUNG (2025-10-16)

**Problem gelöst durch eine einzige Zeile Code!**

### Die Lösung
```cpp
// flx_pdf_text_extractor.cpp:245
// VORHER:
args.Flags = PdfContentReaderFlags::SkipHandleNonFormXObjects;

// NACHHER:
args.Flags = PdfContentReaderFlags::None;
```

### Test-Ergebnis
```
✓ FOUND pH 1,5: 'ca. 1,5 (1%)'
pH 1,5 found:  YES ✓
pH 12,0 found: NO
```

**Erkenntnis:** PoDoFo hatte XObject/FormXObject-Unterstützung bereits vollständig implementiert, sie war nur per Flag deaktiviert. Durch Aktivierung werden ALLE PDF-Textformate unterstützt, einschließlich Text in FormXObjects.

### Betroffene Dateien
- `/home/fenno/Projects/flucture/documents/pdf/flx_pdf_text_extractor.cpp` - Eine Zeile geändert
- `/home/fenno/Projects/IbisSDBExtractor` - Neu kompiliert mit aktualisierter flucture-Version

---

## 🔴 NEUES PROBLEM ENTDECKT (2025-10-16 Fortsetzung)

**Das XObject-Problem ist NICHT vollständig gelöst!**

### Symptome
- `test_xobject_simple` extrahiert **75 Texte** von Seite 5 ✅ (inkl. "ca. 1,5")
- `pdf_to_layout` extrahiert nur **69 Texte** von Seite 5 ❌ (fehlt "ca. 1,5")
- Beide verwenden **denselben** Extraktionscode (`flx_pdf_text_extractor::extract_text_with_fonts()`)
- XObject-Flag ist auf `None` gesetzt (korrekt)

### Debugging-Erkenntnisse

#### 1. Doppelte Extraktion in flx_pdf_sio.cpp
Es gibt zwei separate Extraktionsphasen:
- **Phase 1 (Zeile 82-91):** `extract_texts_and_images()` - alle Texte in eine große Liste
- **Phase 2 (Zeile 169-186):** Per-Page Extraktion in `page_geom.texts`
- Phase 1 Ergebnisse werden **nicht verwendet** (waren für deaktivierte Geometrie-Pipeline gedacht)

#### 2. Static Font Cache Hypothese
- Globaler Font-Cache: `static std::unordered_map<std::string, const PdfFont*> g_font_cache`
- **Hypothese:** Cache enthält stale PdfFont* Pointer von Phase 1
- **Test:** Font-Cache vor Phase 2 geleert (`flx_pdf_text_extractor::clear_font_cache()`)
- **Ergebnis:** ❌ Problem bleibt - immer noch nur 69 Texte

#### 3. Extraktionsvergleich
```
test_xobject_simple:  75 Texte ✓ (inkl. "ca. 1,5 (1%)")
pdf_to_layout:        69 Texte ✗ (fehlt "ca. 1,5")
Differenz:            6 Texte fehlen (die XObject-Texte!)
```

### ✅ ROOT CAUSE GEFUNDEN! (2025-10-16 - Bisect-Analyse)

**Problem:** `LoadFromBuffer()` vs `Load()` Unterschied!

**Bisektionstest Ergebnisse:**
```
✅ TEST 1 (doc.Load(path)):           75 Texte - FUNKTIONIERT
❌ TEST 2 (flx_pdf_sio::parse()):     EOF Parse Error
💥 TEST 3 (doc.LoadFromBuffer()):     SEGFAULT (Exit 139)
```

**ROOT CAUSE:** PoDoFo's XObject-Processing greift **SPÄTER** auf den LoadFromBuffer-Puffer zu!

### Das fundamentale Problem

```cpp
// In flx_pdf_sio::parse() - VORHER (BROKEN):
bufferview buffer(data.c_str(), data.size());  // ❌ 'data' ist Parameter!
m_pdf->LoadFromBuffer(buffer);                  // PoDoFo speichert nur Pointer
// ... später bei XObject extraction:
// PoDoFo greift auf buffer zu → aber 'data' existiert nicht mehr → SEGFAULT/69 texts
```

**Warum Load() funktioniert:**
- `Load(path)` liest Datei und macht eigene Kopie im Speicher
- Alle Daten persistent verfügbar während PDF-Verarbeitung

**Warum LoadFromBuffer() scheitert:**
- PoDoFo speichert nur Pointer auf Buffer (für Performance)
- XObject-Content wird **lazy** geladen beim ersten Zugriff
- Buffer muss während **gesamter Lebensdauer** des PdfMemDocument gültig sein!

### Versuchter Fix (NICHT AUSREICHEND)

```cpp
// FIX-VERSUCH: Zeile 70 geändert
bufferview buffer(pdf_data.c_str(), pdf_data.size()); // ✅ Verwendet Member-Variable
```

**Ergebnis:** Immer noch 69 Texte! → Nicht ausreichend!

**Grund:** `pdf_data` ist `flx_string` (lazy_ptr), dessen c_str() Pointer sich ändern kann!

### ✅ WICHTIGE ERKENNTNIS: Load() vs LoadFromBuffer() ist NICHT das Problem!

**Bisektionstest mit Load():**
```
pdf_to_layout mit Load():           69 texts ❌
test_xobject_simple mit Load():     75 texts ✅
```

**FAZIT:** Beide verwenden jetzt Load(), aber das Ergebnis bleibt unterschiedlich!
→ Load/LoadFromBuffer ist **NICHT** der entscheidende Unterschied!

### Das eigentliche Problem liegt woanders

**Hypothesen:**
1. Loop durch alle Seiten modifiziert irgendetwas
2. Font cache zwischen Seiten-Extraktionen ✅ **DAS WAR ES!**
3. Multiple Extractor-Instanzen auf demselben PdfMemDocument
4. Reihenfolge-Problem (Seite 1-4 vor Seite 5)

---

## ✅✅ FINALE LÖSUNG (2025-10-16 - Font Cache Clearing)

**Problem endgültig gelöst durch Per-Page Font Cache Clearing!**

### Die Root Cause
Der **statische Font-Cache** (`g_font_cache` in `flx_pdf_text_extractor`) enthielt stale `PdfFont*` Pointer von vorherigen Seiten-Extraktionen. Beim Durchlaufen aller Seiten in einem Loop wurde der Cache kontaminiert, wodurch XObject-Text-Extraktion auf späteren Seiten fehlschlug.

### Die Lösung (3 Zeilen Code)
```cpp
// In flx_pdf_sio.cpp, Zeile 199-202:
// CRITICAL FIX: Clear font cache between pages to prevent XObject processing issues
// The static font cache can contain stale PdfFont* pointers from previous pages,
// causing XObject text extraction to fail on subsequent pages.
flx_pdf_text_extractor::clear_font_cache();
```

**Platzierung:** Direkt **VOR** jeder Page-Extraktion im Loop!

### Test-Ergebnisse
```
✅ VORHER (ohne fix):  Page 5 = 69 texts (XObject-Texte fehlen)
✅ NACHHER (mit fix):  Page 5 = 75 texts (ALLE Texte!)

Verification:
🔍 DEBUG Page 1: Extracted 82 texts
🔍 DEBUG Page 2: Extracted 116 texts
🔍 DEBUG Page 3: Extracted 79 texts
🔍 DEBUG Page 4: Extracted 85 texts
🔍 DEBUG Page 5: Extracted 75 texts ✅ ← GELÖST!
🔍 DEBUG Page 6: Extracted 94 texts
```

### Betroffene Dateien
- `/home/fenno/Projects/flucture/documents/pdf/flx_pdf_sio.cpp` - Font cache clearing hinzugefügt
- `/home/fenno/Projects/IbisSDBExtractor` - Automatisch behoben durch aktualisierte flucture-Version

### Warum Load/LoadFromBuffer nicht das Problem war
Frühere Hypothese war falsch: Beide Methoden zeigten dasselbe Problem (69 Texte). Der eigentliche Unterschied zwischen `test_xobject_simple` (funktioniert) und `pdf_to_layout` (kaputt) war der **Seiten-Loop** mit kontaminiertem Font-Cache!

### Bisektions-Methode Erfolg
Systematische Annäherung beider Code-Pfade führte zur Erkenntnis, dass der Loop selbst das Problem war. Font-Cache-Clearing zwischen Seiten löste es vollständig.

---

## 🎉 STATUS: VOLLSTÄNDIG GELÖST

**Alle XObject-Texte werden nun korrekt extrahiert!**
