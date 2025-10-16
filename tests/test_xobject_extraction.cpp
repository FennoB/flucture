#include <catch2/catch_all.hpp>
#include "../documents/pdf/flx_pdf_sio.h"
#include <fstream>

SCENARIO("XObject text extraction from real PDF", "[pdf][xobject]")
{
  GIVEN("BTS 5070 Safety Data Sheet with XObject text")
  {
    // Test PDF location
    std::string pdf_path = "/home/fenno/SB/Fenno/Werk/Business/Aidoo/test_datasets/landhandel_schmidt_2023/015_bts_5070/sdb.pdf";

    // Read PDF file
    std::ifstream file(pdf_path, std::ios::binary);
    REQUIRE(file.is_open());

    std::string pdf_data((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    REQUIRE(pdf_data.size() > 0);

    WHEN("Extracting text with XObject support")
    {
      flx_pdf_sio pdf;
      flx_string flx_data(pdf_data.c_str());

      bool success = pdf.parse(flx_data);
      REQUIRE(success);

      // Get page 5 (where the pH value should be)
      auto& pages = pdf.pages;
      REQUIRE(pages.size() >= 5);

      auto& page5 = pages[4]; // 0-indexed
      auto& texts = page5.texts;

      THEN("pH value '1,5' should be found in extracted text")
      {
        bool found_ph_15 = false;
        bool found_ph_120 = false;

        std::cout << "\n=== Page 5 Text Extraction ===" << std::endl;
        std::cout << "Total texts on page 5: " << texts.size() << std::endl;

        for (size_t i = 0; i < texts.size(); i++)
        {
          auto& text = texts[i];
          std::string content(text.text->c_str());

          // Look for the correct pH value
          if (content.find("1,5") != std::string::npos)
          {
            found_ph_15 = true;
            std::cout << "✓ Found pH 1,5 in text: '" << content << "'" << std::endl;
          }

          // Also check if wrong value exists
          if (content.find("12,0") != std::string::npos)
          {
            found_ph_120 = true;
            std::cout << "✗ Found pH 12,0 in text: '" << content << "'" << std::endl;
          }
        }

        std::cout << "Result - pH 1,5: " << (found_ph_15 ? "FOUND ✓" : "NOT FOUND ✗") << std::endl;
        std::cout << "Result - pH 12,0: " << (found_ph_120 ? "FOUND" : "NOT FOUND") << std::endl;

        if (!found_ph_15)
        {
          std::cout << "\nShowing first 20 texts:" << std::endl;
          for (size_t i = 0; i < std::min(texts.size(), size_t(20)); i++)
          {
            std::string txt(texts[i].text->c_str());
            std::cout << "  [" << i << "] '" << txt << "'" << std::endl;
          }
        }

        REQUIRE(found_ph_15);
      }
    }
  }
}
