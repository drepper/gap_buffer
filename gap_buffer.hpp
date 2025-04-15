#include <cstddef>
#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <memory>
#include <initializer_list>
#include <type_traits>

template <typename T, typename Allocator = std::allocator<T>>
class gap_buffer {
private:
    Allocator alloc;
    T* buffer;
    size_t gap_start;
    size_t gap_end;
    size_t buffer_size;

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

    // Increase the gap size
    void grow(size_t min_capacity = 0) {
        size_t old_size = size();
        size_t new_capacity = buffer_size == 0 ? 16 : buffer_size * 2;
        if (min_capacity > 0 && new_capacity < min_capacity)
            new_capacity = min_capacity;
        
        T* new_buffer = alloc.allocate(new_capacity);
        
        // Copy data from old buffer
        if (buffer) {
            // Copy element before gap
            if (gap_start > 0)
                std::uninitialized_copy(buffer, buffer + gap_start, new_buffer);
            
            // Copy elements after gap
            if (gap_end < buffer_size)
                std::uninitialized_copy(buffer + gap_end, buffer + buffer_size, 
                                        new_buffer + gap_start);
            
            // Free the old buffer
            for (size_t i = 0; i < gap_start; ++i)
                alloc.destroy(buffer + i);
            for (size_t i = gap_end; i < buffer_size; ++i)
                alloc.destroy(buffer + i);
            alloc.deallocate(buffer, buffer_size);
        }
        
        buffer = new_buffer;
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
        
        iterator_impl(buffer_ptr buff, size_t p, size_t gs, size_t ge)
            : buffer(buff), pos(p), gap_start(gs), gap_end(ge) {}
        
        size_t real_pos() const {
            return pos >= gap_start ? pos + (gap_end - gap_start) : pos;
        }
        
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = std::conditional_t<IsConst, const T, T>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        
        iterator_impl() = default;
        
        // Conversion constructor from non-const to const
        template <bool IsConst2 = IsConst, typename = std::enable_if_t<IsConst2>>
        iterator_impl(const iterator_impl<false>& other)
            : buffer(other.buffer), pos(other.pos), gap_start(other.gap_start), gap_end(other.gap_end) {}
        
        reference operator*() const {
            return buffer[real_pos()];
        }
        
        pointer operator->() const {
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
            return pos - other.pos;
        }
        
        reference operator[](difference_type n) const {
            return *(*this + n);
        }
        
        bool operator==(const iterator_impl& other) const {
            return pos == other.pos;
        }
        
        bool operator!=(const iterator_impl& other) const {
            return !(*this == other);
        }
        
        bool operator<(const iterator_impl& other) const {
            return pos < other.pos;
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
        
        // Add friends for member access between iterator classes
        template <bool>
        friend class iterator_impl;
    };

    using iterator = iterator_impl<false>;
    using const_iterator = iterator_impl<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Constructor
    gap_buffer() : buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {}
    
    explicit gap_buffer(const Allocator& alloc) 
        : alloc(alloc), buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {}
    
    gap_buffer(size_type count, const T& value, const Allocator& alloc = Allocator())
        : alloc(alloc), buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {
        assign(count, value);
    }
    
    explicit gap_buffer(size_type count, const Allocator& alloc = Allocator())
        : alloc(alloc), buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {
        resize(count);
    }
    
    template <typename InputIt, typename = 
              std::enable_if_t<!std::is_integral<InputIt>::value>>
    gap_buffer(InputIt first, InputIt last, const Allocator& alloc = Allocator())
        : alloc(alloc), buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {
        assign(first, last);
    }
    
    gap_buffer(const gap_buffer& other)
        : alloc(other.alloc), buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {
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
    
    gap_buffer(std::initializer_list<T> init, const Allocator& alloc = Allocator())
        : alloc(alloc), buffer(nullptr), gap_start(0), gap_end(0), buffer_size(0) {
        assign(init);
    }
    
    // Destructor
    ~gap_buffer() {
        if (buffer) {
            for (size_t i = 0; i < gap_start; ++i)
                alloc.destroy(buffer + i);
            for (size_t i = gap_end; i < buffer_size; ++i)
                alloc.destroy(buffer + i);
            alloc.deallocate(buffer, buffer_size);
        }
    }
    
    // Assignment operator
    gap_buffer& operator=(const gap_buffer& other) {
        if (this != &other) {
            clear();
            assign(other.begin(), other.end());
        }
        return *this;
    }
    
    gap_buffer& operator=(gap_buffer&& other) noexcept {
        if (this != &other) {
            clear();
            if (buffer)
                alloc.deallocate(buffer, buffer_size);
            
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
    
    // assign
    void assign(size_type count, const T& value) {
        clear();
        if (count > 0) {
            if (buffer_size < count)
                grow(count);
            
            for (size_t i = 0; i < count; ++i)
                alloc.construct(buffer + i, value);
            
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
        if (pos >= size())
            throw std::out_of_range("gap_buffer::at");
        if (pos >= gap_start)
            return buffer[pos + (gap_end - gap_start)];
        return buffer[pos];
    }
    
    const_reference at(size_type pos) const {
        if (pos >= size())
            throw std::out_of_range("gap_buffer::at");
        if (pos >= gap_start)
            return buffer[pos + (gap_end - gap_start)];
        return buffer[pos];
    }
    
    reference operator[](size_type pos) {
        if (pos >= gap_start)
            return buffer[pos + (gap_end - gap_start)];
        return buffer[pos];
    }
    
    const_reference operator[](size_type pos) const {
        if (pos >= gap_start)
            return buffer[pos + (gap_end - gap_start)];
        return buffer[pos];
    }
    
    reference front() {
        return buffer[0];
    }
    
    const_reference front() const {
        return buffer[0];
    }
    
    reference back() {
        return buffer[buffer_size - 1 - (gap_end - gap_start)];
    }
    
    const_reference back() const {
        return buffer[buffer_size - 1 - (gap_end - gap_start)];
    }
    
    T* data() noexcept {
        if (gap_start == 0 && gap_end == buffer_size)
            return nullptr;
        
        move_gap(size());  // Move gap to end
        return buffer;
    }
    
    const T* data() const noexcept {
        if (gap_start == 0 && gap_end == buffer_size)
            return nullptr;
        
        const_cast<gap_buffer*>(this)->move_gap(size());  // Move gap to end
        return buffer;
    }
    
    // Iterator
    iterator begin() noexcept {
        return iterator(buffer, 0, gap_start, gap_end);
    }
    
    const_iterator begin() const noexcept {
        return const_iterator(buffer, 0, gap_start, gap_end);
    }
    
    const_iterator cbegin() const noexcept {
        return const_iterator(buffer, 0, gap_start, gap_end);
    }
    
    iterator end() noexcept {
        return iterator(buffer, size(), gap_start, gap_end);
    }
    
    const_iterator end() const noexcept {
        return const_iterator(buffer, size(), gap_start, gap_end);
    }
    
    const_iterator cend() const noexcept {
        return const_iterator(buffer, size(), gap_start, gap_end);
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
        if (new_cap > buffer_size - (gap_end - gap_start)) {
            new_cap += gap_end - gap_start;  // Add capacity for gap
            if (new_cap > buffer_size)
                grow(new_cap);
        }
    }
    
    size_type capacity() const noexcept {
        return buffer_size - (gap_end - gap_start);
    }
    
    void shrink_to_fit() {
        if (size() < buffer_size) {
            gap_buffer tmp(*this);
            swap(tmp);
        }
    }
    
    // Modifier
    void clear() noexcept {
        if (buffer) {
            for (size_t i = 0; i < gap_start; ++i)
                alloc.destroy(buffer + i);
            for (size_t i = gap_end; i < buffer_size; ++i)
                alloc.destroy(buffer + i);
        }
        gap_start = 0;
        gap_end = buffer_size;
    }
    
    iterator insert(const_iterator pos, const T& value) {
        size_t position = pos.pos;
        move_gap(position);
        
        if (gap_end == gap_start)
            grow();
        
        alloc.construct(buffer + gap_start, value);
        size_t old_gap_start = gap_start;
        ++gap_start;
        
        return iterator(buffer, old_gap_start, gap_start, gap_end);
    }
    
    iterator insert(const_iterator pos, T&& value) {
        size_t position = pos.pos;
        move_gap(position);
        
        if (gap_end == gap_start)
            grow();
        
        alloc.construct(buffer + gap_start, std::move(value));
        size_t old_gap_start = gap_start;
        ++gap_start;
        
        return iterator(buffer, old_gap_start, gap_start, gap_end);
    }
    
    iterator insert(const_iterator pos, size_type count, const T& value) {
        if (count == 0)
            return iterator(buffer, pos.pos, gap_start, gap_end);
        
        size_t position = pos.pos;
        move_gap(position);
        
        if (gap_end - gap_start < count)
            grow(buffer_size + count - (gap_end - gap_start));
        
        size_t old_gap_start = gap_start;
        for (size_t i = 0; i < count; ++i) {
            alloc.construct(buffer + gap_start, value);
            ++gap_start;
        }
        
        return iterator(buffer, old_gap_start, gap_start, gap_end);
    }
    
    template <typename InputIt, typename = 
              std::enable_if_t<!std::is_integral<InputIt>::value>>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        size_t position = pos.pos;
        move_gap(position);
        
        size_t count = std::distance(first, last);
        if (count == 0)
            return iterator(buffer, position, gap_start, gap_end);
        
        if (gap_end - gap_start < count)
            grow(buffer_size + count - (gap_end - gap_start));
        
        size_t old_gap_start = gap_start;
        for (auto it = first; it != last; ++it) {
            alloc.construct(buffer + gap_start, *it);
            ++gap_start;
        }
        
        return iterator(buffer, old_gap_start, gap_start, gap_end);
    }
    
    // Overload that takes non-const iterator
    iterator insert(iterator pos, const T* first, const T* last) {
        // Now that iterator can be converted to const_iterator,
        // Redirect to existing method for const_iterator
        return insert(const_iterator(pos), first, last);
    }
    
    iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }
    
    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        size_t position = pos.pos;
        move_gap(position);
        
        if (gap_end == gap_start)
            grow();
        
        alloc.construct(buffer + gap_start, std::forward<Args>(args)...);
        size_t old_gap_start = gap_start;
        ++gap_start;
        
        return iterator(buffer, old_gap_start, gap_start, gap_end);
    }
    
    iterator erase(const_iterator pos) {
        return erase(pos, pos + 1);
    }
    
    iterator erase(const_iterator first, const_iterator last) {
        if (first == last)
            return iterator(buffer, first.pos, gap_start, gap_end);
        
        size_t position = first.pos;
        size_t count = last.pos - first.pos;
        
        move_gap(position);
        
        for (size_t i = 0; i < count; ++i) {
            if (gap_end < buffer_size) {
                alloc.destroy(buffer + gap_end);
                ++gap_end;
            }
        }
        
        return iterator(buffer, position, gap_start, gap_end);
    }
    
    void push_back(const T& value) {
        move_gap(size());
        
        if (gap_end == gap_start)
            grow();
        
        alloc.construct(buffer + gap_start, value);
        ++gap_start;
    }
    
    void push_back(T&& value) {
        move_gap(size());
        
        if (gap_end == gap_start)
            grow();
        
        alloc.construct(buffer + gap_start, std::move(value));
        ++gap_start;
    }
    
    template <typename... Args>
    reference emplace_back(Args&&... args) {
        move_gap(size());
        
        if (gap_end == gap_start)
            grow();
        
        alloc.construct(buffer + gap_start, std::forward<Args>(args)...);
        ++gap_start;
        
        return buffer[gap_start - 1];
    }
    
    void pop_back() {
        if (size() > 0) {
            if (gap_start > 0) {
                --gap_start;
                alloc.destroy(buffer + gap_start);
            } else {
                move_gap(0);
                --gap_start;
                alloc.destroy(buffer + gap_start);
            }
        }
    }
    
    void resize(size_type count) {
        size_type old_size = size();
        if (count > old_size) {
            // Enlarge
            reserve(count);
            move_gap(old_size);
            
            for (size_t i = old_size; i < count; ++i) {
                alloc.construct(buffer + gap_start, T());
                ++gap_start;
            }
        } else if (count < old_size) {
            // Shrink
            if (count <= gap_start) {
                // Delete data before gap
                for (size_t i = count; i < gap_start; ++i)
                    alloc.destroy(buffer + i);
                gap_start = count;
            } else {
                // Delete data after gap
                size_t to_remove = old_size - count;
                for (size_t i = buffer_size - to_remove; i < buffer_size; ++i)
                    alloc.destroy(buffer + i);
                gap_end = buffer_size - (old_size - count);
            }
        }
    }
    
    void resize(size_type count, const value_type& value) {
        size_type old_size = size();
        if (count > old_size) {
            // Enlarge
            reserve(count);
            move_gap(old_size);
            
            for (size_t i = old_size; i < count; ++i) {
                alloc.construct(buffer + gap_start, value);
                ++gap_start;
            }
        } else if (count < old_size) {
            // Shrink (same process as resize(count))
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
};

// Non-member function
template <typename T, typename Alloc>
bool operator==(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs) {
    if (lhs.size() != rhs.size())
        return false;
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
