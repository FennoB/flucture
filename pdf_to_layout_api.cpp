#include "documents/pdf/flx_pdf_sio.h"
#include "documents/layout/flx_layout_geometry.h"
#include "api/json/flx_json.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>

// Simple API function to convert PDF file to Layout structure
bool pdf_file_to_layout(const std::string& pdf_path, flx_model_list<flx_layout_geometry>& pages) {
    try {
        // Read PDF file
        std::ifstream pdf_file(pdf_path, std::ios::binary);
        if (!pdf_file) {
            std::cerr << "Error: Cannot open PDF file: " << pdf_path << std::endl;
            return false;
        }
        
        // Read file content
        pdf_file.seekg(0, std::ios::end);
        size_t file_size = pdf_file.tellg();
        pdf_file.seekg(0, std::ios::beg);
        
        std::string pdf_data_str;
        pdf_data_str.resize(file_size);
        pdf_file.read(&pdf_data_str[0], file_size);
        
        // IMPORTANT: Use the binary constructor, not c_str() which stops at NULL bytes!
        flx_string pdf_data(pdf_data_str.data(), pdf_data_str.size());
        pdf_file.close();
        
        std::cout << "📄 Loaded PDF: " << pdf_path << " (" << file_size << " bytes)" << std::endl;
        std::cout << "🔍 PDF string size: " << pdf_data.size() << " bytes" << std::endl;
        std::cout << "🔍 First 20 bytes: ";
        for (int i = 0; i < std::min(20, (int)pdf_data.size()); i++) {
            printf("%02X ", (unsigned char)pdf_data.c_str()[i]);
        }
        std::cout << std::endl;
        std::cout << "🔍 Last 20 bytes: ";
        size_t start = std::max(0, (int)pdf_data.size() - 20);
        for (size_t i = start; i < pdf_data.size(); i++) {
            printf("%02X ", (unsigned char)pdf_data.c_str()[i]);
        }
        std::cout << std::endl;
        
        // Parse PDF to Layout
        flx_pdf_sio parser;
        bool success = parser.parse(pdf_data);
        
        if (!success) {
            std::cerr << "❌ Failed to parse PDF with our current implementation" << std::endl;
            return false;
        }
        
        // Copy parsed pages to output
        pages = parser.pages;
        
        std::cout << "✅ SUCCESS: Converted PDF to " << pages.size() << " page(s)" << std::endl;
        
        // Print structure summary
        int total_geometries = 0;
        int total_texts = 0;
        
        for (size_t page_num = 0; page_num < pages.size(); page_num++) {
            auto& page = pages[page_num];
            std::cout << "📖 Page " << (page_num + 1) << ": " << page.sub_geometries.size() << " geometries, " 
                     << page.texts.size() << " texts" << std::endl;
            
            total_geometries += page.sub_geometries.size();
            total_texts += page.texts.size();
        }
        
        std::cout << "📊 TOTAL: " << total_geometries << " geometries, " << total_texts << " texts across " 
                  << pages.size() << " pages" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error converting PDF: " << e.what() << std::endl;
        return false;
    }
}

// Convert PDF SIO object to JSON
std::string pdf_sio_to_json(flx_pdf_sio& parser) {
    try {
        flx_json json_converter(&*parser);
        // Convert to JSON string
        flx_string json_output = json_converter.create();        
        return std::string(json_output.c_str());
    } catch (const std::exception& e) {
        std::cerr << "Error converting to JSON: " << e.what() << std::endl;
        return "{}"; // Return empty JSON object on error
    }
}

// Main function for command-line usage
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <pdf_file> [output_context.txt]" << std::endl;
        std::cout << "Converts PDF to layout structure for AI context." << std::endl;
        return 1;
    }
    
    std::string pdf_path = argv[1];
    std::string output_path = (argc >= 3) ? argv[2] : "";
    
    std::cout << "🚀 Flucture PDF → Layout Converter" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Parse PDF directly with our parser (no intermediate step needed)
    flx_pdf_sio parser;
    
    // Read PDF file
    std::ifstream pdf_file(pdf_path, std::ios::binary);
    if (!pdf_file) {
        std::cerr << "Error: Cannot open PDF file: " << pdf_path << std::endl;
        return 1;
    }
    
    // Read file content
    pdf_file.seekg(0, std::ios::end);
    size_t file_size = pdf_file.tellg();
    pdf_file.seekg(0, std::ios::beg);
    
    std::string pdf_data_str;
    pdf_data_str.resize(file_size);
    pdf_file.read(&pdf_data_str[0], file_size);
    
    flx_string pdf_data(pdf_data_str.data(), pdf_data_str.size());
    pdf_file.close();
    
    std::cout << "📄 Loaded PDF: " << pdf_path << " (" << file_size << " bytes)" << std::endl;
    
    // Parse PDF to Layout
    bool success = parser.parse(pdf_data);
    if (!success) {
        std::cerr << "❌ Failed to parse PDF" << std::endl;
        return 1;
    }
    
    std::cout << "✅ SUCCESS: Parsed PDF with " << parser.pages.size() << " page(s)" << std::endl;
    
    // Convert parser (entire pdf_sio object) to JSON
    std::string json_output = pdf_sio_to_json(parser);
    
    // Output to file or stdout
    if (!output_path.empty()) {
        std::ofstream output_file(output_path);
        output_file << json_output;
        output_file.close();
        std::cout << "📝 JSON written to: " << output_path << std::endl;
    } else {
        std::cout << json_output << std::endl;
    }
    
    std::cout << "🎉 PDF → JSON conversion complete!" << std::endl;
    return 0;
}
