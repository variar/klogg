//
// Created by marvin on 23-7-24.
//

#pragma once

#include <vector>

#include <QString>

#include "containers.h"
#include "highlightedmatch.h"
#include "sgrparser.h"

namespace ansi {
using position_t = QString::size_type;
}

struct TextColorAttr {
    ANSI::Color color;
    ansi::position_t start;
    ansi::position_t len;
    operator HighlightedMatch();
};

struct ColorfulText {
    QString text;
    klogg::vector<TextColorAttr> color;
};

// ANSI: start, data
using SGRSequence = std::pair<ansi::position_t, QString>;

class CSIFilter {
  public:
    CSIFilter() = default;
    ~CSIFilter() = default;

    static klogg::vector<SGRSequence> filter( QString& text );
};

class ColorfulTextParser {
  public:
    enum class Mode {
        MARKED_TEXT,
        ALL_TEXT,
    };

  public:
    explicit ColorfulTextParser( const ANSI::TextAttribute& defaultAttr,
                                 const ANSI::TextAttribute& currentAttr );

    // QString
    ColorfulText parse( QString strings, Mode mode = Mode::ALL_TEXT );

    klogg::vector<ColorfulText> parse( const klogg::vector<QString>& strings,
                                       Mode mode = Mode::ALL_TEXT );

  private:
    void markedStringToText( klogg::vector<ColorfulText>& textList,
                             const klogg::vector<SGRSequence>& sgrSeqs, QString&& string );
    void allStringToText( klogg::vector<ColorfulText>& textList,
                          const klogg::vector<SGRSequence>& sgrSeqs, QString&& string );

  private:
    ANSI::TextAttribute currentTextAttr_;
    ANSI::SGRParser sgrParser_;
};
