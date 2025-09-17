# Changelog

## [Unreleased] - 2025-01-17

### Added
- Exception-based const-correct property system for flx_model
- SFINAE-enabled int comparison operators for long long properties
- Comprehensive test suite with Catch2 auto-discovery
- flx_null_field_exception and flx_null_access_exception for robust error handling

### Changed
- **BREAKING**: Refactored flx_object to flx_lazy_ptr with const-correctness
- **BREAKING**: Updated flx_variant to use long long internally while supporting int literals
- Enhanced flx_string with long long constructor for disambiguation
- Improved CMakeLists.txt with English comments and proper test discovery
- Updated flx_model property system with template-based type safety

### Fixed
- Const-correctness issues in lazy pointer access patterns
- Type ambiguity between int and long long in variant system
- String concatenation operator for const char* + flx_string
- Compilation errors in JSON processing and snippet handling

### Technical Details
- flx_property<long long> now supports `property == 42` (int literal comparison)
- Non-const property access creates default values automatically
- Const property access throws flx_null_field_exception when null
- All integer properties store as long long internally for sufficient range
- Template specialization ensures type safety without runtime overhead

### Migration Guide
- Replace `flx_object<T>` with `flx_lazy_ptr<T>`
- Handle potential flx_null_field_exception in const contexts
- No changes needed for int literal usage with properties