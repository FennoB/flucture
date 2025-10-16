#include "flx_pdf_text_extractor_fast.h"
#include <podofo/podofo.h>
#include <cstring>
#include <algorithm>

using namespace PoDoFo;

flx_pdf_text_extractor_fast::flx_pdf_text_extractor_fast() {
    batch.reserve(1000); // Pre-allocate for typical batch sizes
}

flx_pdf_text_extractor_fast::FastString flx_pdf_text_extractor_fast::allocate_string(const std::string& str) {
    if (pools.string_offset + str.length() > pools.string_pool.size()) {
        pools.string_pool.resize(pools.string_pool.size() + std::max(str.length(), size_t(1000)));
    }
    
    char* dest = pools.string_pool.data() + pools.string_offset;
    std::memcpy(dest, str.data(), str.length());
    
    FastString result(dest, str.length());
    pools.string_offset += str.length();
    
    return result;
}

void flx_pdf_text_extractor_fast::flush_batch_to_texts(flx_model_list<flx_layout_text>& texts, double page_height) {
    // Reserve space to avoid reallocations
    size_t initial_size = texts.size();
    size_t batch_size = batch.pending_texts.size();
    
    // Batch allocate all text objects at once
    for (size_t i = 0; i < batch_size; i++) {
        texts.add_element();
    }
    
    // Fast bulk assignment using pointer arithmetic
    for (size_t i = 0; i < batch_size; i++) {
        auto& text_elem = texts[initial_size + i];
        
        // Direct assignment without string copying where possible
        text_elem.text = flx_string(batch.pending_texts[i].to_string().c_str());
        text_elem.x = batch.pending_positions_x[i];
        text_elem.y = page_height - batch.pending_positions_y[i]; // PDF coordinate conversion
        text_elem.font_size = batch.pending_font_sizes[i];
        text_elem.width = batch.pending_texts[i].length * batch.pending_font_sizes[i] * 0.6; // Estimate
        text_elem.height = batch.pending_font_sizes[i];
        
        // Set default font info - could be cached/optimized further
        text_elem.font_family = flx_string("Arial");
        text_elem.color = flx_string("#000000");
        text_elem.bold = false;
        text_elem.italic = false;
    }
    
    batch.clear();
}

bool flx_pdf_text_extractor_fast::extract_text_fast(const PdfPage& page, flx_model_list<flx_layout_text>& texts) {
    try {
        reset_pools();
        
        // Fast path: Direct PoDoFo content stream processing
        PdfContentReaderArgs args;
        args.Flags = PdfContentReaderFlags::None; // IMPORTANT: Process ALL content including FormXObjects
        PdfContentStreamReader reader(page, args);
        
        PdfContent content;
        std::string decoded;
        std::vector<double> lengths;
        std::vector<unsigned> positions;
        
        // Current text state (minimal tracking)
        double current_font_size = 12.0;
        double current_x = 0.0, current_y = 0.0;
        const PdfFont* current_font = nullptr;
        bool in_text_block = false;
        
        // Pre-allocate vectors to avoid reallocations
        decoded.reserve(1000);
        lengths.reserve(100);
        positions.reserve(100);
        
        while (reader.TryReadNext(content)) {
            if (content.Type != PdfContentType::Operator || 
                content.Warnings != PdfContentWarnings::None) {
                continue;
            }
            
            switch (content.Operator) {
                case PdfOperator::BT:
                    in_text_block = true;
                    break;
                    
                case PdfOperator::ET:
                    in_text_block = false;
                    // Flush any pending batch
                    if (!batch.pending_texts.empty()) {
                        flush_batch_to_texts(texts, page.GetRect().Height);
                    }
                    break;
                    
                case PdfOperator::Tf: {
                    if (content.Stack.size() >= 2) {
                        current_font_size = content.Stack[0].GetReal();
                        const PdfName& fontName = content.Stack[1].GetName();
                        
                        // Fast font lookup with caching
                        auto it = font_cache.find(fontName.GetString());
                        if (it != font_cache.end()) {
                            current_font = it->second;
                        } else {
                            try {
                                auto resources = page.GetResources();
                                current_font = resources.GetFont(fontName);
                                font_cache[fontName.GetString()] = current_font;
                            } catch (...) {
                                current_font = nullptr;
                            }
                        }
                    }
                    break;
                }
                
                case PdfOperator::Tm: {
                    if (content.Stack.size() >= 6) {
                        current_x = content.Stack[4].GetReal();
                        current_y = content.Stack[5].GetReal();
                    }
                    break;
                }
                
                case PdfOperator::Tj:
                case PdfOperator::Quote:
                case PdfOperator::DoubleQuote: {
                    if (!in_text_block || content.Stack.empty()) break;
                    
                    const PdfString& str = content.Stack[content.Stack.size() - 1].GetString();
                    
                    // Fast string decoding - simplified for performance
                    decoded = str.GetString(); // Direct conversion for speed
                    
                    if (!decoded.empty()) {
                        // Add to batch instead of immediate processing
                        FastString fast_str = allocate_string(decoded);
                        batch.pending_texts.push_back(fast_str);
                        batch.pending_positions_x.push_back(current_x);
                        batch.pending_positions_y.push_back(current_y);
                        batch.pending_font_sizes.push_back(current_font_size);
                        
                        // Update position for next text (simplified)
                        current_x += decoded.length() * current_font_size * 0.6;
                        
                        // Flush batch when it gets large enough
                        if (batch.pending_texts.size() >= 100) {
                            flush_batch_to_texts(texts, page.GetRect().Height);
                        }
                    }
                    break;
                }
                
                case PdfOperator::TJ: {
                    if (!in_text_block || content.Stack.empty()) break;
                    
                    const PdfArray& arr = content.Stack[0].GetArray();
                    for (unsigned i = 0; i < arr.size(); i++) {
                        const PdfVariant& variant = arr[i];
                        if (variant.IsString()) {
                            const PdfString& str = variant.GetString();
                            decoded = str.GetString();
                            
                            if (!decoded.empty()) {
                                FastString fast_str = allocate_string(decoded);
                                batch.pending_texts.push_back(fast_str);
                                batch.pending_positions_x.push_back(current_x);
                                batch.pending_positions_y.push_back(current_y);
                                batch.pending_font_sizes.push_back(current_font_size);
                                
                                current_x += decoded.length() * current_font_size * 0.6;
                            }
                        } else if (variant.IsNumber()) {
                            // Adjust spacing
                            double value = variant.GetReal();
                            current_x -= value / 1000.0 * current_font_size;
                        }
                    }
                    
                    // Flush large batches
                    if (batch.pending_texts.size() >= 100) {
                        flush_batch_to_texts(texts, page.GetRect().Height);
                    }
                    break;
                }
                
                default:
                    break;
            }
        }
        
        // Final flush
        if (!batch.pending_texts.empty()) {
            flush_batch_to_texts(texts, page.GetRect().Height);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Fast text extraction failed: " << e.what() << std::endl;
        return false;
    }
}

void flx_pdf_text_extractor_fast::reset_pools() {
    pools.reset();
    batch.clear();
    // Don't clear font_cache - keep it for reuse across documents
}