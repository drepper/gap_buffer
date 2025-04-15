#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include "gap_buffer.hpp" // Include your gap buffer implementation

// Timer class for benchmarking
class Timer {
private:
    using clock_t = std::chrono::high_resolution_clock;
    using second_t = std::chrono::duration<double, std::ratio<1>>;
    std::chrono::time_point<clock_t> start_time;

public:
    Timer() : start_time(clock_t::now()) {}
    
    void reset() {
        start_time = clock_t::now();
    }
    
    double elapsed() const {
        return std::chrono::duration_cast<second_t>(clock_t::now() - start_time).count();
    }
};

// Template wrapper for data structures to provide a common interface for benchmarking
template <typename Container>
class EditorDataStructure {
public:
    virtual ~EditorDataStructure() = default;
    
    virtual void insertAtCursor(char c) = 0;
    virtual void insertStringAtCursor(const std::string& str) = 0;
    virtual void eraseAtCursor() = 0;
    virtual void eraseRangeAtCursor(size_t count) = 0;
    virtual void moveCursor(int offset) = 0;
    virtual size_t getCursorPosition() const = 0;
    virtual size_t size() const = 0;
    virtual std::string getText() const = 0;
};

// Implementation for gap_buffer
class GapBufferEditor : public EditorDataStructure<gap_buffer<char>> {
private:
    gap_buffer<char> buffer;
    size_t cursor_pos;

public:
    GapBufferEditor() : buffer(), cursor_pos(0) {}
    
    void insertAtCursor(char c) override {
        auto it = buffer.begin() + cursor_pos;
        buffer.insert(it, c);
        cursor_pos++;
    }
    
    void insertStringAtCursor(const std::string& str) override {
        auto it = buffer.begin() + cursor_pos;
        buffer.insert(it, str.begin(), str.end());
        cursor_pos += str.length();
    }
    
    void eraseAtCursor() override {
        if (cursor_pos > 0) {
            auto it = buffer.begin() + (cursor_pos - 1);
            buffer.erase(it);
            cursor_pos--;
        }
    }
    
    void eraseRangeAtCursor(size_t count) override {
        if (cursor_pos > 0 && count > 0) {
            size_t start_pos = cursor_pos > count ? cursor_pos - count : 0;
            auto start_it = buffer.begin() + start_pos;
            auto end_it = buffer.begin() + cursor_pos;
            buffer.erase(start_it, end_it);
            cursor_pos = start_pos;
        }
    }
    
    void moveCursor(int offset) override {
        int new_pos = static_cast<int>(cursor_pos) + offset;
        if (new_pos >= 0 && new_pos <= static_cast<int>(buffer.size())) {
            cursor_pos = static_cast<size_t>(new_pos);
        }
    }
    
    size_t getCursorPosition() const override {
        return cursor_pos;
    }
    
    size_t size() const override {
        return buffer.size();
    }
    
    std::string getText() const override {
        return std::string(buffer.begin(), buffer.end());
    }
};

// Implementation for std::vector
class VectorEditor : public EditorDataStructure<std::vector<char>> {
private:
    std::vector<char> buffer;
    size_t cursor_pos;

public:
    VectorEditor() : buffer(), cursor_pos(0) {}
    
    void insertAtCursor(char c) override {
        buffer.insert(buffer.begin() + cursor_pos, c);
        cursor_pos++;
    }
    
    void insertStringAtCursor(const std::string& str) override {
        buffer.insert(buffer.begin() + cursor_pos, str.begin(), str.end());
        cursor_pos += str.length();
    }
    
    void eraseAtCursor() override {
        if (cursor_pos > 0) {
            buffer.erase(buffer.begin() + (cursor_pos - 1));
            cursor_pos--;
        }
    }
    
    void eraseRangeAtCursor(size_t count) override {
        if (cursor_pos > 0 && count > 0) {
            size_t start_pos = cursor_pos > count ? cursor_pos - count : 0;
            buffer.erase(buffer.begin() + start_pos, buffer.begin() + cursor_pos);
            cursor_pos = start_pos;
        }
    }
    
    void moveCursor(int offset) override {
        int new_pos = static_cast<int>(cursor_pos) + offset;
        if (new_pos >= 0 && new_pos <= static_cast<int>(buffer.size())) {
            cursor_pos = static_cast<size_t>(new_pos);
        }
    }
    
    size_t getCursorPosition() const override {
        return cursor_pos;
    }
    
    size_t size() const override {
        return buffer.size();
    }
    
    std::string getText() const override {
        return std::string(buffer.begin(), buffer.end());
    }
};

// Benchmark class
template <typename T>
class Benchmark {
private:
    T editor;
    std::mt19937 rng;
    std::uniform_int_distribution<int> char_dist;
    std::uniform_int_distribution<int> operation_dist;
    std::uniform_int_distribution<int> movement_dist;
    std::uniform_int_distribution<int> count_dist;
    
    // Test scenarios
    void sequential_insert(int count) {
        for (int i = 0; i < count; i++) {
            char c = static_cast<char>('a' + (char_dist(rng) % 26));
            editor.insertAtCursor(c);
        }
    }
    
    void random_edits(int count) {
        for (int i = 0; i < count; i++) {
            int op = operation_dist(rng) % 4;
            
            switch (op) {
                case 0: { // Insert single character
                    char c = static_cast<char>('a' + (char_dist(rng) % 26));
                    editor.insertAtCursor(c);
                    break;
                }
                case 1: { // Delete single character
                    editor.eraseAtCursor();
                    break;
                }
                case 2: { // Move cursor
                    int offset = movement_dist(rng) % 21 - 10; // -10 to +10
                    editor.moveCursor(offset);
                    break;
                }
                case 3: { // Insert word
                    std::string word = "lorem";
                    editor.insertStringAtCursor(word);
                    break;
                }
            }
        }
    }
    
    void cursor_movement_intensive(int count) {
        // First insert some text
        std::string text = "The quick brown fox jumps over the lazy dog. ";
        for (int i = 0; i < 10; i++) {
            editor.insertStringAtCursor(text);
        }
        
        // Now do a lot of cursor movements with occasional edits
        for (int i = 0; i < count; i++) {
            int op = operation_dist(rng) % 10;
            
            if (op < 8) { // 80% cursor movement
                int offset = movement_dist(rng) % 51 - 25; // -25 to +25
                editor.moveCursor(offset);
            } else if (op == 8) { // 10% insert
                editor.insertAtCursor('X');
            } else { // 10% delete
                editor.eraseAtCursor();
            }
        }
    }
    
    void bulk_insert_delete(int count) {
        for (int i = 0; i < count; i++) {
            int op = operation_dist(rng) % 2;
            
            if (op == 0) { // Insert string
                std::string text = "This is a test of bulk operations. ";
                editor.insertStringAtCursor(text);
            } else { // Delete range
                int deleteCount = count_dist(rng) % 10 + 1; // 1 to 10
                editor.eraseRangeAtCursor(deleteCount);
            }
        }
    }
    
public:
    Benchmark() 
        : rng(std::random_device{}()), 
          char_dist(0, 255),
          operation_dist(0, 3),
          movement_dist(-50, 50),
          count_dist(1, 20) {}
    
    struct BenchmarkResult {
        double sequential_insert_time;
        double random_edits_time;
        double cursor_movement_time;
        double bulk_operations_time;
        size_t final_size;
    };
    
    BenchmarkResult run(int operations_count) {
        BenchmarkResult result;
        Timer timer;
        
        // Reset state
        editor = T();
        
        // Sequential insert benchmark
        timer.reset();
        sequential_insert(operations_count);
        result.sequential_insert_time = timer.elapsed();
        
        // Random edits benchmark
        timer.reset();
        random_edits(operations_count);
        result.random_edits_time = timer.elapsed();
        
        // Reset state
        editor = T();
        
        // Cursor movement intensive benchmark
        timer.reset();
        cursor_movement_intensive(operations_count);
        result.cursor_movement_time = timer.elapsed();
        
        // Reset state
        editor = T();
        
        // Bulk operations benchmark
        timer.reset();
        bulk_insert_delete(operations_count / 10);  // Fewer operations as they're more intensive
        result.bulk_operations_time = timer.elapsed();
        
        result.final_size = editor.size();
        
        return result;
    }
};

void print_results(int operations, const typename Benchmark<GapBufferEditor>::BenchmarkResult& gap_results, 
                   const typename Benchmark<VectorEditor>::BenchmarkResult& vec_results) {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "===== Benchmark Results (" << operations << " operations) =====\n\n";
    
    std::cout << std::setw(25) << "Operation" << " | " 
              << std::setw(15) << "gap_buffer (s)" << " | " 
              << std::setw(15) << "std::vector (s)" << " | " 
              << std::setw(15) << "Ratio (vec/gap)" << "\n";
    std::cout << std::string(78, '-') << "\n";
    
    auto print_row = [](const std::string& name, double gap_time, double vec_time) {
        double ratio = vec_time / gap_time;
        std::cout << std::setw(25) << name << " | " 
                  << std::setw(15) << gap_time << " | " 
                  << std::setw(15) << vec_time << " | " 
                  << std::setw(15) << ratio << "\n";
    };
    
    print_row("Sequential Insert", gap_results.sequential_insert_time, vec_results.sequential_insert_time);
    print_row("Random Edits", gap_results.random_edits_time, vec_results.random_edits_time);
    print_row("Cursor Movement", gap_results.cursor_movement_time, vec_results.cursor_movement_time);
    print_row("Bulk Operations", gap_results.bulk_operations_time, vec_results.bulk_operations_time);
    
    double gap_total = gap_results.sequential_insert_time + gap_results.random_edits_time + 
                       gap_results.cursor_movement_time + gap_results.bulk_operations_time;
    
    double vec_total = vec_results.sequential_insert_time + vec_results.random_edits_time + 
                       vec_results.cursor_movement_time + vec_results.bulk_operations_time;
    
    std::cout << std::string(78, '-') << "\n";
    print_row("Total", gap_total, vec_total);
    std::cout << "\nFinal size (gap_buffer): " << gap_results.final_size << "\n";
    std::cout << "Final size (std::vector): " << vec_results.final_size << "\n";
    
    std::cout << "\nSummary: ";
    if (vec_total / gap_total > 1.1) {
        std::cout << "gap_buffer is " << std::setprecision(2) << (vec_total / gap_total) 
                  << "x faster overall\n";
    } else if (gap_total / vec_total > 1.1) {
        std::cout << "std::vector is " << std::setprecision(2) << (gap_total / vec_total) 
                  << "x faster overall\n";
    } else {
        std::cout << "Performance is roughly equivalent\n";
    }
}

int main(int argc, char* argv[]) {
    int operations = 100000;
    if (argc > 1) {
        operations = std::stoi(argv[1]);
    }
    
    std::cout << "Running benchmarks with " << operations << " operations...\n";
    
    Benchmark<GapBufferEditor> gap_benchmark;
    auto gap_results = gap_benchmark.run(operations);
    
    Benchmark<VectorEditor> vec_benchmark;
    auto vec_results = vec_benchmark.run(operations);
    
    print_results(operations, gap_results, vec_results);
    
    return 0;
}
