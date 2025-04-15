# Gap Buffer

A modern C++ implementation of a gap buffer data structure, commonly used in text editors for efficient text insertion and deletion operations.

## Overview

The gap buffer is a dynamic array data structure that maintains a gap (an unused space) in memory that can be moved around to optimize insertions and deletions. This implementation provides a container interface similar to `std::vector` with Standard Library compatibility.

Key features:
- Efficient insertion and deletion operations at the cursor position
- STL-compatible iterators
- Full compliance with C++ container requirements
- Memory-efficient with minimal reallocation during typical edit operations
- Template-based implementation for any data type

## Requirements

- C++11 or later
- Standard Template Library

## Usage

### Basic Usage

```cpp
#include "gap_buffer.hpp"
#include <iostream>
#include <string>

int main() {
    // Create a gap buffer of integers
    gap_buffer<int> buffer = {1, 2, 3, 4, 5};
    
    // Access elements like a vector
    std::cout << "First element: " << buffer.front() << std::endl;
    std::cout << "Last element: " << buffer.back() << std::endl;
    std::cout << "Element at index 2: " << buffer[2] << std::endl;
    
    // Insert at position
    auto it = buffer.begin() + 2;
    buffer.insert(it, 10);
    
    // Print all elements
    std::cout << "After insertion: ";
    for (const auto& value : buffer) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
    
    // Erase elements
    buffer.erase(buffer.begin() + 1, buffer.begin() + 3);
    
    // Print after erasure
    std::cout << "After erasure: ";
    for (const auto& value : buffer) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

### Text Editor Example

```cpp
#include "gap_buffer.hpp"
#include <iostream>
#include <string>

class SimpleTextEditor {
private:
    gap_buffer<char> text;
    size_t cursor_position = 0;
    
public:
    // Insert text at cursor
    void insertText(const std::string& str) {
        auto it = text.begin() + cursor_position;
        text.insert(it, str.begin(), str.end());
        cursor_position += str.length();
    }
    
    // Delete n characters before cursor
    void backspace(size_t n = 1) {
        if (cursor_position >= n) {
            auto start = text.begin() + (cursor_position - n);
            auto end = text.begin() + cursor_position;
            text.erase(start, end);
            cursor_position -= n;
        }
    }
    
    // Delete n characters after cursor
    void deleteForward(size_t n = 1) {
        if (cursor_position < text.size()) {
            auto start = text.begin() + cursor_position;
            auto end = text.begin() + std::min(cursor_position + n, text.size());
            text.erase(start, end);
        }
    }
    
    // Move cursor
    void moveCursor(int offset) {
        int new_pos = static_cast<int>(cursor_position) + offset;
        if (new_pos >= 0 && new_pos <= static_cast<int>(text.size())) {
            cursor_position = static_cast<size_t>(new_pos);
        }
    }
    
    // Get text content
    std::string getContent() const {
        return std::string(text.begin(), text.end());
    }
    
    // Display text with cursor
    void display() const {
        std::string content = getContent();
        content.insert(cursor_position, "|");
        std::cout << content << std::endl;
    }
};

int main() {
    SimpleTextEditor editor;
    
    editor.insertText("Hello world");
    editor.display();  // Hello world|
    
    editor.moveCursor(-5);
    editor.display();  // Hello |world
    
    editor.insertText("beautiful ");
    editor.display();  // Hello beautiful |world
    
    editor.backspace(2);
    editor.display();  // Hello beautifu|world
    
    editor.deleteForward(3);
    editor.display();  // Hello beautifu|ld
    
    return 0;
}
```

## API Reference

### Constructor and Assignment

```cpp
// Default constructor
gap_buffer();

// Allocator constructor
explicit gap_buffer(const Allocator& alloc);

// Fill constructor
gap_buffer(size_type count, const T& value, const Allocator& alloc = Allocator());

// Count constructor
explicit gap_buffer(size_type count, const Allocator& alloc = Allocator());

// Range constructor
template <typename InputIt>
gap_buffer(InputIt first, InputIt last, const Allocator& alloc = Allocator());

// Copy constructor
gap_buffer(const gap_buffer& other);

// Move constructor
gap_buffer(gap_buffer&& other) noexcept;

// Initializer list constructor
gap_buffer(std::initializer_list<T> init, const Allocator& alloc = Allocator());

// Copy assignment
gap_buffer& operator=(const gap_buffer& other);

// Move assignment
gap_buffer& operator=(gap_buffer&& other) noexcept;

// Initializer list assignment
gap_buffer& operator=(std::initializer_list<T> ilist);

// Assign content
void assign(size_type count, const T& value);
template <typename InputIt>
void assign(InputIt first, InputIt last);
void assign(std::initializer_list<T> ilist);
```

### Element Access

```cpp
// Access with bounds checking
reference at(size_type pos);
const_reference at(size_type pos) const;

// Access without bounds checking
reference operator[](size_type pos);
const_reference operator[](size_type pos) const;

// Access first element
reference front();
const_reference front() const;

// Access last element
reference back();
const_reference back() const;

// Access underlying data
T* data() noexcept;
const T* data() const noexcept;
```

### Iterators

```cpp
iterator begin() noexcept;
const_iterator begin() const noexcept;
const_iterator cbegin() const noexcept;

iterator end() noexcept;
const_iterator end() const noexcept;
const_iterator cend() const noexcept;

reverse_iterator rbegin() noexcept;
const_reverse_iterator rbegin() const noexcept;
const_reverse_iterator crbegin() const noexcept;

reverse_iterator rend() noexcept;
const_reverse_iterator rend() const noexcept;
const_reverse_iterator crend() const noexcept;
```

### Capacity

```cpp
[[nodiscard]] bool empty() const noexcept;
size_type size() const noexcept;
size_type max_size() const noexcept;
void reserve(size_type new_cap);
size_type capacity() const noexcept;
void shrink_to_fit();
```

### Modifiers

```cpp
// Clear content
void clear() noexcept;

// Insert element
iterator insert(const_iterator pos, const T& value);
iterator insert(const_iterator pos, T&& value);
iterator insert(const_iterator pos, size_type count, const T& value);
template <typename InputIt>
iterator insert(const_iterator pos, InputIt first, InputIt last);
iterator insert(const_iterator pos, std::initializer_list<T> ilist);

// Construct element in-place
template <typename... Args>
iterator emplace(const_iterator pos, Args&&... args);

// Erase elements
iterator erase(const_iterator pos);
iterator erase(const_iterator first, const_iterator last);

// Add element at the end
void push_back(const T& value);
void push_back(T&& value);
template <typename... Args>
reference emplace_back(Args&&... args);

// Remove last element
void pop_back();

// Change size
void resize(size_type count);
void resize(size_type count, const value_type& value);

// Swap content
void swap(gap_buffer& other) noexcept;
```

### Non-member functions

```cpp
// Comparisons
template <typename T, typename Alloc>
bool operator==(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs);
template <typename T, typename Alloc>
bool operator!=(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs);
template <typename T, typename Alloc>
bool operator<(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs);
template <typename T, typename Alloc>
bool operator>(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs);
template <typename T, typename Alloc>
bool operator<=(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs);
template <typename T, typename Alloc>
bool operator>=(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs);

// Specialized swap
template <typename T, typename Alloc>
void swap(gap_buffer<T, Alloc>& lhs, gap_buffer<T, Alloc>& rhs) noexcept;
```

## Performance

The gap buffer offers the following performance characteristics:

- Insertion/deletion at the cursor position: O(1) amortized
- Insertion/deletion away from cursor: O(n) where n is distance from cursor
- Random access: O(1)
- Moving cursor: O(n) where n is distance from current position

This makes the gap buffer particularly well-suited for text editing operations where most edits occur near the cursor position.

## Implementation Details

The gap buffer maintains the following internal structure:
- A contiguous memory buffer
- A gap start position
- A gap end position

When insertions or deletions occur, the gap is first moved to the position of operation, then the operation is performed at the edge of the gap, minimizing memory moves.

## License

[MIT License](LICENSE)

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
