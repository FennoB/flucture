#pragma once

#include "flx_pdf_text_extractor.h"
#include <unordered_map>
#include <vector>
#include <memory_resource>

// Fast PDF text extractor using memory pools and cache-friendly data structures
class flx_pdf_text_extractor_fast {
private:
    // Pre-allocated memory pools for hot-path allocations
    struct MemoryPools {
        std::vector<char> string_pool;
        std::vector<double> lengths_pool;
        std::vector<unsigned> positions_pool;
        std::vector<flx_layout_text> text_pool;
        
        size_t string_offset = 0;
        size_t lengths_offset = 0;
        size_t positions_offset = 0;
        size_t text_offset = 0;
        
        void reset() {
            string_offset = 0;
            lengths_offset = 0;
            positions_offset = 0;
            text_offset = 0;
        }
        
        MemoryPools() {
            // Pre-allocate for 10,000 text elements
            string_pool.reserve(1000000);  // 1MB for text content
            lengths_pool.reserve(100000);  // lengths arrays
            positions_pool.reserve(100000); // positions arrays  
            text_pool.reserve(10000);      // text objects
        }
    };
    
    // Fast string allocation from pool
    struct FastString {
        const char* data;
        size_t length;
        
        FastString(const char* d, size_t len) : data(d), length(len) {}
        
        std::string to_string() const {
            return std::string(data, length);
        }
    };
    
    // Batch text processor - processes multiple texts at once
    struct BatchProcessor {
        std::vector<FastString> pending_texts;
        std::vector<double> pending_positions_x;
        std::vector<double> pending_positions_y;
        std::vector<double> pending_font_sizes;
        
        void reserve(size_t count) {
            pending_texts.reserve(count);
            pending_positions_x.reserve(count);
            pending_positions_y.reserve(count);
            pending_font_sizes.reserve(count);
        }
        
        void clear() {
            pending_texts.clear();
            pending_positions_x.clear();
            pending_positions_y.clear();
            pending_font_sizes.clear();
        }
    };
    
    // Cache for font lookups to avoid repeated map searches
    std::unordered_map<std::string, const PoDoFo::PdfFont*> font_cache;
    
    MemoryPools pools;
    BatchProcessor batch;
    
    // Fast text allocation from pool
    FastString allocate_string(const std::string& str);
    
    // Batch process accumulated texts
    void flush_batch_to_texts(flx_model_list<flx_layout_text>& texts, double page_height);
    
public:
    flx_pdf_text_extractor_fast();
    
    // Fast extraction using memory pools and batching
    bool extract_text_fast(const PoDoFo::PdfPage& page, flx_model_list<flx_layout_text>& texts);
    
    // Reset memory pools for reuse
    void reset_pools();
};