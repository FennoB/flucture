#ifndef FLX_PDF_TEXT_EXTRACTOR_H
#define FLX_PDF_TEXT_EXTRACTOR_H

#include "../layout/flx_layout_text.h"
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <list>
#include <regex>

// Include PoDoFo headers instead of forward declarations
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

// Complete PDF text extraction class with full PoDoFo compatibility
class flx_pdf_text_extractor {
public:
    flx_pdf_text_extractor();
    ~flx_pdf_text_extractor();
    
    // Public flx-compatible functions
    bool extract_text_with_fonts(const PdfPage& page, 
                                 std::vector<flx_layout_text>& texts);
                                 
private:
    // Direct copy of PoDoFo internal structures and functions
    
    // 5.2 Text State Parameters and Operators
    // 5.3 Text Objects
    struct TextState;
    class StatefulString;
    struct EntryOptions;
    struct ExtractionContext;
    struct XObjectState;
    struct GlyphAddress;
    
    // Type aliases from PoDoFo
    using StringChunk = std::list<StatefulString>;
    using StringChunkPtr = std::unique_ptr<StringChunk>;
    using StringChunkList = std::list<StringChunkPtr>;
    using StringChunkListPtr = std::unique_ptr<StringChunkList>;
    using TextStateStack = StateStack<TextState>;
    
    // Core extraction function (adapted from PdfPage::ExtractTextTo)
    void ExtractTextTo(std::vector<PdfTextEntry>& entries, const PdfPage& page) const;
    void ExtractTextTo(std::vector<PdfTextEntry>& entries, const PdfPage& page, 
                      const std::string_view& pattern) const;
    
    // String processing functions
    static bool decodeString(const PdfString &str, TextState &state, std::string &decoded,
        std::vector<double> &lengths, std::vector<unsigned>& positions);
    static bool areEqual(double lhs, double rhs);
    static bool isWhiteSpaceChunk(const StringChunk &chunk);
    static void splitChunkBySpaces(std::vector<StringChunkPtr> &splittedChunks, const StringChunk &chunk);
    static void splitStringBySpaces(std::vector<StatefulString> &separatedStrings, const StatefulString &string);
    static void trimSpacesBegin(StringChunk &chunk);
    static void trimSpacesEnd(StringChunk &chunk);
    static void addEntry(std::vector<PdfTextEntry> &textEntries, StringChunkList &strings,
        const std::string_view &pattern, const EntryOptions &options, const nullable<Rect> &clipRect,
        int pageIndex, const Matrix* rotation);
    static void addEntryChunk(std::vector<PdfTextEntry> &textEntries, StringChunkList &strings,
        const std::string_view &pattern, const EntryOptions& options, const nullable<Rect> &clipRect,
        int pageIndex, const Matrix* rotation);
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
    
    // Conversion functions from PoDoFo types to flx types
    void convertPdfEntriesToFlxTexts(const std::vector<PdfTextEntry>& entries, 
                                     std::vector<flx_layout_text>& texts);
};

#endif // FLX_PDF_TEXT_EXTRACTOR_H