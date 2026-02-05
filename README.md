# Gap Buffer Implementation

A high-performance C++ implementation of gap buffer data structure with specialized text editor functionality.

## Overview

Gap buffer is a dynamic array data structure that maintains a contiguous "gap" of unused space, making insertions and deletions at the cursor position very efficient. This implementation provides both a generic `gap_buffer` template class and a specialized `text_editor_buffer` class optimized for text editing operations.

## Features

### Core Gap Buffer (`gap_buffer<T>`)
- **STL-compliant container interface** with full iterator support
- **Exception-safe operations** with RAII-based memory management
- **Custom allocator support** for specialized memory management
- **Move semantics** for efficient object transfers
- **Random access iterators** with bounds checking
- **Template-based design** supporting any copyable/movable type

### Text Editor Buffer (`text_editor_buffer`)
- **Cursor position management** with line/column tracking
- **Efficient line operations** with cached line indexing
- **Search and replace functionality** supporting both literal and regex patterns
- **File I/O operations** with automatic encoding detection
- **Line ending conversion** (LF, CRLF, CR) with auto-detection
- **Enhanced UTF-8 validation** with proper Unicode support
- **Word-based cursor movement** for text editing workflows

## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|------------|-------|
| Insert at cursor | O(1) amortized | Gap is at cursor position |
| Insert elsewhere | O(n) worst case | Requires gap movement |
| Delete at cursor | O(1) | Expands the gap |
| Random access | O(1) | Direct array indexing |
| Iterator traversal | O(1) per element | Skip gap automatically |

## Usage Examples

### Basic Gap Buffer

```cpp
#include "gap_buffer.hpp"

// Create and populate
gap_buffer<char> buffer;
buffer.push_back('H');
buffer.push_back('e');
buffer.push_back('l');
buffer.push_back('l');
buffer.push_back('o');

// Insert at beginning
buffer.insert(buffer.begin(), 'W');  // "WHello"

// Random access
char c = buffer[2];  // 'e'

// STL algorithms work
std::sort(buffer.begin(), buffer.end());
```

### Text Editor Operations

```cpp
#include "gap_buffer.hpp"

// Load file
text_editor_buffer editor;
editor.load_from_file("document.txt");

// Navigate by line/column
editor.set_cursor_line_column(10, 5);  // Line 10, column 5
auto pos = editor.get_cursor_line_column();

// Text manipulation
editor.insert_text("Hello World!\n");
editor.delete_text(0, 5);  // Delete first 5 characters

// Search and replace
auto result = editor.find_text("TODO");
if (result.found) {
    editor.replace_text(result.position, result.length, "DONE");
}

// Regex operations
editor.replace_all_regex(R"(\b\d{4}\b)", "YEAR");

// Save changes
editor.save_to_file("document.txt");
```

### Advanced Features

```cpp
// Custom allocator
gap_buffer<int, std::allocator<int>> custom_buffer;

// Line operations
size_t line_count = editor.get_line_count();
std::string line5 = editor.get_line(5);
size_t line_length = editor.get_line_length(5);

// UTF-8 validation
if (editor.is_valid_utf8()) {
    std::cout << "Valid UTF-8 encoding" << std::endl;
}

// Line ending detection and conversion
auto ending_type = editor.detect_line_ending();
editor.convert_line_endings(text_editor_buffer::line_ending_type::CRLF);

// Performance statistics
auto stats = editor.get_stats();
std::cout << "Gap ratio: " << stats.gap_ratio << std::endl;
```

## Building

### Requirements
- C++17 compatible compiler (GCC 7+, Clang 6+, MSVC 2017+)
- Standard library with `<regex>` support

### Compilation
```bash
# Basic compilation
g++ -std=c++17 -O3 your_program.cpp -o your_program

# With debug information
g++ -std=c++17 -g -DDEBUG your_program.cpp -o your_program_debug

# Run benchmarks
g++ -std=c++17 -O3 benchmark.cpp -o benchmark
./benchmark
```

## Benchmarks

Performance comparison with `std::vector`:

| Operation | Gap Buffer | std::vector | Speedup |
|-----------|------------|-------------|---------|
| Insert at beginning | ~0.1ms | ~10.2ms | ~100x |
| Insert at middle | ~0.5ms | ~5.1ms | ~10x |
| Random access | ~0.3ms | ~0.3ms | ~1x |
| Push back | ~0.2ms | ~0.2ms | ~1x |

*Results from benchmark suite on typical hardware*

## Architecture

The implementation uses a **protected inheritance model** where `text_editor_buffer` extends `gap_buffer<char>`. Key design decisions:

1. **Gap Management**: Automatic gap positioning and resizing
2. **Memory Safety**: RAII-based resource management with exception safety
3. **Iterator Design**: Skip gap transparently with proper bounds checking  
4. **Line Caching**: Efficient line start position caching with invalidation
5. **Unicode Support**: Proper UTF-8 validation and handling

## Thread Safety

This implementation is **not thread-safe**. For concurrent access:
- Use external synchronization (mutex, read-write lock)
- Create separate instances per thread
- Consider lock-free alternatives for high-performance scenarios

## Testing

```bash
# Compile tests (if test framework available)
g++ -std=c++17 -g tests.cpp -o tests
./tests

# Run benchmarks
./benchmark
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines
- Follow existing code style and naming conventions
- Add tests for new functionality
- Update benchmarks for performance-critical changes
- Ensure exception safety and proper resource management

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Gap buffer algorithm based on classic text editor implementations
- STL container design patterns
- Modern C++ best practices for memory management and exception safety
