#ifndef FLX_PDF_TEXT_EXTRACTOR_H
#define FLX_PDF_TEXT_EXTRACTOR_H

#include "../layout/flx_layout_text.h"
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <list>
#include <regex>

#include <main/PdfPage.h>
#include <main/PdfTextState.h>
#include <main/PdfFont.h>
#include <main/PdfArray.h>
#include <main/PdfString.h>
#include <main/PdfName.h>
#include <main/PdfVariant.h>
#include <main/PdfContentStreamReader.h>
#include <main/PdfCanvas.h>
#include <main/PdfXObjectForm.h>
#include <auxiliary/StateStack.h>

using namespace PoDoFo;

class flx_pdf_text_extractor {
public:
    flx_pdf_text_extractor();
    ~flx_pdf_text_extractor();

    bool extract_text_with_fonts(const PdfPage& page, 
                                 flx_model_list<flx_layout_text>& texts);
    
    static void clear_font_cache();
                                 
private:
    struct TextState;
    class StatefulString;
    struct EntryOptions;
    struct ExtractionContext;
    struct flx_extraction_context;
    struct XObjectState;
    struct GlyphAddress;
    
    using StringChunk = std::list<StatefulString>;
    using StringChunkPtr = std::unique_ptr<StringChunk>;
    using StringChunkList = std::list<StringChunkPtr>;
    using StringChunkListPtr = std::unique_ptr<StringChunkList>;
    using TextStateStack = StateStack<TextState>;
    
    void extract_text_directly_to(flx_model_list<flx_layout_text>& texts, const PdfPage& page) const;
    void extract_text_directly_to(flx_model_list<flx_layout_text>& texts, const PdfPage& page, 
                                 const std::string_view& pattern) const;
    
    static bool decodeString(const PdfString &str, TextState &state, std::string &decoded,
        std::vector<double> &lengths, std::vector<unsigned>& positions);
    static bool areEqual(double lhs, double rhs);
    static bool isWhiteSpaceChunk(const StringChunk &chunk);
    static void splitChunkBySpaces(std::vector<StringChunkPtr> &splittedChunks, const StringChunk &chunk);
    static void splitStringBySpaces(std::vector<StatefulString> &separatedStrings, const StatefulString &string);
    static void trimSpacesBegin(StringChunk &chunk);
    static void trimSpacesEnd(StringChunk &chunk);
    static void processChunks(const StringChunkList& chunks, std::string& destString,
        std::vector<unsigned>& positions, std::vector<const StatefulString*>& strings,
        std::vector<GlyphAddress>& glyphAddresses);
    static double computeLength(const std::vector<const StatefulString*>& strings, const std::vector<GlyphAddress>& glyphAddresses,
        unsigned lowerIndex, unsigned upperIndex);
    static bool isMatchWholeWordSubstring(const std::string_view& str, const std::string_view& pattern, size_t& matchPos);
    static Rect computeBoundingBox(const TextState& textState, double boxWidth);
    static void read(const PdfVariantStack& stack, double &tx, double &ty);
    static void read(const PdfVariantStack& stack, double &a, double &b, double &c, double &d, double &e, double &f);
    static void getSubstringIndices(const std::vector<unsigned>& positions, unsigned lowerPos, unsigned upperLimitPos,
        unsigned& lowerIndex, unsigned& upperLimitIndex);
    static EntryOptions optionsFromFlags(PdfTextExtractFlags flags);
    
};

#endif // FLX_PDF_TEXT_EXTRACTOR_H
