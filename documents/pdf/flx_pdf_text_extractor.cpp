/**
 * Complete PDF text extraction implementation based on PoDoFo PdfPage_TextExtraction.cpp
 * Adapted for flx_layout_text output format
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "flx_pdf_text_extractor.h"

#include <regex>
#include <list>
#include <iostream>
#include <limits>
#include <cmath>
#include <unordered_map>

#include <utf8cpp/utf8.h>

#include <podofo/private/PdfDeclarationsPrivate.h>
#include <main/PdfDocument.h>
#include <main/PdfPage.h>
#include <main/PdfTextState.h>
#include <main/PdfMath.h>
#include <main/PdfXObjectForm.h>
#include <main/PdfContentStreamReader.h>
#include <main/PdfFont.h>
#include <main/PdfVariant.h>
#include <main/PdfArray.h>
#include <main/PdfString.h>
#include <main/PdfName.h>
#include <main/PdfCanvas.h>

#include <podofo/private/outstringstream.h>
#include <podofo/auxiliary/StateStack.h>

using namespace std;
using namespace PoDoFo;

constexpr double SAME_LINE_THRESHOLD = 0.01;
constexpr double SEPARATION_EPSILON = 0.0000001;
// Inferred empirically on Adobe Acrobat Pro
constexpr unsigned HARD_SEPARATION_SPACING_MULTIPLIER = 6;
#define ASSERT(condition, message, ...) if (!condition)\
PoDoFo::LogMessage(PdfLogSeverity::Warning, message, ##__VA_ARGS__);

static constexpr float NaN = numeric_limits<float>::quiet_NaN();

// 5.2 Text State Parameters and Operators
// 5.3 Text Objects
struct flx_pdf_text_extractor::TextState
{
  TextState()
  {
    // Initialize all matrices to Identity (PDF spec default)
    T_rm = Matrix();   // Identity matrix
    CTM = Matrix();    // Identity matrix
    T_m = Matrix();    // Identity matrix
    T_lm = Matrix();   // Identity matrix
    // Reset font size
    PdfState.FontSize = -1;
  }

  Matrix T_rm;  // Current T_rm
  Matrix CTM;   // Current CTM
  Matrix T_m;   // Current T_m
  Matrix T_lm;  // Current T_lm
  double T_l = 0;             // Leading text Tl
  PdfTextState PdfState;
  Vector2 WordSpacingVectorRaw;
  Vector2 SpaceCharVectorRaw;
  double WordSpacingLength = 0;
  double CharSpaceLength = 0;
  void ComputeDependentState();
  void ComputeSpaceDescriptors();
  void ComputeT_rm();
  double GetWordSpacingLength() const;
  double GetSpaceCharLength() const;
  void ScanString(const PdfString& encodedStr, string& decoded, vector<double>& lengths, vector<unsigned>& positions);
};

class flx_pdf_text_extractor::StatefulString
{
public:
  StatefulString(string&& str, const TextState& state, vector<double>&& rawLengths, vector<unsigned>&& positions);
public:
  bool BeginsWithWhiteSpace() const;
  bool EndsWithWhiteSpace() const;
  StatefulString GetTrimmedBegin() const;
  StatefulString GetTrimmedEnd() const;
  double GetLengthRaw() const;
  double GetLength() const;
private:
  vector<double> computeLengths(const vector<double>& rawLengths);
public:
  const string String;
  const TextState State;
  const vector<double> RawLengths;
  const vector<double> Lengths;
  // Glyph position in the string
  const vector<unsigned> StringPositions;
  const Vector2 Position;
  const bool IsWhiteSpace;
};

struct flx_pdf_text_extractor::EntryOptions
{
  bool IgnoreCase;
  bool TrimSpaces;
  bool TokenizeWords;
  bool MatchWholeWord;
  bool RegexPattern;
  bool ComputeBoundingBox;
  bool RawCoordinates;
  bool ExtractSubstring;
};

struct flx_pdf_text_extractor::XObjectState
{
  const PdfXObjectForm* Form;
  unsigned TextStateIndex;
};

struct flx_pdf_text_extractor::ExtractionContext
{
public:
  ExtractionContext(vector<PdfTextEntry> &entries, const PdfPage &page, const string_view &pattern,
    PdfTextExtractFlags flags, const nullable<Rect> &clipRect);
public:
  void BeginText();
  void EndText();
  void Tf_Operator(const PdfName &fontname, double fontsize);
  void cm_Operator(double a, double b, double c, double d, double e, double f);
  void Tm_Operator(double a, double b, double c, double d, double e, double f);
  void TdTD_Operator(double tx, double ty);
  void AdvanceSpace(double ty);
  void TStar_Operator();
public:
  void PushString(const StatefulString &str, bool pushchunk = false);
  void TryPushChunk();
  void TryAddLastEntry();
private:
  bool areChunksSpaced(double& distance);
  void pushChunk();
  void addEntry();
  void tryAddEntry(const StatefulString& currStr);
  const PdfCanvas& getActualCanvas();
  const StatefulString& getPreviouString() const;
private:
  const PdfPage& m_page;
public:
  const int PageIndex;
  const string Pattern;
  const EntryOptions Options;
  const nullable<Rect> ClipRect;
  unique_ptr<Matrix> Rotation;
  vector<PdfTextEntry> &Entries;
  StringChunkPtr Chunk = std::make_unique<StringChunk>();
  StringChunkList Chunks;
  TextStateStack States;
  vector<XObjectState> XObjectStateIndices;
  double CurrentEntryT_rm_y = NaN;    // Tracks line changing
  Vector2 PrevChunkT_rm_Pos;          // Tracks space separation
  bool BlockOpen = false;
};

struct flx_pdf_text_extractor::flx_extraction_context
{
public:
  flx_extraction_context(flx_model_list<flx_layout_text> &texts, const PdfPage &page, const string_view &pattern,
    PdfTextExtractFlags flags, const nullable<Rect> &clipRect);
public:
  void BT_Operator();
  void ET_Operator();
  void Tf_Operator(const PdfName &fontname, double fontsize);
  void cm_Operator(double a, double b, double c, double d, double e, double f);
  void Tm_Operator(double a, double b, double c, double d, double e, double f);
  void TdTD_Operator(double tx, double ty);
  void TStar_Operator();
  void Tj_Operator(double tx, double ty);
  void q_Operator();
  void Q_Operator();
public:
  void add_stateful_string(const StatefulString &str);
  void finalize_text_extraction();
  void add_text_entry_from_chunk(const StringChunk& chunk);
  void add_text_entry_from_chunk_at_index(const StringChunk& chunk, size_t index);
private:
  const PdfPage& m_page;
  flx_model_list<flx_layout_text>& m_texts;
  double m_pageHeight;
public:
  const int PageIndex;
  const string Pattern;
  const nullable<Rect> ClipRect;
  TextStateStack States;
  vector<XObjectState> XObjectStateIndices;
  bool BlockOpen = false;
  StringChunkList Chunks;
  StringChunkPtr Chunk = std::make_unique<StringChunk>();
};

struct flx_pdf_text_extractor::GlyphAddress
{
  unsigned StringIndex;
  unsigned GlyphIndex;
};

flx_pdf_text_extractor::flx_pdf_text_extractor() {
}

flx_pdf_text_extractor::~flx_pdf_text_extractor() {
}

bool flx_pdf_text_extractor::extract_text_with_fonts(const PdfPage& page, 
  flx_model_list<flx_layout_text>& texts) {
  try {
    std::cout << "Starting complete PDF text extraction with fonts..." << std::endl;
    extract_text_directly_to(texts, page);

    std::cout << "Successfully extracted " << texts.size() << " text entries with font information" << std::endl;
    return true;

  } catch (const std::exception& e) {
    std::cout << "Complete text extraction failed: " << e.what() << std::endl;
    return false;
  }
}

void flx_pdf_text_extractor::extract_text_directly_to(flx_model_list<flx_layout_text>& texts, const PdfPage& page) const
{
  extract_text_directly_to(texts, page, { });
}

void flx_pdf_text_extractor::extract_text_directly_to(flx_model_list<flx_layout_text>& texts, const PdfPage& page, 
  const std::string_view& pattern) const
{
  flx_extraction_context context(texts, page, pattern, PdfTextExtractFlags::None, { });

  PdfContentReaderArgs args;
  args.Flags = PdfContentReaderFlags::None;

  // DEBUG: Log that XObject processing is enabled
  std::cout << "[TEXT EXTRACTION] Using PdfContentReaderFlags::None (XObjects ENABLED)" << std::endl;

  PdfContentStreamReader reader(page, args);
  PdfContent content;
  vector<double> lengths;
  vector<unsigned> positions;
  string decoded;
  
  lengths.reserve(1000);
  positions.reserve(1000);
  decoded.reserve(1000);
  
  while (reader.TryReadNext(content))
  {
    switch (content.Type)
    {
      case PdfContentType::Operator:
      {
        if (content.Warnings != PdfContentWarnings::None)
        {
          // Ignore invalid operators
          continue;
        }

        switch (content.Operator)
        {
          case PdfOperator::TL:
          {
            context.States.Current->T_l = content.Stack[0].GetReal();
            break;
          }
          case PdfOperator::cm:
          {
            double a, b, c, d, e, f;
            read(content.Stack, a, b, c, d, e, f);
            context.cm_Operator(a, b, c, d, e, f);
            break;
          }
          case PdfOperator::Td:
          case PdfOperator::TD:
          case PdfOperator::Tm:
          {
            if (content.Operator == PdfOperator::Td || content.Operator == PdfOperator::TD)
            {
              double tx, ty;
              read(content.Stack, tx, ty);
              context.TdTD_Operator(tx, ty);

              if (content.Operator == PdfOperator::TD)
                context.States.Current->T_l = -ty;
            }
            else if (content.Operator == PdfOperator::Tm)
            {
              double a, b, c, d, e, f;
              read(content.Stack, a, b, c, d, e, f);
              context.Tm_Operator(a, b, c, d, e, f);
            }

            break;
          }
          case PdfOperator::T_Star:
          {
            context.TStar_Operator();
            break;
          }
          case PdfOperator::Tf:
          {
            double fontSize = content.Stack[0].GetReal();
            const PdfName& fontName = content.Stack[1].GetName();
            context.Tf_Operator(fontName, fontSize);
            break;
          }
          case PdfOperator::Tj:
          case PdfOperator::Quote:
          case PdfOperator::DoubleQuote:
          {
            if (content.Operator == PdfOperator::Quote)
            {
              context.TStar_Operator();
            }
            else if (content.Operator == PdfOperator::DoubleQuote)
            {
              context.States.Current->PdfState.CharSpacing = content.Stack[0].GetReal();
              context.States.Current->PdfState.WordSpacing = content.Stack[1].GetReal();
              context.States.Current->ComputeSpaceDescriptors();
              context.TStar_Operator();
            }

            context.States.Current->ComputeDependentState();

            const PdfString& str = content.Stack[content.Stack.size() - 1].GetString();
            context.States.Current->ScanString(str, decoded, lengths, positions);
            if (!decoded.empty())
            {
              context.add_stateful_string(StatefulString(std::move(decoded), *context.States.Current,
                std::move(lengths), std::move(positions)));
            }
            break;
          }
          case PdfOperator::TJ:
          {
            const PdfArray& arr = content.Stack[0].GetArray();
            for (unsigned i = 0; i < arr.size(); i++)
            {
              const PdfVariant& variant = arr[i];
              if (variant.IsString())
              {
                context.States.Current->ComputeDependentState();

                const PdfString& str = variant.GetString();
                context.States.Current->ScanString(str, decoded, lengths, positions);
                if (!decoded.empty())
                {
                  context.add_stateful_string(StatefulString(std::move(decoded), *context.States.Current,
                    std::move(lengths), std::move(positions)));
                }
              }
              else if (variant.IsNumber())
              {
                double value = variant.GetReal();
                double t_j = -value / 1000.0 * context.States.Current->PdfState.FontSize * context.States.Current->PdfState.FontScale;
                context.Tj_Operator(t_j, 0);
              }
            }
            break;
          }
          case PdfOperator::g:
          case PdfOperator::rg:
          {
            break;
          }
          case PdfOperator::q:
          {
            context.q_Operator();
            break;
          }
          case PdfOperator::Q:
          {
            context.Q_Operator();
            break;
          }
          case PdfOperator::BT:
          {
            context.BT_Operator();
            break;
          }
          case PdfOperator::ET:
          {
            context.ET_Operator();
            break;
          }
          default:
            break;
        }
        break;
      }
      case PdfContentType::ImageData:
      case PdfContentType::ImageDictionary:
      default:
        break;
    }
  }

  context.finalize_text_extraction();
}

void flx_pdf_text_extractor::ExtractionContext::BeginText()
{
  ASSERT(!BlockOpen, "Text block already open");

  BlockOpen = true;
}

void flx_pdf_text_extractor::ExtractionContext::EndText()
{
  ASSERT(BlockOpen, "No text block open");
  States.Current->T_m = Matrix();
  States.Current->T_lm = Matrix();
  States.Current->ComputeDependentState();
  BlockOpen = false;
}

double flx_pdf_text_extractor::TextState::GetWordSpacingLength() const
{
  if (PdfState.Font == nullptr) {
    return 0.0;
  }
  return PdfState.Font->GetWordSpacingLength(PdfState);
}

double flx_pdf_text_extractor::TextState::GetSpaceCharLength() const
{
  if (PdfState.Font == nullptr) {
    return 0.0;
  }
  return PdfState.Font->GetSpaceCharLength(PdfState);
}

void flx_pdf_text_extractor::TextState::ScanString(const PdfString& encodedStr, string& decoded, vector<double>& lengths, vector<unsigned>& positions)
{
  if (PdfState.Font == nullptr) {
    decoded = encodedStr.GetString();
    lengths.clear();
    positions.clear();
    return;
  }

  PdfState.Font->TryScanEncodedString(encodedStr, PdfState, decoded, lengths, positions);
}

void flx_pdf_text_extractor::TextState::ComputeDependentState()
{
  ComputeSpaceDescriptors();
  ComputeT_rm();
}

void flx_pdf_text_extractor::TextState::ComputeSpaceDescriptors()
{
  WordSpacingLength = 0.0;
  CharSpaceLength = 0.0;
}

void flx_pdf_text_extractor::TextState::ComputeT_rm()
{
  T_rm = T_m * CTM;
}

void flx_pdf_text_extractor::read(const PdfVariantStack& stack, double &tx, double &ty)
{
  tx = stack[1].GetReal();
  ty = stack[0].GetReal();
}

void flx_pdf_text_extractor::read(const PdfVariantStack& stack, double &a, double &b, double &c, double &d, double &e, double &f)
{
  a = stack[5].GetReal();
  b = stack[4].GetReal();
  c = stack[3].GetReal();
  d = stack[2].GetReal();
  e = stack[1].GetReal();
  f = stack[0].GetReal();
}

flx_pdf_text_extractor::StatefulString::StatefulString(string&& str, const TextState& state, vector<double>&& rawLengths, vector<unsigned>&& positions)
    : String(std::move(str))
      , State(state)
      , RawLengths(std::move(rawLengths))
      , Lengths(computeLengths(RawLengths))
      , StringPositions(std::move(positions))
      , Position(state.T_rm.GetTranslationVector())
      , IsWhiteSpace(String.find_first_not_of(" \t\n\r") == string::npos)
{
}

vector<double> flx_pdf_text_extractor::StatefulString::computeLengths(const vector<double>& rawLengths)
{
  return rawLengths;
}

double flx_pdf_text_extractor::StatefulString::GetLengthRaw() const
{
  double total = 0.0;
  for (double len : RawLengths)
    total += len;
  return total;
}

flx_pdf_text_extractor::flx_extraction_context::flx_extraction_context(
  flx_model_list<flx_layout_text>& texts, const PdfPage& page, const string_view& pattern,
  PdfTextExtractFlags flags, const nullable<Rect>& clipRect) :
                                                               m_page(page),
                                                               m_texts(texts),
                                                               m_pageHeight(page.GetRect().Height),
                                                               PageIndex(page.GetPageNumber() - 1),
                                                               Pattern(pattern),
                                                               ClipRect(clipRect)
{
  States.Push();
}

void flx_pdf_text_extractor::flx_extraction_context::BT_Operator()
{
  BlockOpen = true;

  States.Current->T_m = Matrix();
  States.Current->T_lm = Matrix();
  States.Current->ComputeDependentState();
}

void flx_pdf_text_extractor::flx_extraction_context::ET_Operator()
{
  BlockOpen = false;
  if (!Chunk->empty())
  {
    Chunks.push_back(std::move(Chunk));
    Chunk.reset(new StringChunk());
  }
}

static std::unordered_map<std::string, const PdfFont*> g_font_cache;

void flx_pdf_text_extractor::clear_font_cache() {
  g_font_cache.clear();
  std::cout << "Static font cache cleared" << std::endl;
}

void flx_pdf_text_extractor::flx_extraction_context::Tf_Operator(const PdfName& fontname, double fontsize)
{
  States.Current->PdfState.FontSize = fontsize;
  
  const std::string font_key(fontname.GetString());
  
  auto cached_font = g_font_cache.find(font_key);
  if (cached_font != g_font_cache.end()) {
    States.Current->PdfState.Font = cached_font->second;
    return;
  }
  
  try {
    auto resources = m_page.GetResources();
    States.Current->PdfState.Font = resources.GetFont(fontname);
    g_font_cache[font_key] = States.Current->PdfState.Font;
  } catch (...) {
    States.Current->PdfState.Font = nullptr;
    g_font_cache[font_key] = nullptr;
  }
}

void flx_pdf_text_extractor::flx_extraction_context::cm_Operator(double a, double b, double c, double d, double e, double f)
{
  Matrix transform(a, b, c, d, e, f);
  States.Current->CTM = States.Current->CTM * transform;
  States.Current->ComputeDependentState();
}

void flx_pdf_text_extractor::flx_extraction_context::Tm_Operator(double a, double b, double c, double d, double e, double f)
{
  States.Current->T_m = Matrix(a, b, c, d, e, f);
  States.Current->T_lm = States.Current->T_m;
  States.Current->ComputeDependentState();
}

void flx_pdf_text_extractor::flx_extraction_context::TdTD_Operator(double tx, double ty)
{
  Matrix transform = Matrix::CreateTranslation(Vector2(tx, ty));
  States.Current->T_lm = States.Current->T_lm * transform;
  States.Current->T_m = States.Current->T_lm;
  States.Current->ComputeDependentState();
}

void flx_pdf_text_extractor::flx_extraction_context::TStar_Operator()
{
  TdTD_Operator(0, -States.Current->T_l);
}

void flx_pdf_text_extractor::flx_extraction_context::Tj_Operator(double tx, double ty)
{
  Matrix transform = Matrix::CreateTranslation(Vector2(tx, ty));
  States.Current->T_m = States.Current->T_m * transform;
  States.Current->ComputeDependentState();
}

void flx_pdf_text_extractor::flx_extraction_context::q_Operator()
{
  States.Push();
}

void flx_pdf_text_extractor::flx_extraction_context::Q_Operator()
{
  States.Pop();
}

void flx_pdf_text_extractor::flx_extraction_context::add_stateful_string(const StatefulString& str)
{
  Chunk->push_back(str);
}

void flx_pdf_text_extractor::flx_extraction_context::finalize_text_extraction()
{
  if (!Chunk->empty())
  {
    Chunks.push_back(std::move(Chunk));
  }

  size_t total_texts = 0;
  for (const auto& chunk_ptr : Chunks) {
    if (!chunk_ptr->empty()) {
      total_texts++;
    }
  }
  
  size_t start_index = m_texts.size();
  for (size_t i = 0; i < total_texts; i++) {
    m_texts.add_element();
  }

  size_t text_index = start_index;
  for (const auto& chunk_ptr : Chunks)
  {
    if (!chunk_ptr->empty())
    {
      add_text_entry_from_chunk_at_index(*chunk_ptr, text_index);
      text_index++;
    }
  }
}

void flx_pdf_text_extractor::flx_extraction_context::add_text_entry_from_chunk_at_index(const StringChunk& chunk, size_t index)
{
  string combined_text;
  double total_length = 0.0;
  Vector2 position(0, 0);
  double font_size = 12.0;
  const PdfFont* font = nullptr;

  size_t estimated_size = 0;
  for (const auto& stateful_str : chunk) {
    estimated_size += stateful_str.String.length();
  }
  combined_text.reserve(estimated_size);

  bool first_string = true;
  for (const auto& stateful_str : chunk)
  {
    combined_text += stateful_str.String;
    total_length += stateful_str.GetLengthRaw();
    if (first_string)
    {
      position = stateful_str.Position;
      font_size = stateful_str.State.PdfState.FontSize > 0 ? stateful_str.State.PdfState.FontSize : 12.0;
      font = stateful_str.State.PdfState.Font;
      first_string = false;
    }
  }

  if (!combined_text.empty())
  {
    string font_name = "Arial";
    if (font != nullptr) {
      try {
        font_name = font->GetName();
        size_t plus_pos = font_name.find('+');
        if (plus_pos != string::npos) {
          font_name = font_name.substr(plus_pos + 1);
        }
      } catch (const std::exception& e) {
        std::cout << "Warning: Failed to extract font name: " << e.what() << std::endl;
      }
    }
    
    auto& layout_text = m_texts[index];

    layout_text.text = flx_string(combined_text.c_str());
    layout_text.x = position.X;
    layout_text.y = m_pageHeight - position.Y;  // Convert PDF coords to layout coords
    layout_text.width = total_length > 0 ? total_length : combined_text.length() * font_size * 0.6;
    layout_text.height = font_size;
    layout_text.font_size = font_size;
    layout_text.font_family = flx_string(font_name.c_str());  // Now using real font name!
    layout_text.color = flx_string("#000000");  // TODO: Extract actual color
    layout_text.bold = false;  // TODO: Extract font style
    layout_text.italic = false;
  }
}
