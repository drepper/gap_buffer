#include "gap_buffer.hpp"
#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <sstream>

class benchmark_timer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    
public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double stop() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
        return duration.count() / 1000.0; // Convert to milliseconds
    }
};

class benchmark_suite {
private:
    std::mt19937 rng;
    
    std::string generate_random_string(size_t length) {
        std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 \n\t";
        std::uniform_int_distribution<> dis(0, chars.size() - 1);
        
        std::string result;
        result.reserve(length);
        
        for (size_t i = 0; i < length; ++i) {
            result += chars[dis(rng)];
        }
        
        return result;
    }
    
    std::vector<size_t> generate_random_positions(size_t count, size_t max_pos) {
        std::vector<size_t> positions;
        std::uniform_int_distribution<size_t> dis(0, max_pos);
        
        for (size_t i = 0; i < count; ++i) {
            positions.push_back(dis(rng));
        }
        
        return positions;
    }
    
    void print_header(const std::string& test_name) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "  " << test_name << std::endl;
        std::cout << std::string(60, '=') << std::endl;
    }
    
    void print_result(const std::string& operation, double gap_time, double vector_time) {
        double ratio = vector_time / gap_time;
        std::cout << std::left << std::setw(25) << operation 
                  << std::right << std::setw(10) << std::fixed << std::setprecision(3) 
                  << gap_time << " ms"
                  << std::setw(10) << vector_time << " ms"
                  << std::setw(8) << std::setprecision(2) << ratio << "x"
                  << (ratio > 1.0 ? " (faster)" : " (slower)") << std::endl;
    }
    
public:
    benchmark_suite() : rng(std::random_device{}()) {}
    
    // Basic operations benchmark
    void benchmark_basic_operations() {
        print_header("Basic Operations Benchmark");
        std::cout << std::left << std::setw(25) << "Operation" 
                  << std::right << std::setw(15) << "GapBuffer" 
                  << std::setw(15) << "std::vector" 
                  << std::setw(12) << "Ratio" << std::endl;
        std::cout << std::string(67, '-') << std::endl;
        
        const size_t test_size = 10000;
        const size_t operations = 1000;
        
        // Test push_back
        {
            benchmark_timer timer;
            gap_buffer<char> gb;
            
            timer.start();
            for (size_t i = 0; i < test_size; ++i) {
                gb.push_back('a');
            }
            double gap_time = timer.stop();
            
            std::vector<char> vec;
            timer.start();
            for (size_t i = 0; i < test_size; ++i) {
                vec.push_back('a');
            }
            double vector_time = timer.stop();
            
            print_result("push_back", gap_time, vector_time);
        }
        
        // Test insert at beginning
        {
            gap_buffer<char> gb;
            std::vector<char> vec;
            
            // Pre-populate
            for (size_t i = 0; i < test_size; ++i) {
                gb.push_back('a');
                vec.push_back('a');
            }
            
            benchmark_timer timer;
            
            timer.start();
            for (size_t i = 0; i < operations; ++i) {
                gb.insert(gb.begin(), 'b');
            }
            double gap_time = timer.stop();
            
            timer.start();
            for (size_t i = 0; i < operations; ++i) {
                vec.insert(vec.begin(), 'b');
            }
            double vector_time = timer.stop();
            
            print_result("insert_at_beginning", gap_time, vector_time);
        }
        
        // Test insert at middle
        {
            gap_buffer<char> gb;
            std::vector<char> vec;
            
            // Pre-populate
            for (size_t i = 0; i < test_size; ++i) {
                gb.push_back('a');
                vec.push_back('a');
            }
            
            benchmark_timer timer;
            
            timer.start();
            for (size_t i = 0; i < operations; ++i) {
                size_t pos = gb.size() / 2;
                gb.insert(gb.begin() + pos, 'b');
            }
            double gap_time = timer.stop();
            
            timer.start();
            for (size_t i = 0; i < operations; ++i) {
                size_t pos = vec.size() / 2;
                vec.insert(vec.begin() + pos, 'b');
            }
            double vector_time = timer.stop();
            
            print_result("insert_at_middle", gap_time, vector_time);
        }
        
        // Test random access
        {
            gap_buffer<char> gb;
            std::vector<char> vec;
            
            // Pre-populate
            for (size_t i = 0; i < test_size; ++i) {
                char c = 'a' + (i % 26);
                gb.push_back(c);
                vec.push_back(c);
            }
            
            auto positions = generate_random_positions(operations * 10, test_size - 1);
            benchmark_timer timer;
            
            volatile char sum = 0; // Prevent optimization
            
            timer.start();
            for (size_t pos : positions) {
                sum += gb[pos];
            }
            double gap_time = timer.stop();
            
            timer.start();
            for (size_t pos : positions) {
                sum += vec[pos];
            }
            double vector_time = timer.stop();
            
            print_result("random_access", gap_time, vector_time);
        }
    }
    
    // Text editor specific benchmark
    void benchmark_text_editor() {
        print_header("Text Editor Operations Benchmark");
        std::cout << std::left << std::setw(30) << "Operation" 
                  << std::right << std::setw(15) << "Time (ms)" 
                  << std::setw(20) << "Operations/sec" << std::endl;
        std::cout << std::string(65, '-') << std::endl;
        
        const size_t doc_size = 50000;
        const size_t operations = 1000;
        
        // Create large document
        text_editor_buffer buffer;
        std::string large_text = generate_random_string(doc_size);
        buffer.insert_text(large_text);
        
        // Test cursor movement
        {
            benchmark_timer timer;
            timer.start();
            
            for (size_t i = 0; i < operations; ++i) {
                buffer.move_cursor_to_start();
                buffer.move_cursor_to_end();
                buffer.set_cursor_position(buffer.size() / 2);
            }
            
            double time = timer.stop();
            double ops_per_sec = (operations * 3.0 * 1000.0) / time;
            
            std::cout << std::left << std::setw(30) << "cursor_movement"
                      << std::right << std::setw(15) << std::fixed << std::setprecision(3) << time
                      << std::setw(20) << std::fixed << std::setprecision(0) << ops_per_sec << std::endl;
        }
        
        // Test line operations
        {
            benchmark_timer timer;
            timer.start();
            
            for (size_t i = 0; i < operations; ++i) {
                size_t line_count = buffer.get_line_count();
                if (line_count > 0) {
                    size_t line = i % line_count;
                    buffer.get_line(line);
                    buffer.get_line_length(line);
                }
            }
            
            double time = timer.stop();
            double ops_per_sec = (operations * 2.0 * 1000.0) / time;
            
            std::cout << std::left << std::setw(30) << "line_operations"
                      << std::right << std::setw(15) << std::fixed << std::setprecision(3) << time
                      << std::setw(20) << std::fixed << std::setprecision(0) << ops_per_sec << std::endl;
        }
        
        // Test text insertion
        {
            text_editor_buffer test_buffer = buffer; // Copy for testing
            std::string insert_text = "Hello World!\n";
            
            benchmark_timer timer;
            timer.start();
            
            for (size_t i = 0; i < operations; ++i) {
                size_t pos = (i * 137) % test_buffer.size(); // Semi-random positions
                test_buffer.insert_text(pos, insert_text);
            }
            
            double time = timer.stop();
            double ops_per_sec = (operations * 1000.0) / time;
            
            std::cout << std::left << std::setw(30) << "text_insertion"
                      << std::right << std::setw(15) << std::fixed << std::setprecision(3) << time
                      << std::setw(20) << std::fixed << std::setprecision(0) << ops_per_sec << std::endl;
        }
        
        // Test text deletion
        {
            text_editor_buffer test_buffer = buffer; // Copy for testing
            
            benchmark_timer timer;
            timer.start();
            
            for (size_t i = 0; i < operations && test_buffer.size() > 100; ++i) {
                size_t pos = (i * 97) % (test_buffer.size() - 50); // Semi-random positions
                test_buffer.delete_text(pos, 10); // Delete 10 characters
            }
            
            double time = timer.stop();
            double ops_per_sec = (operations * 1000.0) / time;
            
            std::cout << std::left << std::setw(30) << "text_deletion"
                      << std::right << std::setw(15) << std::fixed << std::setprecision(3) << time
                      << std::setw(20) << std::fixed << std::setprecision(0) << ops_per_sec << std::endl;
        }
        
        // Test search
        {
            std::string search_terms[] = {"the", "and", "for", "are", "with"};
            
            benchmark_timer timer;
            timer.start();
            
            for (size_t i = 0; i < operations; ++i) {
                std::string term = search_terms[i % 5];
                buffer.find_text(term);
            }
            
            double time = timer.stop();
            double ops_per_sec = (operations * 1000.0) / time;
            
            std::cout << std::left << std::setw(30) << "text_search"
                      << std::right << std::setw(15) << std::fixed << std::setprecision(3) << time
                      << std::setw(20) << std::fixed << std::setprecision(0) << ops_per_sec << std::endl;
        }
    }
    
    // Memory usage benchmark
    void benchmark_memory_usage() {
        print_header("Memory Usage Analysis");
        
        const size_t sizes[] = {1000, 10000, 100000, 1000000};
        
        std::cout << std::left << std::setw(15) << "Size" 
                  << std::right << std::setw(15) << "GapBuffer" 
                  << std::setw(15) << "std::vector" 
                  << std::setw(15) << "Gap Ratio" << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        
        for (size_t size : sizes) {
            // Gap buffer
            gap_buffer<char> gb;
            for (size_t i = 0; i < size; ++i) {
                gb.push_back('a');
            }
            
            // Insert some elements at the beginning to create a gap
            for (size_t i = 0; i < 100; ++i) {
                gb.insert(gb.begin(), 'b');
            }
            
            auto stats = static_cast<text_editor_buffer*>(&gb) ? 
                        static_cast<text_editor_buffer&>(gb).get_stats() :
                        text_editor_buffer::buffer_stats(gb.size(), 0, gb.capacity(), 0, false);
            
            // Vector
            std::vector<char> vec;
            for (size_t i = 0; i < size + 100; ++i) {
                vec.push_back(i < 100 ? 'b' : 'a');
            }
            
            std::cout << std::left << std::setw(15) << size
                      << std::right << std::setw(15) << gb.capacity()
                      << std::setw(15) << vec.capacity()
                      << std::setw(14) << std::fixed << std::setprecision(3) << stats.gap_ratio
                      << "%" << std::endl;
        }
    }
    
    // Gap movement benchmark
    void benchmark_gap_movement() {
        print_header("Gap Movement Performance");
        
        const size_t buffer_size = 10000;
        const size_t movements = 1000;
        
        gap_buffer<char> gb;
        
        // Pre-populate buffer
        for (size_t i = 0; i < buffer_size; ++i) {
            gb.push_back('a' + (i % 26));
        }
        
        // Test different movement patterns
        std::vector<std::pair<std::string, std::vector<size_t>>> patterns = {
            {"Sequential Forward", {}},
            {"Sequential Backward", {}},
            {"Random", {}},
            {"Alternating Ends", {}}
        };
        
        // Generate patterns
        for (size_t i = 0; i < movements; ++i) {
            patterns[0].second.push_back(i % buffer_size);                    // Sequential forward
            patterns[1].second.push_back(buffer_size - 1 - (i % buffer_size)); // Sequential backward
            patterns[2].second.push_back(rng() % buffer_size);                // Random
            patterns[3].second.push_back(i % 2 == 0 ? 0 : buffer_size - 1);  // Alternating ends
        }
        
        std::cout << std::left << std::setw(20) << "Pattern" 
                  << std::right << std::setw(15) << "Time (ms)" 
                  << std::setw(20) << "Insertions/sec" << std::endl;
        std::cout << std::string(55, '-') << std::endl;
        
        for (const auto& pattern : patterns) {
            gap_buffer<char> test_gb = gb; // Copy for testing
            
            benchmark_timer timer;
            timer.start();
            
            for (size_t pos : pattern.second) {
                if (pos < test_gb.size()) {
                    test_gb.insert(test_gb.begin() + pos, 'x');
                }
            }
            
            double time = timer.stop();
            double ops_per_sec = (movements * 1000.0) / time;
            
            std::cout << std::left << std::setw(20) << pattern.first
                      << std::right << std::setw(15) << std::fixed << std::setprecision(3) << time
                      << std::setw(20) << std::fixed << std::setprecision(0) << ops_per_sec << std::endl;
        }
    }
    
    void run_all_benchmarks() {
        std::cout << "Gap Buffer Performance Benchmark Suite" << std::endl;
        std::cout << "=======================================" << std::endl;
        
        benchmark_basic_operations();
        benchmark_text_editor();
        benchmark_memory_usage();
        benchmark_gap_movement();
        
        std::cout << "\nBenchmark completed." << std::endl;
    }
};

int main() {
    benchmark_suite suite;
    suite.run_all_benchmarks();
    return 0;
}
