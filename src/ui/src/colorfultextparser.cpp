//
// Created by marvin on 23-7-24.
//

#include "colorfultextparser.h"

#include <QRegularExpression>
#include <QString>

#ifndef Q_OS_WIN
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
using RemoveCount = decltype( ( (QRegularExpressionMatch*)nullptr )->capturedLength() );
#pragma GCC diagnostic pop
#else
using RemoveCount = decltype( ( (QRegularExpressionMatch*)nullptr )->capturedLength() );
#endif

static QString pattern{ "\\x1B\\[([0-9]{0,4}([;:][0-9]{1,3})*)?[mK]" };

using namespace ANSI;

TextColorAttr::operator HighlightedMatch()
{
    return HighlightedMatch( LineColumn{ (LineColumn::UnderlyingType)start },
                             LineLength{ (LineLength::UnderlyingType)len },
                             QColor{ color.front.r, color.front.g, color.front.b },
                             QColor{ color.back.r, color.back.g, color.back.b } );
}

klogg::vector<SGRSequence> CSIFilter::filter( QString& string )
{
    klogg::vector<SGRSequence> ansiSeqs;
    RemoveCount removeAnsiCharCnt = 0;
    QRegularExpression reg( pattern );
    auto allResult = reg.globalMatch( string );

    while ( allResult.hasNext() ) {
        auto result = allResult.next();

#if QT_VERSION >= QT_VERSION_CHECK( 5, 10, 0 )
        auto ref = result.capturedView();
#else
        auto ref = result.capturedRef();
#endif

        auto startPos = result.capturedStart() - removeAnsiCharCnt;
        ansiSeqs.emplace_back( startPos, ref.toString() );
        removeAnsiCharCnt += result.capturedLength();
    }
    string.remove( reg );
    return ansiSeqs;
}

ColorfulTextParser::ColorfulTextParser( const ANSI::TextAttribute& defaultAttr,
                                        const ANSI::TextAttribute& currentAttr )
    : currentTextAttr_( currentAttr )
    , sgrParser_( defaultAttr )
{
}

ColorfulText ColorfulTextParser::parse( QString string, Mode mode )
{
    klogg::vector<ColorfulText> textList;
    auto sgrSeqs = CSIFilter::filter( string );
    if ( mode == Mode::ALL_TEXT ) {
        allStringToText( textList, sgrSeqs, std::move( string ) );
    }
    else if ( mode == Mode::MARKED_TEXT ) {
        markedStringToText( textList, sgrSeqs, std::move( string ) );
    }

    return textList.empty() ? ColorfulText{} : textList.front();
}

klogg::vector<ColorfulText> ColorfulTextParser::parse( const klogg::vector<QString>& strings,
                                                       ColorfulTextParser::Mode mode )
{
    klogg::vector<ColorfulText> textList;
    for ( auto string : strings ) {
        auto sgrSeqs = CSIFilter::filter( string );
        if ( mode == Mode::ALL_TEXT ) {
            allStringToText( textList, sgrSeqs, std::move( string ) );
        }
        else if ( mode == Mode::MARKED_TEXT ) {
            markedStringToText( textList, sgrSeqs, std::move( string ) );
        }
    }
    return textList;
}

void ColorfulTextParser::markedStringToText( klogg::vector<ColorfulText>& textList,
                                             const klogg::vector<SGRSequence>& sgrSeqs,
                                             QString&& string )
{
    ColorfulText colorfulText{ string, {} };
    textList.emplace_back( std::move( colorfulText ) );
    auto& colors = textList.back().color;

    // empty SGR sequences process
    if ( sgrSeqs.empty() ) {
        if ( currentTextAttr_.state == TextAttribute::State::CUSTOM ) {
            TextColorAttr desc{ currentTextAttr_.color, 0, string.size() };
            colors.emplace_back( desc );
        }
        return;
    }

    // get first colorful text pos and attribute
    auto firstResult
        = sgrParser_.parseSGRSequence( currentTextAttr_, sgrSeqs[ 0 ].second.toStdString() );
    auto curPos = sgrSeqs[ 0 ].first;
    auto curTextAttr = firstResult.second;

    for ( size_t i = 0; i < sgrSeqs.size(); ++i ) {
        // if exist next colorful text , parse sequence
        // else set next pos to string end
        ansi::position_t nextPos;
        decltype( curTextAttr ) nextTextAttr;
        if ( i + 1 < sgrSeqs.size() ) {
            auto result
                = sgrParser_.parseSGRSequence( curTextAttr, sgrSeqs[ i + 1 ].second.toStdString() );
            nextPos = sgrSeqs[ i + 1 ].first;
            nextTextAttr = result.second;
        }
        else {
            nextPos = string.size();
            nextTextAttr = curTextAttr;
        }

        if ( curTextAttr.state == TextAttribute::State::CUSTOM ) {
            TextColorAttr desc{ curTextAttr.color, curPos, nextPos - curPos };
            colors.emplace_back( desc );
        }

        curPos = nextPos;
        curTextAttr = nextTextAttr;
    }
    currentTextAttr_ = curTextAttr;
}

void ColorfulTextParser::allStringToText( klogg::vector<ColorfulText>& textList,
                                          const klogg::vector<SGRSequence>& sgrSeqs,
                                          QString&& string )
{
    ColorfulText colorfulText{ string, {} };
    textList.emplace_back( std::move( colorfulText ) );
    auto& colors = textList.back().color;

    // empty SGR sequences process
    if ( sgrSeqs.empty() ) {
        TextColorAttr desc{ currentTextAttr_.color, 0, string.size() };
        colors.emplace_back( desc );
        return;
    }

    ansi::position_t curPos = 0;
    auto curTextAttr = currentTextAttr_;

    // first text push back
    auto firstResult
        = sgrParser_.parseSGRSequence( curTextAttr, sgrSeqs[ 0 ].second.toStdString() );
    auto nextPos = sgrSeqs[ 0 ].first;
    auto nextTextAttr = firstResult.second;
    if ( curPos < nextPos ) {
        TextColorAttr desc{ curTextAttr.color, curPos, nextPos - curPos };
        colors.emplace_back( desc );
    }

    // update context
    curPos = nextPos;
    curTextAttr = nextTextAttr;

    for ( size_t i = 0; i < sgrSeqs.size(); ++i ) {
        if ( i + 1 < sgrSeqs.size() ) {
            auto result
                = sgrParser_.parseSGRSequence( curTextAttr, sgrSeqs[ i + 1 ].second.toStdString() );
            nextPos = sgrSeqs[ i + 1 ].first;
            nextTextAttr = result.second;
        }
        else {
            nextPos = string.size();
            nextTextAttr = curTextAttr;
        }

        TextColorAttr desc{ curTextAttr.color, curPos, nextPos - curPos };
        colors.emplace_back( desc );

        curPos = nextPos;
        curTextAttr = nextTextAttr;
    }
    currentTextAttr_ = curTextAttr;
}
