# Debug: Hierarchical Search - Empty Reviews

## Problem
Test `test_semantic_search.cpp:558` schlägt fehl:
```cpp
REQUIRE(results[0].reviews.size() == 2);  // Expected: 2, Actual: 0
```

## Hypothesen (Ebenen)

### Level 1: Grundlegende Ursachen
- [ ] **H1.1**: Reviews existieren nicht in der DB
- [ ] **H1.2**: Reviews werden nicht in die DB geschrieben
- [ ] **H1.3**: SQL Query lädt reviews nicht
- [ ] **H1.4**: Reviews werden geladen aber nicht ins Model kopiert
- [ ] **H1.5**: Reviews werden kopiert aber gehen beim Assignment verloren

### Level 2: Spezifische Implementierung
(Wird gefüllt nach Level 1 Ausschluss)

## Erkenntnisse

### Test 1: FK-Persistierung (GELÖST ✅)
**Problem:** FK wurde nicht gesetzt weil Property-zu-Property Assignment nicht funktioniert
```cpp
review1.product_id = product.id;  // ❌ Setzt FK NICHT
review1.product_id = product.id.value();  // ✅ Funktioniert!
```

**DB Verification:**
```sql
SELECT id, product_id, review_text FROM test_semantic_reviews ORDER BY id DESC LIMIT 5;
 id  | product_id |     review_text
-----+------------+---------------------
 386 |       2051 | Amazing bass
 385 |       2051 | Great sound quality  # FK korrekt gesetzt!
```

**Schlussfolgerung:** H1.2 widerlegt - FK-Persistierung funktioniert korrekt mit `.value()`

### Test 2: search_hierarchical lädt Children nicht
**Status:** 🔍 Aktuelles Problem

Nach FK-Fix:
```
DB shows 2 reviews for product           # ✅ repo.search() findet beide
Results size: 1                           # ✅ Parent wird geladen
Result[0].reviews.size() = 0              # ❌ Children fehlen!
```

**Schlussfolgerung:** H1.3 oder H1.4 ist die Ursache - entweder SQL Query oder Model-Assignment

### Nächster Schritt
`search_hierarchical()` Code reviewen um zu verstehen warum Children nicht geladen werden
