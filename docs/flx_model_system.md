# flx_model Property System

## Overview

The flx_model system provides a sophisticated property-based data model with lazy loading, const-correctness, and exception-safe null handling. It's designed to work seamlessly with the flx_variant type system and supports both int literals and long long values without ambiguity.

## Core Components

### flx_lazy_ptr<T>
A smart pointer with lazy initialization and const-correct access patterns.

```cpp
flx_lazy_ptr<flxv_map> data;
(*data)["key"] = "value";  // Creates map if null
const auto& const_data = data;
// *const_data;  // Throws flx_null_access_exception if null
```

### flx_property<type>
Template-based property wrapper with automatic type conversion and null handling.

```cpp
class MyModel : public flx_model {
public:
    flxp_int(id);        // long long internally, accepts int literals
    flxp_string(name);   // flx_string property
    flxp_bool(active);   // boolean property
};
```

## Usage Examples

### Basic Property Access
```cpp
MyModel model;

// Assignment works with int literals
model.id = 42;           // Converts int to long long automatically
model.name = "Test";     // Direct string assignment
model.active = true;     // Boolean assignment

// Comparison with int literals (SFINAE magic)
if (model.id == 42) {    // Works seamlessly
    std::cout << "Found!";
}
```

### Const-Correct Access
```cpp
void processModel(const MyModel& model) {
    try {
        auto name = model.name.value();  // May throw if null
        std::cout << name;
    } catch (const flx_null_field_exception& e) {
        std::cout << "Field " << e.get_field_name() << " is null";
    }
}
```

### Null Checking
```cpp
MyModel model;
if (model.id.is_null()) {
    std::cout << "ID not set yet";
}

// Accessing creates default value
auto id_value = model.id.value();  // Creates 0 if null
assert(!model.id.is_null());       // Now false
```

### Reference Assignment
```cpp
MyModel model;
auto& id_ref = model.id;    // Gets reference to property
id_ref = 123;               // Modifies original property
assert(model.id == 123);    // True
```

## Technical Implementation

### Type Safety with SFINAE
The system uses template specialization to provide int comparison operators only for long long properties:

```cpp
// Only available for flx_property<long long>
template<typename T = type>
typename std::enable_if<std::is_same<T, long long>::value, bool>::type
operator==(int other) const;
```

### Exception Hierarchy
- `flx_null_access_exception`: Thrown by const lazy pointer access
- `flx_null_field_exception`: Thrown by const property access with field name

### Memory Management
- Properties use lazy loading via flx_lazy_ptr
- Reference counting ensures proper cleanup
- Default values created on first non-const access

## Property Macros

| Macro | Type | Description |
|-------|------|-------------|
| `flxp_int(name)` | `long long` | Integer property with int literal support |
| `flxp_string(name)` | `flx_string` | String property |
| `flxp_bool(name)` | `bool` | Boolean property |
| `flxp_double(name)` | `double` | Floating point property |
| `flxp_vector(name)` | `flxv_vector` | Vector property |
| `flxp_map(name)` | `flxv_map` | Map property |

## Best Practices

1. **Always check for null in const contexts** or use try/catch
2. **Use int literals freely** with integer properties
3. **Prefer `.value()` for explicit access** when needed
4. **Handle exceptions** in const access scenarios
5. **Use reference assignment** for efficient property updates

## Integration with flx_variant

The property system seamlessly integrates with the flx_variant type system:
- All properties backed by flx_variant storage
- Automatic type conversion through variant system
- JSON serialization support through variant conversion
- Cross-type compatibility maintained