#include "documents/pdf/flx_pdf_text_extractor.h"
#include <podofo/podofo.h>
#include <iostream>
#include <fstream>

using namespace PoDoFo;

int main() {
    std::cout << "=== Simple XObject Test ===" << std::endl;

    // Load PDF
    std::string pdf_path = "/home/fenno/Projects/IbisSDBExtractor/test_datasets/landhandel_schmidt_2023/019_bts_5070/sdb.pdf";

    std::ifstream file(pdf_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open PDF file: " << pdf_path << std::endl;
        return 1;
    }

    std::string pdf_data((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    std::cout << "PDF loaded: " << pdf_data.size() << " bytes" << std::endl;

    // Parse with PoDoFo
    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf_data);

    std::cout << "Pages: " << doc.GetPages().GetCount() << std::endl;

    // Get page 5
    auto& page = doc.GetPages().GetPageAt(4); // 0-indexed

    std::cout << "\n=== Extracting text from Page 5 ===" << std::endl;

    // Extract text with our extractor
    flx_pdf_text_extractor extractor;
    flx_model_list<flx_layout_text> texts;

    extractor.extract_text_with_fonts(page, texts);

    std::cout << "Total texts extracted: " << texts.size() << std::endl;

    // Look for pH values
    bool found_15 = false;
    bool found_120 = false;

    for (size_t i = 0; i < texts.size(); i++) {
        auto& text = texts[i];
        std::string content(text.text->c_str());

        if (content.find("1,5") != std::string::npos) {
            found_15 = true;
            std::cout << "\n✓ FOUND pH 1,5: '" << content << "'" << std::endl;
        }

        if (content.find("12,0") != std::string::npos) {
            found_120 = true;
            std::cout << "\n✗ FOUND pH 12,0: '" << content << "'" << std::endl;
        }
    }

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "pH 1,5 found:  " << (found_15 ? "YES ✓" : "NO ✗") << std::endl;
    std::cout << "pH 12,0 found: " << (found_120 ? "YES" : "NO") << std::endl;

    if (!found_15) {
        std::cout << "\n=== First 30 texts ===" << std::endl;
        for (size_t i = 0; i < std::min(texts.size(), size_t(30)); i++) {
            std::string txt(texts[i].text->c_str());
            if (!txt.empty()) {
                std::cout << "[" << i << "] " << txt << std::endl;
            }
        }
    }

    return found_15 ? 0 : 1;
}
