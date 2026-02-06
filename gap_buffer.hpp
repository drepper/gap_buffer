#ifndef GAP_BUFFER_HPP
#define GAP_BUFFER_HPP

#include <cstddef>
#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <memory>
#include <initializer_list>
#include <type_traits>
#include <string>
#include <vector>
#include <regex>
#include <functional>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>

template <typename T, typename Allocator = std::allocator<T>>
class gap_buffer {
protected:  // privateからprotectedに変更
    Allocator alloc;
    T* buffer;
    size_t gap_start;
    size_t gap_end;
    size_t buffer_size;

    // RAII helper for exception safety
    class buffer_guard {
    private:
        Allocator& alloc_;
        T* buffer_;
        size_t capacity_;
        size_t constructed_count_;
        
    public:
        buffer_guard(Allocator& alloc, T* buf, size_t cap) 
            : alloc_(alloc), buffer_(buf), capacity_(cap), constructed_count_(0) {}
	buffer_guard(const buffer_guard&) = delete;
	buffer_guard& operator=(const buffer_guard&) = delete;
        
        ~buffer_guard() {
            if (buffer_) {
                // Destroy constructed elements
                for (size_t i = 0; i < constructed_count_; ++i) {
                    std::allocator_traits<Allocator>::destroy(alloc_, buffer_ + i);
                }
                std::allocator_traits<Allocator>::deallocate(alloc_, buffer_, capacity_);
            }
        }
        
        void increment_constructed() { ++constructed_count_; }
        void release() { buffer_ = nullptr; }
        T* get() const { return buffer_; }
    };

    // Move the gap to a specified position
    void move_gap(size_t pos) {
        if (pos == gap_start) return;
        
        if (pos < gap_start) {
            // Move Gap Left
            size_t count = gap_start - pos;
            std::move_backward(buffer + pos, buffer + gap_start, buffer + gap_end);
            gap_end -= count;
            gap_start -= count;
        } else {
            // Move Gap Right
            size_t count = pos - gap_start;
            std::move(buffer + gap_end, buffer + gap_end + count, buffer + gap_start);
            gap_start += count;
            gap_end += count;
        }
    }

    // Increase the gap size with exception safety
    void grow(size_t min_capacity = 0) {
        size_t old_size = size();
        size_t new_capacity = buffer_size == 0 ? 16 : buffer_size * 2;
        if (min_capacity > 0 && new_capacity < min_capacity)
            new_capacity = min_capacity;
        
        T* new_buffer = alloc.allocate(new_capacity);
        buffer_guard guard(alloc, new_buffer, new_capacity);
        
        // Copy data from old buffer with exception safety
        if (buffer && old_size > 0) {
            // Copy elements before gap
            size_t before_gap = gap_start;
            for (size_t i = 0; i < before_gap; ++i) {
                std::allocator_traits<Allocator>::construct(alloc, new_buffer + i, std::move_if_noexcept(buffer[i]));
                guard.increment_constructed();
            }
            
            // Copy elements after gap
            size_t after_gap = buffer_size - gap_end;
            for (size_t i = 0; i < after_gap; ++i) {
                std::allocator_traits<Allocator>::construct(alloc, new_buffer + before_gap + i, 
                                                            std::move_if_noexcept(buffer[gap_end + i]));
                guard.increment_constructed();
            }
        }
        
        // Destroy old buffer
        if (buffer) {
            for (size_t i = 0; i < gap_start; ++i)
                std::allocator_traits<Allocator>::destroy(alloc, buffer + i);
            for (size_t i = gap_end; i < buffer_size; ++i)
                std::allocator_traits<Allocator>::destroy(alloc, buffer + i);
            std::allocator_traits<Allocator>::deallocate(alloc, buffer, buffer_size);
        }
        
        buffer = guard.get();
        guard.release();
        gap_end = gap_start + (new_capacity - old_size);
        buffer_size = new_capacity;
    }

public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    // Iterator implementation
    template <bool IsConst>
    class iterator_impl {
    private:
        friend class gap_buffer;
        using buffer_ptr = std::conditional_t<IsConst, const T*, T*>;
        buffer_ptr buffer;
        size_t pos;
        size_t gap_start;
        size_t gap_end;
        size_t buffer_size;
        
        iterator_impl(buffer_ptr buff, size_t p, size_t gs, size_t ge, size_t bs)
            : buffer(buff), pos(p), gap_start(gs), gap_end(ge), buffer_size(bs) {}
        
        size_t real_pos() const {
            return pos >= gap_start ? pos + (gap_end - gap_start) : pos;
        }
        
        void check_bounds() const {
            if (!buffer || pos > buffer_size - (gap_end - gap_start)) {
                throw std::out_of_range("gap_buffer iterator out of bounds");
            }
        }
        
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = std::conditional_t<IsConst, const T, T>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        
        iterator_impl() : buffer(nullptr), pos(0), gap_start(0), gap_end(0), buffer_size(0) {}
        
        // Conversion constructor from non-const to const
        template <bool IsConst2 = IsConst, typename = std::enable_if_t<IsConst2>>
        iterator_impl(const iterator_impl<false>& other)
            : buffer(other.buffer), pos(other.pos), gap_start(other.gap_start), 
              gap_end(other.gap_end), buffer_size(other.buffer_size) {}
        
        reference operator*() const {
            check_bounds();
            return buffer[real_pos()];
        }
        
        pointer operator->() const {
            check_bounds();
            return &buffer[real_pos()];
        }
        
        iterator_impl& operator++() {
            ++pos;
            return *this;
        }
        
        iterator_impl operator++(int) {
            iterator_impl tmp = *this;
            ++*this;
            return tmp;
        }
        
        iterator_impl& operator--() {
            --pos;
            return *this;
        }
        
        iterator_impl operator--(int) {
            iterator_impl tmp = *this;
            --*this;
            return tmp;
        }
        
        iterator_impl& operator+=(difference_type n) {
            pos += n;
            return *this;
        }
        
        iterator_impl operator+(difference_type n) const {
            iterator_impl tmp = *this;
            return tmp += n;
        }
        
        iterator_impl& operator-=(difference_type n) {
            pos -= n;
            return *this;
        }
        
        iterator_impl operator-(difference_type n) const {
            iterator_impl tmp = *this;
            return tmp -= n;
        }
        
        difference_type operator-(const iterator_impl& other) const {
            return static_cast<difference_type>(pos) - static_cast<difference_type>(other.pos);
        }
        
        reference operator[](difference_type n) const {
            return *(*this + n);
        }
        
        bool operator==(const iterator_impl& other) const {
            return buffer == other.buffer && pos == other.pos;
        }
        
        bool operator!=(const iterator_impl& other) const {
            return !(*this == other);
        }
        
        bool operator<(const iterator_impl& other) const {
            return buffer == other.buffer && pos < other.pos;
        }
        
        bool operator>(const iterator_impl& other) const {
            return other < *this;
        }
        
        bool operator<=(const iterator_impl& other) const {
            return !(other < *this);
        }
        
        bool operator>=(const iterator_impl& other) const {
            return !(*this < other);
        }
        
        template <bool>
        friend class iterator_impl;
    };

    using iterator = iterator_impl<false>;
    using const_iterator = iterator_impl<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Constructors
    gap_buffer() : alloc(), buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {}
    
    explicit gap_buffer(const Allocator& alloc_) 
        : alloc(alloc_), buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {}
    
    gap_buffer(size_type count, const T& value, const Allocator& alloc_ = Allocator())
        : alloc(alloc_), buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {
        assign(count, value);
    }
    
    explicit gap_buffer(size_type count, const Allocator& alloc_ = Allocator())
        : alloc(alloc_), buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {
        resize(count);
    }
    
    template <typename InputIt, typename = 
              std::enable_if_t<!std::is_integral<InputIt>::value>>
    gap_buffer(InputIt first, InputIt last, const Allocator& alloc_ = Allocator())
        : alloc(alloc_), buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {
        assign(first, last);
    }
    
    gap_buffer(const gap_buffer& other)
        : alloc(std::allocator_traits<Allocator>::select_on_container_copy_construction(other.alloc)),
          buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {
        assign(other.begin(), other.end());
    }
    
    gap_buffer(gap_buffer&& other) noexcept
        : alloc(std::move(other.alloc)), buffer(other.buffer), 
          gap_start(other.gap_start), gap_end(other.gap_end), buffer_size(other.buffer_size) {
        other.buffer = nullptr;
        other.gap_start = 0;
        other.gap_end = 0;
        other.buffer_size = 0;
    }
    
    gap_buffer(std::initializer_list<T> init, const Allocator& alloc_ = Allocator())
        : alloc(alloc_), buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {
        assign(init);
    }
    
    // Destructor
    ~gap_buffer() {
        clear();
        if (buffer) {
            alloc.deallocate(buffer, buffer_size);
        }
    }
    
    // Assignment operators
    gap_buffer& operator=(const gap_buffer& other) {
        if (this != &other) {
            gap_buffer tmp(other);
            swap(tmp);
        }
        return *this;
    }
    
    gap_buffer& operator=(gap_buffer&& other) noexcept {
        if (this != &other) {
            clear();
            if (buffer) {
                alloc.deallocate(buffer, buffer_size);
            }
            
            alloc = std::move(other.alloc);
            buffer = other.buffer;
            gap_start = other.gap_start;
            gap_end = other.gap_end;
            buffer_size = other.buffer_size;
            
            other.buffer = nullptr;
            other.gap_start = 0;
            other.gap_end = 0;
            other.buffer_size = 0;
        }
        return *this;
    }
    
    gap_buffer& operator=(std::initializer_list<T> ilist) {
        assign(ilist);
        return *this;
    }
    
    // assign methods
    void assign(size_type count, const T& value) {
        clear();
        if (count > 0) {
            if (buffer_size < count) {
                grow(count);
            }
            
            for (size_t i = 0; i < count; ++i) {
                std::allocator_traits<Allocator>::construct(alloc, buffer + i, value);
            }
            
            gap_start = count;
            gap_end = buffer_size;
        }
    }
    
    template <typename InputIt, typename = 
              std::enable_if_t<!std::is_integral<InputIt>::value>>
    void assign(InputIt first, InputIt last) {
        clear();
        insert(begin(), first, last);
    }
    
    void assign(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
    }
    
    // Get allocator
    allocator_type get_allocator() const noexcept {
        return alloc;
    }
    
    // Element access
    reference at(size_type pos) {
        if (pos >= size()) {
            throw std::out_of_range("gap_buffer::at");
        }
        return (*this)[pos];
    }
    
    const_reference at(size_type pos) const {
        if (pos >= size()) {
            throw std::out_of_range("gap_buffer::at");
        }
        return (*this)[pos];
    }
    
    reference operator[](size_type pos) {
        return pos >= gap_start ? buffer[pos + (gap_end - gap_start)] : buffer[pos];
    }
    
    const_reference operator[](size_type pos) const {
        return pos >= gap_start ? buffer[pos + (gap_end - gap_start)] : buffer[pos];
    }
    
    reference front() {
        if (empty()) {
            throw std::out_of_range("gap_buffer::front called on empty container");
        }
        return buffer[0];
    }
    
    const_reference front() const {
        if (empty()) {
            throw std::out_of_range("gap_buffer::front called on empty container");
        }
        return buffer[0];
    }
    
    reference back() {
        if (empty()) {
            throw std::out_of_range("gap_buffer::back called on empty container");
        }
        size_t last_pos = size() - 1;
        return (*this)[last_pos];
    }
    
    const_reference back() const {
        if (empty()) {
            throw std::out_of_range("gap_buffer::back called on empty container");
        }
        size_t last_pos = size() - 1;
        return (*this)[last_pos];
    }
    
    T* data() noexcept {
        if (empty()) return nullptr;
        move_gap(size());  // Move gap to end
        return buffer;
    }
    
    const T* data() const noexcept {
        if (empty()) return nullptr;
        // For const version, we cannot modify the buffer
        // Return nullptr to indicate non-contiguous data
        if (gap_start != 0 && gap_end != buffer_size) {
            return nullptr;
        }
        return buffer;
    }
    
    // Iterators
    iterator begin() noexcept {
        return iterator(buffer, 0, gap_start, gap_end, buffer_size);
    }
    
    const_iterator begin() const noexcept {
        return const_iterator(buffer, 0, gap_start, gap_end, buffer_size);
    }
    
    const_iterator cbegin() const noexcept {
        return const_iterator(buffer, 0, gap_start, gap_end, buffer_size);
    }
    
    iterator end() noexcept {
        return iterator(buffer, size(), gap_start, gap_end, buffer_size);
    }
    
    const_iterator end() const noexcept {
        return const_iterator(buffer, size(), gap_start, gap_end, buffer_size);
    }
    
    const_iterator cend() const noexcept {
        return const_iterator(buffer, size(), gap_start, gap_end, buffer_size);
    }
    
    reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }
    
    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }
    
    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }
    
    reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }
    
    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }
    
    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }
    
    // Capacity
    [[nodiscard]] bool empty() const noexcept {
        return size() == 0;
    }
    
    size_type size() const noexcept {
        return buffer_size - (gap_end - gap_start);
    }
    
    size_type max_size() const noexcept {
        return std::allocator_traits<Allocator>::max_size(alloc);
    }
    
    void reserve(size_type new_cap) {
        if (new_cap > capacity()) {
            grow(new_cap + (gap_end - gap_start));
        }
    }
    
    size_type capacity() const noexcept {
        return buffer_size - (gap_end - gap_start);
    }
    
    void shrink_to_fit() {
        if (size() < capacity()) {
            gap_buffer tmp(*this);
            swap(tmp);
        }
    }
    
    // Modifiers
    void clear() noexcept {
        if (buffer) {
            for (size_t i = 0; i < gap_start; ++i) {
                std::allocator_traits<Allocator>::destroy(alloc, buffer + i);
            }
            for (size_t i = gap_end; i < buffer_size; ++i) {
                std::allocator_traits<Allocator>::destroy(alloc, buffer + i);
            }
        }
        gap_start = 0;
        gap_end = buffer_size;
    }
    
    iterator insert(const_iterator pos, const T& value) {
        size_t position = pos.pos;
        move_gap(position);
        
        if (gap_end == gap_start) {
            grow();
        }
        
        std::allocator_traits<Allocator>::construct(alloc, buffer + gap_start, value);
        size_t old_gap_start = gap_start;
        ++gap_start;
        
        return iterator(buffer, old_gap_start, gap_start, gap_end, buffer_size);
    }
    
    iterator insert(const_iterator pos, T&& value) {
        size_t position = pos.pos;
        move_gap(position);
        
        if (gap_end == gap_start) {
            grow();
        }
        
        std::allocator_traits<Allocator>::construct(alloc, buffer + gap_start, std::move(value));
        size_t old_gap_start = gap_start;
        ++gap_start;
        
        return iterator(buffer, old_gap_start, gap_start, gap_end, buffer_size);
    }
    
    iterator insert(const_iterator pos, size_type count, const T& value) {
        if (count == 0) {
            return iterator(buffer, pos.pos, gap_start, gap_end, buffer_size);
        }
        
        size_t position = pos.pos;
        move_gap(position);
        
        if (gap_end - gap_start < count) {
            grow(buffer_size + count - (gap_end - gap_start));
        }
        
        size_t old_gap_start = gap_start;
        for (size_t i = 0; i < count; ++i) {
            std::allocator_traits<Allocator>::construct(alloc, buffer + gap_start, value);
            ++gap_start;
        }
        
        return iterator(buffer, old_gap_start, gap_start, gap_end, buffer_size);
    }
    
    template <typename InputIt, typename = 
              std::enable_if_t<!std::is_integral<InputIt>::value>>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        size_t position = pos.pos;
        move_gap(position);
        
        size_t count = std::distance(first, last);
        if (count == 0) {
            return iterator(buffer, position, gap_start, gap_end, buffer_size);
        }
        
        if (gap_end - gap_start < count) {
            grow(buffer_size + count - (gap_end - gap_start));
        }
        
        size_t old_gap_start = gap_start;
        for (auto it = first; it != last; ++it) {
            std::allocator_traits<Allocator>::construct(alloc, buffer + gap_start, *it);
            ++gap_start;
        }
        
        return iterator(buffer, old_gap_start, gap_start, gap_end, buffer_size);
    }
    
    iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }
    
    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        size_t position = pos.pos;
        move_gap(position);
        
        if (gap_end == gap_start) {
            grow();
        }
        
        std::allocator_traits<Allocator>::construct(alloc, buffer + gap_start, std::forward<Args>(args)...);
        size_t old_gap_start = gap_start;
        ++gap_start;
        
        return iterator(buffer, old_gap_start, gap_start, gap_end, buffer_size);
    }
    
    iterator erase(const_iterator pos) {
        return erase(pos, pos + 1);
    }
    
    iterator erase(const_iterator first, const_iterator last) {
        if (first == last) {
            return iterator(buffer, first.pos, gap_start, gap_end, buffer_size);
        }
        
        size_t position = first.pos;
        size_t count = last.pos - first.pos;
        
        move_gap(position);
        
        for (size_t i = 0; i < count && gap_end < buffer_size; ++i) {
            std::allocator_traits<Allocator>::destroy(alloc, buffer + gap_end);
            ++gap_end;
        }
        
        return iterator(buffer, position, gap_start, gap_end, buffer_size);
    }
    
    void push_back(const T& value) {
        move_gap(size());
        
        if (gap_end == gap_start) {
            grow();
        }
        
        std::allocator_traits<Allocator>::construct(alloc, buffer + gap_start, value);
        ++gap_start;
    }
    
    void push_back(T&& value) {
        move_gap(size());
        
        if (gap_end == gap_start) {
            grow();
        }
        
        std::allocator_traits<Allocator>::construct(alloc, buffer + gap_start, std::move(value));
        ++gap_start;
    }
    
    template <typename... Args>
    reference emplace_back(Args&&... args) {
        move_gap(size());
        
        if (gap_end == gap_start) {
            grow();
        }
        
        std::allocator_traits<Allocator>::construct(alloc, buffer + gap_start, std::forward<Args>(args)...);
        ++gap_start;
        
        return buffer[gap_start - 1];
    }
    
    void pop_back() {
        if (empty()) return;
        
        size_t last_pos = size() - 1;
        if (last_pos < gap_start) {
            // Element is before gap
            --gap_start;
            alloc.destroy(buffer + gap_start);
        } else {
            // Element is after gap
            size_t real_pos = last_pos + (gap_end - gap_start);
            alloc.destroy(buffer + real_pos);
            ++gap_end;
        }
    }
    
    void resize(size_type count) {
        size_type current_size = size();
        if (count > current_size) {
            // Enlarge
            reserve(count);
            move_gap(current_size);
            
            for (size_t i = current_size; i < count; ++i) {
                std::allocator_traits<Allocator>::construct(alloc, buffer + gap_start, T());
                ++gap_start;
            }
        } else if (count < current_size) {
            // Shrink
            size_t to_remove = current_size - count;
            
            // Remove elements from the end
            for (size_t i = 0; i < to_remove; ++i) {
                pop_back();
            }
        }
    }
    
    void resize(size_type count, const value_type& value) {
        size_type current_size = size();
        if (count > current_size) {
            // Enlarge
            reserve(count);
            move_gap(current_size);
            
            for (size_t i = current_size; i < count; ++i) {
                std::allocator_traits<Allocator>::construct(alloc, buffer + gap_start, value);
                ++gap_start;
            }
        } else if (count < current_size) {
            // Shrink
            resize(count);
        }
    }
    
    void swap(gap_buffer& other) noexcept {
        std::swap(buffer, other.buffer);
        std::swap(gap_start, other.gap_start);
        std::swap(gap_end, other.gap_end);
        std::swap(buffer_size, other.buffer_size);
        std::swap(alloc, other.alloc);
    }
    
    // Convert to string (for text_editor_buffer)
    std::string to_string() const {
        std::string result;
        result.reserve(size());
        
        for (size_t i = 0; i < size(); ++i) {
            result += (*this)[i];
        }
        
        return result;
    }
};

// Non-member functions
template <typename T, typename Alloc>
bool operator==(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs) {
    if (lhs.size() != rhs.size()) return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, typename Alloc>
bool operator!=(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs) {
    return !(lhs == rhs);
}

template <typename T, typename Alloc>
bool operator<(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, typename Alloc>
bool operator>(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs) {
    return rhs < lhs;
}

template <typename T, typename Alloc>
bool operator<=(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs) {
    return !(rhs < lhs);
}

template <typename T, typename Alloc>
bool operator>=(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs) {
    return !(lhs < rhs);
}

template <typename T, typename Alloc>
void swap(gap_buffer<T, Alloc>& lhs, gap_buffer<T, Alloc>& rhs) noexcept {
    lhs.swap(rhs);
}


// Text Editor Buffer class - specialization for char with cursor and line/column tracking
class text_editor_buffer : public gap_buffer<char> {
private:
    size_t cursor_pos;
    mutable std::vector<size_t> line_starts;
    mutable bool line_cache_valid;
    
    // Line cache management with better efficiency
    void update_line_cache() const {
        if (line_cache_valid) return;
        
        line_starts.clear();
        line_starts.reserve(std::max(size() / 50, size_t(1))); // Rough estimate
        line_starts.push_back(0);
        
        for (size_t i = 0; i < size(); ++i) {
            if ((*this)[i] == '\n') {
                line_starts.push_back(i + 1);
            }
        }
        
        line_cache_valid = true;
    }
    
    void invalidate_line_cache() {
        line_cache_valid = false;
    }
    
    // Enhanced UTF-8 validation
    static bool is_utf8_continuation(unsigned char byte) {
        return (byte & 0xC0) == 0x80;
    }
    
    static size_t utf8_char_length(unsigned char first_byte) {
        if (first_byte < 0x80) return 1;
        if ((first_byte & 0xE0) == 0xC0) return 2;
        if ((first_byte & 0xF0) == 0xE0) return 3;
        if ((first_byte & 0xF8) == 0xF0) return 4;
        return 0; // Invalid
    }

public:
    struct cursor_position {
        size_t line;
        size_t column;
        size_t absolute_pos;
        
        cursor_position(size_t l = 0, size_t c = 0, size_t a = 0) 
            : line(l), column(c), absolute_pos(a) {}
    };
    
    struct find_result {
        size_t position;
        size_t length;
        bool found;
        
        find_result(size_t p = 0, size_t l = 0, bool f = false)
            : position(p), length(l), found(f) {}
    };
    
    // Constructors
    text_editor_buffer() : gap_buffer<char>(), cursor_pos(0), line_starts(), line_cache_valid(false) {}
    
    explicit text_editor_buffer(const std::string& text) 
        : gap_buffer<char>(text.begin(), text.end()), cursor_pos(0), line_starts(), line_cache_valid(false) {}
    
    // Cursor position management
    size_t get_cursor_position() const noexcept {
        return cursor_pos;
    }
    
    void set_cursor_position(size_t pos) {
        cursor_pos = std::min(pos, size());
    }
    
    cursor_position get_cursor_line_column() const {
        update_line_cache();
        
        if (line_starts.empty()) {
            return cursor_position(0, cursor_pos, cursor_pos);
        }
        
        // Binary search for line
        auto it = std::upper_bound(line_starts.begin(), line_starts.end(), cursor_pos);
        if (it != line_starts.begin()) {
            --it;
        }
        
        size_t line = std::distance(line_starts.begin(), it);
        size_t column = cursor_pos - *it;
        
        return cursor_position(line, column, cursor_pos);
    }
    
    void set_cursor_line_column(size_t line, size_t column) {
        update_line_cache();
        
        if (line >= line_starts.size()) {
            cursor_pos = size();
            return;
        }
        
        size_t line_start = line_starts[line];
        size_t line_end = (line + 1 < line_starts.size()) ? 
                          line_starts[line + 1] - 1 : size();
        
        cursor_pos = std::min(line_start + column, line_end);
    }
    
    // Line information
    size_t get_line_count() const {
        update_line_cache();
        return line_starts.size();
    }
    
    size_t get_line_length(size_t line) const {
        update_line_cache();
        
        if (line >= line_starts.size()) return 0;
        
        size_t start = line_starts[line];
        size_t end = (line + 1 < line_starts.size()) ? 
                     line_starts[line + 1] - 1 : size();
        
        // Exclude newline character from length
        if (end > start && end <= size() && (*this)[end - 1] == '\n') {
            --end;
        }
        
        return end - start;
    }
    
    std::string get_line(size_t line) const {
        update_line_cache();
        
        if (line >= line_starts.size()) return "";
        
        size_t start = line_starts[line];
        size_t end = (line + 1 < line_starts.size()) ? 
                     line_starts[line + 1] - 1 : size();
        
        // Exclude newline character
        if (end > start && end <= size() && (*this)[end - 1] == '\n') {
            --end;
        }
        
        std::string result;
        result.reserve(end - start);
        
        for (size_t i = start; i < end; ++i) {
            result += (*this)[i];
        }
        
        return result;
    }
    
    // Text manipulation
    void insert_text(const std::string& text) {
        insert_text(cursor_pos, text);
    }
    
    void insert_text(size_t pos, const std::string& text) {
        if (text.empty()) return;
        
        auto it = begin() + pos;
        insert(it, text.begin(), text.end());
        
        if (pos <= cursor_pos) {
            cursor_pos += text.length();
        }
        
        invalidate_line_cache();
    }
    
    void delete_text(size_t pos, size_t count) {
        if (pos >= size() || count == 0) return;
        
        count = std::min(count, size() - pos);
        auto start_it = begin() + pos;
        auto end_it = start_it + count;
        
        erase(start_it, end_it);
        
        if (pos < cursor_pos) {
            cursor_pos = (cursor_pos >= pos + count) ? 
                         cursor_pos - count : pos;
        }
        
        invalidate_line_cache();
    }
    
    void replace_text(size_t pos, size_t count, const std::string& replacement) {
        delete_text(pos, count);
        insert_text(pos, replacement);
    }
    
    // Search functionality
    find_result find_text(const std::string& search_text, size_t start_pos = 0) const {
        if (search_text.empty() || start_pos >= size()) {
            return find_result(0, 0, false);
        }
        
        std::string buffer_text = to_string();
        size_t found_pos = buffer_text.find(search_text, start_pos);
        
        if (found_pos != std::string::npos) {
            return find_result(found_pos, search_text.length(), true);
        }
        
        return find_result(0, 0, false);
    }
    
    find_result find_text_reverse(const std::string& search_text, 
                                  size_t start_pos = std::string::npos) const {
        if (search_text.empty()) {
            return find_result(0, 0, false);
        }
        
        std::string buffer_text = to_string();
        if (start_pos == std::string::npos || start_pos >= buffer_text.length()) {
            start_pos = buffer_text.length() - 1;
        }
        
        size_t found_pos = buffer_text.rfind(search_text, start_pos);
        
        if (found_pos != std::string::npos) {
            return find_result(found_pos, search_text.length(), true);
        }
        
        return find_result(0, 0, false);
    }
    
    // Regular expression search
    find_result find_regex(const std::string& pattern, size_t start_pos = 0) const {
        if (pattern.empty() || start_pos >= size()) {
            return find_result(0, 0, false);
        }
        
        try {
            std::regex regex_pattern(pattern);
            std::string buffer_text = to_string();
            std::smatch match;
            
            // Use substr to avoid iterator compatibility issues
            std::string search_text = buffer_text.substr(start_pos);
            if (std::regex_search(search_text, match, regex_pattern)) {
                size_t found_pos = start_pos + match.position();
                return find_result(found_pos, match.length(), true);
            }
        } catch (const std::regex_error&) {
            // Return not found on regex error
        }
        
        return find_result(0, 0, false);
    }
    
    find_result find_regex_reverse(const std::string& pattern, 
                                   size_t start_pos = std::string::npos) const {
        if (pattern.empty()) {
            return find_result(0, 0, false);
        }
        
        try {
            std::regex regex_pattern(pattern);
            std::string buffer_text = to_string();
            
            if (start_pos == std::string::npos || start_pos >= buffer_text.length()) {
                start_pos = buffer_text.length();
            }
            
            // For reverse search, find all matches up to start_pos
            std::string search_text = buffer_text.substr(0, start_pos);
            std::smatch match;
            
            find_result last_result(0, 0, false);
            size_t search_start = 0;
            
            // Find all matches and keep the last one
            while (search_start < search_text.length()) {
                std::string remaining_text = search_text.substr(search_start);
                if (std::regex_search(remaining_text, match, regex_pattern)) {
                    last_result.position = search_start + match.position();
                    last_result.length = match.length();
                    last_result.found = true;
                    search_start = last_result.position + 1;
                } else {
                    break;
                }
            }
            
            return last_result;
        } catch (const std::regex_error&) {
            // Return not found on regex error
        }
        
        return find_result(0, 0, false);
    }
    
    // Replace functionality
    size_t replace_all(const std::string& search_text, const std::string& replacement) {
        if (search_text.empty()) return 0;
        
        size_t count = 0;
        size_t pos = 0;
        
        while (true) {
            find_result result = find_text(search_text, pos);
            if (!result.found) break;
            
            replace_text(result.position, result.length, replacement);
            pos = result.position + replacement.length();
            count++;
            
            // Prevent infinite loop
            if (replacement.empty() && pos == result.position) {
                pos++;
            }
            if (pos >= size()) break;
        }
        
        return count;
    }
    
    size_t replace_all_regex(const std::string& pattern, const std::string& replacement) {
        if (pattern.empty()) return 0;
        
        try {
            std::regex regex_pattern(pattern);
            std::string buffer_text = to_string();
            std::string original_text = buffer_text;
            std::string result = std::regex_replace(buffer_text, regex_pattern, replacement);
            
            if (result != original_text) {
                // Replace buffer contents
                clear();
                assign(result.begin(), result.end());
                invalidate_line_cache();
                
                // Estimate replacement count
                std::sregex_iterator iter(original_text.begin(), original_text.end(), regex_pattern);
                std::sregex_iterator end;
                return std::distance(iter, end);
            }
            
            return 0;
        } catch (const std::regex_error&) {
            return 0;
        }
    }
    
    // File operations
    bool load_from_file(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        try {
            // Get file size
            file.seekg(0, std::ios::end);
            std::streamsize file_size = file.tellg();
            file.seekg(0, std::ios::beg);
            
            if (file_size < 0) return false;
            
            // Clear buffer and read file
            clear();
            
            if (file_size > 0) {
                reserve(static_cast<size_t>(file_size));
                
                std::vector<char> temp_buffer(static_cast<size_t>(file_size));
                file.read(temp_buffer.data(), file_size);
                
                std::streamsize bytes_read = file.gcount();
                if (bytes_read > 0) {
                    assign(temp_buffer.begin(), temp_buffer.begin() + bytes_read);
                }
            }
            
            cursor_pos = 0;
            invalidate_line_cache();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    
    bool save_to_file(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        try {
            // Write gap buffer contents sequentially
            for (size_t i = 0; i < size(); ++i) {
                file.put((*this)[i]);
                if (file.fail()) return false;
            }
            
            return file.good();
        } catch (const std::exception&) {
            return false;
        }
    }
    
    // Line ending conversion
    enum class line_ending_type {
        LF,      // Unix/Linux/macOS (\n)
        CRLF,    // Windows (\r\n)
        CR       // Classic Mac (\r)
    };
    
    void convert_line_endings(line_ending_type target) {
        std::string buffer_text = to_string();
        std::string result;
        result.reserve(buffer_text.length() * 2); // Conservative estimate
        
        const char* target_ending;
        switch (target) {
            case line_ending_type::LF:   target_ending = "\n"; break;
            case line_ending_type::CRLF: target_ending = "\r\n"; break;
            case line_ending_type::CR:   target_ending = "\r"; break;
            default: return;
        }
        
        for (size_t i = 0; i < buffer_text.length(); ++i) {
            char ch = buffer_text[i];
            
            if (ch == '\r') {
                // Handle \r\n or standalone \r
                if (i + 1 < buffer_text.length() && buffer_text[i + 1] == '\n') {
                    // \r\n sequence
                    result += target_ending;
                    i++; // Skip the \n
                } else {
                    // Standalone \r
                    result += target_ending;
                }
            } else if (ch == '\n') {
                // Standalone \n
                result += target_ending;
            } else {
                result += ch;
            }
        }
        
        // Replace buffer contents
        clear();
        assign(result.begin(), result.end());
        invalidate_line_cache();
    }
    
    line_ending_type detect_line_ending() const {
        bool has_crlf = false;
        bool has_lf = false;
        bool has_cr = false;
        
        for (size_t i = 0; i < size(); ++i) {
            char ch = (*this)[i];
            
            if (ch == '\r') {
                if (i + 1 < size() && (*this)[i + 1] == '\n') {
                    has_crlf = true;
                } else {
                    has_cr = true;
                }
            } else if (ch == '\n') {
                has_lf = true;
            }
        }
        
        // Priority order: CRLF > LF > CR
        if (has_crlf) return line_ending_type::CRLF;
        if (has_lf) return line_ending_type::LF;
        if (has_cr) return line_ending_type::CR;
        
        // Default based on system
#ifdef _WIN32
        return line_ending_type::CRLF;
#else
        return line_ending_type::LF;
#endif
    }
    
    // Enhanced UTF-8 validation
    bool is_valid_utf8() const {
        for (size_t i = 0; i < size();) {
            unsigned char ch = static_cast<unsigned char>((*this)[i]);
            size_t char_len = utf8_char_length(ch);
            
            if (char_len == 0) return false; // Invalid first byte
            if (i + char_len > size()) return false; // Incomplete character
            
            // Check continuation bytes
            for (size_t j = 1; j < char_len; ++j) {
                if (!is_utf8_continuation(static_cast<unsigned char>((*this)[i + j]))) {
                    return false;
                }
            }
            
            // Additional validation for overlong encodings and invalid code points
            if (char_len > 1) {
                uint32_t code_point = 0;
                
                switch (char_len) {
                    case 2:
                        code_point = ((ch & 0x1F) << 6) | 
                                   (static_cast<unsigned char>((*this)[i + 1]) & 0x3F);
                        if (code_point < 0x80) return false; // Overlong
                        break;
                    case 3:
                        code_point = ((ch & 0x0F) << 12) | 
                                   ((static_cast<unsigned char>((*this)[i + 1]) & 0x3F) << 6) |
                                   (static_cast<unsigned char>((*this)[i + 2]) & 0x3F);
                        if (code_point < 0x800) return false; // Overlong
                        if (code_point >= 0xD800 && code_point <= 0xDFFF) return false; // Surrogate
                        break;
                    case 4:
                        code_point = ((ch & 0x07) << 18) | 
                                   ((static_cast<unsigned char>((*this)[i + 1]) & 0x3F) << 12) |
                                   ((static_cast<unsigned char>((*this)[i + 2]) & 0x3F) << 6) |
                                   (static_cast<unsigned char>((*this)[i + 3]) & 0x3F);
                        if (code_point < 0x10000) return false; // Overlong
                        if (code_point > 0x10FFFF) return false; // Beyond Unicode range
                        break;
                }
            }
            
            i += char_len;
        }
        
        return true;
    }
    
    // Cursor movement utilities
    void move_cursor_to_start() {
        cursor_pos = 0;
    }
    
    void move_cursor_to_end() {
        cursor_pos = size();
    }
    
    void move_cursor_up() {
        auto pos = get_cursor_line_column();
        if (pos.line > 0) {
            set_cursor_line_column(pos.line - 1, pos.column);
        }
    }
    
    void move_cursor_down() {
        auto pos = get_cursor_line_column();
        if (pos.line + 1 < get_line_count()) {
            set_cursor_line_column(pos.line + 1, pos.column);
        }
    }
    
    void move_cursor_left() {
        if (cursor_pos > 0) {
            cursor_pos--;
        }
    }
    
    void move_cursor_right() {
        if (cursor_pos < size()) {
            cursor_pos++;
        }
    }
    
    void move_cursor_line_start() {
        auto pos = get_cursor_line_column();
        set_cursor_line_column(pos.line, 0);
    }
    
    void move_cursor_line_end() {
        auto pos = get_cursor_line_column();
        size_t line_len = get_line_length(pos.line);
        set_cursor_line_column(pos.line, line_len);
    }
    
    // Word-based movement
    void move_cursor_word_left() {
        if (cursor_pos == 0) return;
        
        size_t pos = cursor_pos - 1;
        
        // Skip whitespace
        while (pos > 0 && std::isspace(static_cast<unsigned char>((*this)[pos]))) {
            pos--;
        }
        
        // Skip word characters
        while (pos > 0 && !std::isspace(static_cast<unsigned char>((*this)[pos - 1]))) {
            pos--;
        }
        
        cursor_pos = pos;
    }
    
    void move_cursor_word_right() {
        if (cursor_pos >= size()) return;
        
        size_t pos = cursor_pos;
        
        // Skip current word
        while (pos < size() && !std::isspace(static_cast<unsigned char>((*this)[pos]))) {
            pos++;
        }
        
        // Skip whitespace
        while (pos < size() && std::isspace(static_cast<unsigned char>((*this)[pos]))) {
            pos++;
        }
        
        cursor_pos = pos;
    }
    
    // Selection utilities
    std::string get_selection(size_t start, size_t end) const {
        if (start >= size() || end > size() || start >= end) {
            return "";
        }
        
        std::string result;
        result.reserve(end - start);
        
        for (size_t i = start; i < end; ++i) {
            result += (*this)[i];
        }
        
        return result;
    }
    
    // Debug utilities
    void print_debug_info() const {
        std::cout << "=== Text Editor Buffer Debug Info ===" << std::endl;
        std::cout << "Buffer size: " << size() << std::endl;
        std::cout << "Buffer capacity: " << capacity() << std::endl;
        std::cout << "Cursor position: " << cursor_pos << std::endl;
        std::cout << "Line count: " << get_line_count() << std::endl;
        
        auto pos = get_cursor_line_column();
        std::cout << "Cursor line: " << pos.line << ", column: " << pos.column << std::endl;
        
        std::cout << "Line ending type: ";
        switch (detect_line_ending()) {
            case line_ending_type::LF:
                std::cout << "LF (Unix)" << std::endl;
                break;
            case line_ending_type::CRLF:
                std::cout << "CRLF (Windows)" << std::endl;
                break;
            case line_ending_type::CR:
                std::cout << "CR (Classic Mac)" << std::endl;
                break;
        }
        
        std::cout << "UTF-8 valid: " << (is_valid_utf8() ? "Yes" : "No") << std::endl;
        std::cout << "Line cache valid: " << (line_cache_valid ? "Yes" : "No") << std::endl;
        
        // Show first few lines
        size_t line_count = get_line_count();
        std::cout << "First " << std::min(line_count, size_t(5)) << " lines:" << std::endl;
        for (size_t i = 0; i < std::min(line_count, size_t(5)); ++i) {
            std::cout << "  " << i << ": \"" << get_line(i) << "\"" << std::endl;
        }
        
        std::cout << "====================================" << std::endl;
    }
    
    // Performance statistics
    struct buffer_stats {
        size_t total_size;
        size_t gap_size;
        size_t capacity;
        double gap_ratio;
        size_t line_count;
        bool line_cache_valid;
        
        buffer_stats(size_t ts, size_t gs, size_t cap, size_t lc, bool lcv)
            : total_size(ts), gap_size(gs), capacity(cap), 
              gap_ratio(cap > 0 ? static_cast<double>(gs) / cap : 0.0),
              line_count(lc), line_cache_valid(lcv) {}
    };
    
    buffer_stats get_stats() const {
        size_t gap_size = gap_end - gap_start;
        return buffer_stats(size(), gap_size, buffer_size, 
                          get_line_count(), line_cache_valid);
    }
};

#endif // GAP_BUFFER_HPP
