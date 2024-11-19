//
// Created by marvin on 23-7-19.
//

#include "sgrparser.h"

#include <cassert>
#include <limits>

#include "ansi.h"

namespace ANSI {

using ParseResult = SGRParseCore::ParseResult;
using ColorIndex = ColorTable::ColorIndex;

SGRParser::SGRParser( const TextAttribute& defaultTextAttr )
    : defaultTextAttr_{ TextAttribute::State::DEFAULT, defaultTextAttr.color }
{
}

SGRParser::SGRParseReturn SGRParser::parseSGRSequence( const TextAttribute& currentTextAttr,
                                                       const std::string& sequence )
{
    // sequence start byte + CSI final byte size error, return current color
    if ( sequence.size() < SequenceStartCnt::HEAD_CNT + 1 ) {
        return { Return::PARSE_ERROR, currentTextAttr };
    }
    // check sequence format
    if ( sequence[ 0 ] != SequenceFirst::EXC || sequence[ 1 ] != SequenceSecond::CSI
         || sequence.back() != CSIFinalBytes::SGR ) {
        return { Return::PARSE_ERROR, currentTextAttr };
    }
    // remove sequence start byte
    std::string_view seqView( sequence );
    seqView.remove_prefix( HEAD_CNT );

    SGRParseCore core{};
    SGRParseCore::ReturnVal ctxRet;
    SGRParseReturn ret{ Return::PARSE_SUCC, currentTextAttr };

    while ( !seqView.empty() ) {
        // continuous parsing and logging of results at each step
        ctxRet = core.parse( seqView );
        switch ( core.result() ) {
        case ParseResult::RESULT_FRONT_COLOR: {
            ret.second.state = TextAttribute::State::CUSTOM;
            ret.second.color.front = core.color();
        } break;
        case ParseResult::RESULT_BACK_COLOR: {
            ret.second.state = TextAttribute::State::CUSTOM;
            ret.second.color.back = core.color();
        } break;
        case ParseResult::RESULT_DEFAULT_FRONT_COLOR: {
            ret.second.color.front = defaultTextAttr_.color.front;
        } break;
        case ParseResult::RESULT_DEFAULT_BACK_COLOR: {
            ret.second.color.back = defaultTextAttr_.color.back;
        } break;
        case ParseResult::RESULT_DEFAULT_TEXT_ATTR: {
            ret.second = defaultTextAttr_;
        } break;
        case ParseResult::RESULT_CURRENT_TEXT_ATTR:
        case ParseResult::RESULT_UNSUPPORTED_ATTR: {
            // keep parsed attribute, do nothing
            // ret.second = currentTextAttr;
        } break;
        }

        core.reset();

        // RETURN_ERROR_BREAK aborts parsing and invalidates parsed results
        if ( ctxRet == SGRParseCore::ReturnVal::RETURN_ERROR_BREAK ) {
            ret = { Return::PARSE_ERROR, currentTextAttr };
            break;
        }
    }

    return ret;
}

enum class ConvertRet {
    NOT_U8 = -2,
    NOT_NUM = -1,
    SUCCESS = 0,
};

std::pair<ConvertRet, uint8_t> base10ToU8( const std::string_view& num )
{
    bool has = false;
    int value = 0;

    for ( auto ch : num ) {
        if ( ch > '9' || ch < '0' ) {
            has = false;
            break;
        }

        value *= 10;
        value += ( ch - '0' );
        has = true;
    }

    if ( has ) {
        if ( value > std::numeric_limits<uint8_t>::max() ) {
            return { ConvertRet::NOT_U8, {} };
        }
        return { ConvertRet::SUCCESS, static_cast<uint8_t>( value ) };
    }
    return { ConvertRet::NOT_NUM, {} };
}

SGRParseCore::SGRParseCore()
    : result_( ParseResult::RESULT_CURRENT_TEXT_ATTR )
    , state_( ParseState::STATE_WAIT_FIRST_PARAMETER )
    , color_()
    , bit24Valid_( true )
{
}

SGRParseCore::SGRParseCore( ParseResult result, RGB rgb, ParseState s )
    : result_( result )
    , state_( s )
    , color_( rgb )
    , bit24Valid_( true )
{
}

SGRParseCore::ReturnVal SGRParseCore::stringToParameter( const std::string_view& in, uint8_t& out )
{
    auto [ ret, value ] = base10ToU8( in );
    // not number parse break, keep current text attribute
    if ( ret == ConvertRet::NOT_NUM ) {
        result_ = ParseResult ::RESULT_CURRENT_TEXT_ATTR;
        return ReturnVal::RETURN_ERROR_BREAK;
    }
    // not u8 parse continue, use last parse result
    else if ( ret == ConvertRet::NOT_U8 ) {
        state_ = ParseState::STATE_WAIT_FIRST_PARAMETER;
        return ReturnVal::RETURN_ERROR_CONTINUE;
    }
    out = value;
    return ReturnVal::RETURN_SUCCESS_CONTINUE;
}

SGRParseCore::ReturnVal SGRParseCore::setFirstParameter( const std::string_view& num )
{
    // in the first parameter, the default value is 0, which will reset all text attributes.
    if ( num.empty() ) {
        result_ = ParseResult::RESULT_DEFAULT_TEXT_ATTR;
        return ReturnVal::RETURN_SUCCESS_BREAK;
    }

    uint8_t value;
    auto ret = stringToParameter( num, value );
    if ( ret != ReturnVal::RETURN_SUCCESS_CONTINUE ) {
        return ret;
    }

    *this = ColorTable::index( ColorTable::ColorIndex( value ) );
    // UNKNOWN is not support, so continue
    if ( result_ == ParseResult::RESULT_UNSUPPORTED_ATTR ) {
        return ReturnVal::RETURN_ERROR_CONTINUE;
    }

    // when index valid result, state is STATE_WAIT_FIRST_PARAMETER,
    return ( state_ == ParseState::STATE_WAIT_FIRST_PARAMETER
                 ? ReturnVal::RETURN_SUCCESS_BREAK
                 : ReturnVal::RETURN_SUCCESS_CONTINUE );
}

SGRParseCore::ReturnVal SGRParseCore::setColorVersion( const std::string_view& num )
{
    // before version parameter is 38, empty parameter will reset parse state, and use last color
    if ( num.empty() ) {
        state_ = ParseState::STATE_WAIT_FIRST_PARAMETER;
        return ReturnVal::RETURN_ERROR_CONTINUE;
    }

    uint8_t value;
    auto ret = stringToParameter( num, value );
    if ( ret != ReturnVal::RETURN_SUCCESS_CONTINUE ) {
        return ret;
    }

    // update state
    if ( ColorVersion::BIT_8 == ColorVersion( value ) ) {
        state_ = ParseState::STATE_WAIT_BIT_8_ARGS;
    }
    else if ( ColorVersion::BIT_24 == ColorVersion( value ) ) {
        state_ = ParseState::STATE_WAIT_BIT_24_ARGS_R;
    }
    else {
        // invalid value will reset parse state, and use last parse result
        state_ = ParseState::STATE_WAIT_FIRST_PARAMETER;
        return ReturnVal::RETURN_ERROR_CONTINUE;
    }

    return ReturnVal::RETURN_SUCCESS_CONTINUE;
}

SGRParseCore::ReturnVal SGRParseCore::setBit8Color( const std::string_view& num )
{
    // 8-bit color parameter empty, will use last color, then parse continue
    if ( num.empty() ) {
        state_ = ParseState::STATE_WAIT_FIRST_PARAMETER;
        return ReturnVal::RETURN_ERROR_CONTINUE;
    }

    uint8_t value;
    auto ret = stringToParameter( num, value );
    if ( ret != ReturnVal::RETURN_SUCCESS_CONTINUE ) {
        return ret;
    }

    // reference: https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit
    // Standard colors
    if ( value <= 7 ) {
        // storage result color position, after index color, recover it
        auto result = result_;
        *this = ColorTable::index( ColorTable::ColorIndex( ColorTable::F_BLACK + value ) );
        result_ = result;
    }
    // High-intensity colors
    else if ( value >= 8 && value <= 15 ) {
        // storage result color position, after index color, recover it
        auto result = result_;
        *this = ColorTable::index( ColorTable::ColorIndex( ColorTable::F_BRIGHT_BLACK + value ) );
        result_ = result;
    }
    // Grayscale colors
    else if ( value >= 232 ) {
        auto colorValue = uint8_t( ( value - 232 ) * 10 + 8 );
        color_.r = colorValue;
        color_.g = colorValue;
        color_.b = colorValue;
        state_ = ParseState::STATE_WAIT_FIRST_PARAMETER;
    }
    // 216 colors
    else {
        constexpr uint8_t colorValue[]{ 0, 95, 135, 175, 215, 255 };
        auto val = value - 16;
        auto remainder = val % 36;
        color_.r = colorValue[ val / 36 ];
        color_.g = colorValue[ remainder / 6 ];
        color_.b = colorValue[ remainder % 6 ];
        state_ = ParseState::STATE_WAIT_FIRST_PARAMETER;
    }

    return ReturnVal::RETURN_SUCCESS_BREAK;
}

SGRParseCore::ReturnVal SGRParseCore::setBit24Color( const std::string_view& num )
{
    if ( num.empty() ) {
        // if bit24 color parameter empty, all the 24bit color parameters are invalid.
        bit24Valid_ = false;
    }

    // bit24Valid is false, no need to convert num and set color
    if ( bit24Valid_ ) {
        uint8_t value;
        auto ret = stringToParameter( num, value );
        if ( ret != ReturnVal::RETURN_SUCCESS_CONTINUE ) {
            return ret;
        }
        setBit24ColorValue( value );

        // if state is STATE_WAIT_FIRST_PARAMETER, exist color, so break
        return ( state_ == ParseState::STATE_WAIT_FIRST_PARAMETER
                     ? ReturnVal::RETURN_SUCCESS_BREAK
                     : ReturnVal::RETURN_SUCCESS_CONTINUE );
    }
    else {
        setBit24ColorValue( 0 );
        // restore state after ignoring invalid parameters
        if ( state_ == ParseState::STATE_WAIT_FIRST_PARAMETER ) {
            bit24Valid_ = true;
        }
        return ReturnVal::RETURN_ERROR_CONTINUE;
    }
}

// save color value and change state
void SGRParseCore::setBit24ColorValue( uint8_t num )
{
    switch ( state_ ) {
    case ParseState::STATE_WAIT_BIT_24_ARGS_R: {
        color_.r = num;
        state_ = ParseState::STATE_WAIT_BIT_24_ARGS_G;
    } break;
    case ParseState::STATE_WAIT_BIT_24_ARGS_G: {
        color_.g = num;
        state_ = ParseState::STATE_WAIT_BIT_24_ARGS_B;
    } break;
    case ParseState::STATE_WAIT_BIT_24_ARGS_B: {
        color_.b = num;
        state_ = ParseState::STATE_WAIT_FIRST_PARAMETER;
    } break;
    default:
        // If the code is not written correctly, it will go here
        assert( false );
    }
}

SGRParseCore::ReturnVal SGRParseCore::parse( std::string_view& seqs )
{
    auto pos = seqs.find_first_of( ";:m" );
    ReturnVal parseRet = ReturnVal::RETURN_SUCCESS_CONTINUE;

    while ( pos != std::string_view::npos ) {
        std::string_view num{ seqs.data(), pos };

        switch ( state_ ) {
        case ParseState::STATE_WAIT_FIRST_PARAMETER: {
            parseRet = setFirstParameter( num );
        } break;
        case ParseState::STATE_WAIT_VERSION: {
            parseRet = setColorVersion( num );
        } break;
        case ParseState::STATE_WAIT_BIT_8_ARGS: {
            parseRet = setBit8Color( num );
        } break;
        case ParseState::STATE_WAIT_BIT_24_ARGS_R: {
        case ParseState::STATE_WAIT_BIT_24_ARGS_G:
        case ParseState::STATE_WAIT_BIT_24_ARGS_B:
            parseRet = setBit24Color( num );
        } break;
        }

        seqs.remove_prefix( pos + 1 );
        pos = seqs.find_first_of( ";:m" );

        // if return BREAK, return current parse result
        if ( ReturnVal::RETURN_SUCCESS_BREAK == parseRet
             || ReturnVal::RETURN_ERROR_BREAK == parseRet ) {
            break;
        }
    }

    return parseRet;
}

// reference: https://en.wikipedia.org/wiki/ANSI_escape_code#3-bit_and_4-bit
// {index, {result, color, state}}
// If it is a valid color, state must be STATE_WAIT_FIRST_PARAMETER
std::map<ColorIndex, SGRParseCore> ColorTable::colorTable{
    // reset to default
    { ColorIndex::RESET_DEFAULT, { ParseResult::RESULT_DEFAULT_TEXT_ATTR, {} } },

    // 3/4-bit front color
    { ColorIndex::F_BLACK, { ParseResult::RESULT_FRONT_COLOR, { 1, 1, 1 } } },
    { ColorIndex::F_RED, { ParseResult::RESULT_FRONT_COLOR, { 222, 56, 43 } } },
    { ColorIndex::F_GREEN, { ParseResult::RESULT_FRONT_COLOR, { 57, 181, 74 } } },
    { ColorIndex::F_YELLOW, { ParseResult::RESULT_FRONT_COLOR, { 255, 199, 6 } } },
    { ColorIndex::F_BLUE, { ParseResult::RESULT_FRONT_COLOR, { 0, 111, 184 } } },
    { ColorIndex::F_MAGENTA, { ParseResult::RESULT_FRONT_COLOR, { 118, 38, 113 } } },
    { ColorIndex::F_CYAN, { ParseResult::RESULT_FRONT_COLOR, { 44, 181, 233 } } },
    { ColorIndex::F_WHITE, { ParseResult::RESULT_FRONT_COLOR, { 204, 204, 204 } } },

    // custom front color
    { ColorIndex::F_CUSTOM_COLOR,
      { ParseResult::RESULT_FRONT_COLOR, {}, SGRParseCore::ParseState::STATE_WAIT_VERSION } },

    // default front color
    { ColorIndex::F_DEFAULT_COLOR, { ParseResult::RESULT_DEFAULT_FRONT_COLOR, {} } },

    // 3/4-bit back color
    { ColorIndex::B_BLACK, { ParseResult::RESULT_BACK_COLOR, { 1, 1, 1 } } },
    { ColorIndex::B_RED, { ParseResult::RESULT_BACK_COLOR, { 222, 56, 43 } } },
    { ColorIndex::B_GREEN, { ParseResult::RESULT_BACK_COLOR, { 57, 181, 74 } } },
    { ColorIndex::B_YELLOW, { ParseResult::RESULT_BACK_COLOR, { 255, 199, 6 } } },
    { ColorIndex::B_BLUE, { ParseResult::RESULT_BACK_COLOR, { 0, 111, 184 } } },
    { ColorIndex::B_MAGENTA, { ParseResult::RESULT_BACK_COLOR, { 118, 38, 113 } } },
    { ColorIndex::B_CYAN, { ParseResult::RESULT_BACK_COLOR, { 44, 181, 233 } } },
    { ColorIndex::B_WHITE, { ParseResult::RESULT_BACK_COLOR, { 204, 204, 204 } } },

    // custom back color
    { ColorIndex::B_CUSTOM_COLOR,
      { ParseResult::RESULT_BACK_COLOR, {}, SGRParseCore::ParseState::STATE_WAIT_VERSION } },

    // default front color
    { ColorIndex::B_DEFAULT_COLOR, { ParseResult::RESULT_DEFAULT_BACK_COLOR, {} } },

    // 3/4-bit front bright color
    { ColorIndex::F_BRIGHT_BLACK, { ParseResult::RESULT_FRONT_COLOR, { 128, 128, 128 } } },
    { ColorIndex::F_BRIGHT_RED, { ParseResult::RESULT_FRONT_COLOR, { 255, 0, 0 } } },
    { ColorIndex::F_BRIGHT_GREEN, { ParseResult::RESULT_FRONT_COLOR, { 0, 255, 0 } } },
    { ColorIndex::F_BRIGHT_YELLOW, { ParseResult::RESULT_FRONT_COLOR, { 255, 255, 0 } } },
    { ColorIndex::F_BRIGHT_BLUE, { ParseResult::RESULT_FRONT_COLOR, { 0, 0, 255 } } },
    { ColorIndex::F_BRIGHT_MAGENTA, { ParseResult::RESULT_FRONT_COLOR, { 255, 0, 255 } } },
    { ColorIndex::F_BRIGHT_CYAN, { ParseResult::RESULT_FRONT_COLOR, { 0, 255, 255 } } },
    { ColorIndex::F_BRIGHT_WHITE, { ParseResult::RESULT_FRONT_COLOR, { 255, 255, 255 } } },

    // 3/4-bit back bright color
    { ColorIndex::B_BRIGHT_BLACK, { ParseResult::RESULT_BACK_COLOR, { 128, 128, 128 } } },
    { ColorIndex::B_BRIGHT_RED, { ParseResult::RESULT_BACK_COLOR, { 255, 0, 0 } } },
    { ColorIndex::B_BRIGHT_GREEN, { ParseResult::RESULT_BACK_COLOR, { 0, 255, 0 } } },
    { ColorIndex::B_BRIGHT_YELLOW, { ParseResult::RESULT_BACK_COLOR, { 255, 255, 0 } } },
    { ColorIndex::B_BRIGHT_BLUE, { ParseResult::RESULT_BACK_COLOR, { 0, 0, 255 } } },
    { ColorIndex::B_BRIGHT_MAGENTA, { ParseResult::RESULT_BACK_COLOR, { 255, 0, 255 } } },
    { ColorIndex::B_BRIGHT_CYAN, { ParseResult::RESULT_BACK_COLOR, { 0, 255, 255 } } },
    { ColorIndex::B_BRIGHT_WHITE, { ParseResult::RESULT_BACK_COLOR, { 255, 255, 255 } } },
};

SGRParseCore ColorTable::index( ColorIndex num )
{
    auto ret = colorTable.find( num );
    if ( ret == colorTable.end() ) {
        return { SGRParseCore::ParseResult::RESULT_UNSUPPORTED_ATTR,
                 {},
                 SGRParseCore ::ParseState::STATE_WAIT_FIRST_PARAMETER };
    }
    return ret->second;
}

} // namespace ANSI