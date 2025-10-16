#include <iostream>
#include <fstream>
#include <sstream>
#include "documents/pdf/flx_pdf_text_extractor.h"
#include "documents/pdf/flx_pdf_sio.h"
#include <main/PdfMemDocument.h>

using namespace PoDoFo;

void test_direct_extraction(const std::string& pdf_path) {
    std::cout << "\n=== TEST 1: Direct Extraction (wie test_xobject_simple) ===" << std::endl;

    // Load PDF
    PdfMemDocument doc;
    doc.Load(pdf_path);

    // Extract from page 5 (index 4)
    auto& page = doc.GetPages().GetPageAt(4);

    flx_pdf_text_extractor extractor;
    flx_model_list<flx_layout_text> texts;
    extractor.extract_text_with_fonts(page, texts);

    std::cout << "✓ Direct extraction: " << texts.size() << " texts" << std::endl;
}

void test_via_flx_pdf_sio(const std::string& pdf_path) {
    std::cout << "\n=== TEST 2: Via flx_pdf_sio::parse() ===" << std::endl;

    // Read PDF file
    std::ifstream file(pdf_path, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string pdf_data = buffer.str();
    flx_string flx_data(pdf_data.c_str());

    // Parse via flx_pdf_sio
    flx_pdf_sio parser;
    if (!parser.parse(flx_data)) {
        std::cout << "✗ Parse failed!" << std::endl;
        return;
    }

    // Access pages via parent class
    if (parser.pages.size() <= 4) {
        std::cout << "✗ Not enough pages!" << std::endl;
        return;
    }

    std::cout << "✓ Via flx_pdf_sio: " << parser.pages[4].texts.size() << " texts" << std::endl;
}

void test_manual_replication(const std::string& pdf_path) {
    std::cout << "\n=== TEST 3: Manual Replication (schrittweise wie flx_pdf_sio) ===" << std::endl;

    // Step 1: Load PDF like flx_pdf_sio does
    std::ifstream file(pdf_path, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string pdf_data = buffer.str();

    PdfMemDocument doc;
    doc.LoadFromBuffer({pdf_data.data(), pdf_data.size()});
    std::cout << "  Step 1: PDF loaded" << std::endl;

    // Step 2: Get page 5 via GetPageAt like flx_pdf_sio does
    auto& page = doc.GetPages().GetPageAt(4);
    std::cout << "  Step 2: Got page 5" << std::endl;

    // Step 3: Extract text
    flx_pdf_text_extractor extractor;
    flx_model_list<flx_layout_text> texts;
    extractor.extract_text_with_fonts(page, texts);

    std::cout << "✓ Manual replication: " << texts.size() << " texts" << std::endl;
}

void test_with_multiple_extractors(const std::string& pdf_path) {
    std::cout << "\n=== TEST 4: Multiple Extractor Instances (wie in flx_pdf_sio loop) ===" << std::endl;

    PdfMemDocument doc;
    doc.Load(pdf_path);

    int page_count = doc.GetPages().GetCount();
    std::cout << "  Total pages: " << page_count << std::endl;

    // Loop through ALL pages like flx_pdf_sio does
    for (int page_idx = 0; page_idx < page_count; page_idx++) {
        auto& page = doc.GetPages().GetPageAt(page_idx);

        // Create NEW extractor for each page (like flx_pdf_sio)
        flx_pdf_text_extractor extractor;
        flx_model_list<flx_layout_text> texts;
        extractor.extract_text_with_fonts(page, texts);

        if (page_idx == 4) {
            std::cout << "✓ Page 5 in loop: " << texts.size() << " texts" << std::endl;
        }
    }
}

void test_with_font_cache_clear(const std::string& pdf_path) {
    std::cout << "\n=== TEST 5: With Font Cache Clear (wie flx_pdf_sio Zeile 174) ===" << std::endl;

    PdfMemDocument doc;
    doc.Load(pdf_path);

    // Clear font cache like flx_pdf_sio does
    flx_pdf_text_extractor::clear_font_cache();
    std::cout << "  Font cache cleared" << std::endl;

    int page_count = doc.GetPages().GetCount();

    for (int page_idx = 0; page_idx < page_count; page_idx++) {
        auto& page = doc.GetPages().GetPageAt(page_idx);

        flx_pdf_text_extractor extractor;
        flx_model_list<flx_layout_text> texts;
        extractor.extract_text_with_fonts(page, texts);

        if (page_idx == 4) {
            std::cout << "✓ Page 5 after cache clear: " << texts.size() << " texts" << std::endl;
        }
    }
}

int main() {
    std::string pdf_path = "/home/fenno/Projects/IbisSDBExtractor/test_datasets/landhandel_schmidt_2023/019_bts_5070/sdb.pdf";

    std::cout << "=== XObject Bisection Test ===" << std::endl;
    std::cout << "Systematische Analyse der Unterschiede zwischen test_xobject_simple und pdf_to_layout" << std::endl;
    std::cout << "PDF: " << pdf_path << std::endl;

    try {
        test_direct_extraction(pdf_path);
        test_via_flx_pdf_sio(pdf_path);
        test_manual_replication(pdf_path);
        test_with_multiple_extractors(pdf_path);
        test_with_font_cache_clear(pdf_path);

        std::cout << "\n=== Erwartung ===" << std::endl;
        std::cout << "Alle Tests sollten 75 Texte zeigen." << std::endl;
        std::cout << "Wenn ein Test 69 zeigt, liegt das Problem in diesem Schritt!" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
