//
// Created by marvin on 23-7-19.
//
#pragma once

#include <map>
#include <string>
#include <utility>

#include "ansi.h"

namespace ANSI {

struct RGB {
    uint8_t r, g, b;
};

template <typename T>
RGB toRgb( T color )
{
    return { (uint8_t)color.red(), (uint8_t)color.green(), (uint8_t)color.blue() };
}

struct Color {
    RGB front;
    RGB back;
};

struct TextAttribute {
    enum class State {
        DEFAULT,
        CUSTOM,
    };

    State state;
    Color color;
};

class SGRParser {
  public:
    using SGRParseReturn = std::pair<Return, TextAttribute>;

  public:
    explicit SGRParser( const TextAttribute& defaultTextAttr );
    ~SGRParser() = default;

    SGRParser( const SGRParser& ) = delete;
    SGRParser( SGRParser&& ) = delete;
    SGRParser& operator=( const SGRParser& ) = delete;
    SGRParser& operator=( SGRParser&& ) = delete;

    /*
     * @param currentTextAttr   Properties of the current text
     * @param sequence          SGR sequence, example: "\033[31m"
     * @return                  {return value, parsed text attribute}
     *
     * If the return value is ERROR, the parsed value is still guaranteed to be valid.
     */
    SGRParseReturn parseSGRSequence( const TextAttribute& currentTextAttr,
                                     const std::string& sequence );

  private:
    TextAttribute defaultTextAttr_;
};

class ColorTable;

class SGRParseCore {
    friend class ColorTable;

  public:
    enum class ReturnVal {
        RETURN_SUCCESS_BREAK,
        RETURN_SUCCESS_CONTINUE,

        RETURN_ERROR_BREAK,
        RETURN_ERROR_CONTINUE,
    };

    enum class ParseResult {
        RESULT_UNSUPPORTED_ATTR,
        RESULT_FRONT_COLOR,
        RESULT_BACK_COLOR,
        RESULT_DEFAULT_FRONT_COLOR,
        RESULT_DEFAULT_BACK_COLOR,
        RESULT_DEFAULT_TEXT_ATTR,
        RESULT_CURRENT_TEXT_ATTR,
    };

  private:
    enum class ColorVersion : uint8_t {
        BIT_8 = 5,
        BIT_24 = 2,
    };

    enum class ParseState {
        STATE_WAIT_FIRST_PARAMETER,
        STATE_WAIT_VERSION,
        STATE_WAIT_BIT_8_ARGS,
        STATE_WAIT_BIT_24_ARGS_R,
        STATE_WAIT_BIT_24_ARGS_G,
        STATE_WAIT_BIT_24_ARGS_B,
    };

  public:
    SGRParseCore();
    ~SGRParseCore() = default;

    SGRParseCore( const SGRParseCore& ) = default;
    SGRParseCore( SGRParseCore&& ) = default;
    SGRParseCore& operator=( const SGRParseCore& ) = default;
    SGRParseCore& operator=( SGRParseCore&& ) = default;

    ReturnVal parse( std::string_view& seqs );

    inline void reset()
    {
        new ( this ) SGRParseCore();
    }

    inline ParseResult result()
    {
        return result_;
    }

    inline RGB color()
    {
        return color_;
    }

  private:
    SGRParseCore( ParseResult result, RGB rgb,
                  ParseState s = ParseState::STATE_WAIT_FIRST_PARAMETER );

    ReturnVal stringToParameter( const std::string_view& in, uint8_t& out );

    ReturnVal setFirstParameter( const std::string_view& num );
    ReturnVal setColorVersion( const std::string_view& num );
    ReturnVal setBit8Color( const std::string_view& num );
    ReturnVal setBit24Color( const std::string_view& num );
    void setBit24ColorValue( uint8_t num );

  private:
    ParseResult result_;
    ParseState state_;
    RGB color_;
    bool bit24Valid_;
};

class ColorTable {
  public:
    enum ColorIndex : uint8_t {
        RESET_DEFAULT = 0,

        // 3/4-bit front color
        F_BLACK = 30,
        F_RED = 31,
        F_GREEN = 32,
        F_YELLOW = 33,
        F_BLUE = 34,
        F_MAGENTA = 35,
        F_CYAN = 36,
        F_WHITE = 37,

        // custom front color
        F_CUSTOM_COLOR = 38,
        // default front color
        F_DEFAULT_COLOR = 39,

        // 3/4-bit back color
        B_BLACK = 40,
        B_RED = 41,
        B_GREEN = 42,
        B_YELLOW = 43,
        B_BLUE = 44,
        B_MAGENTA = 45,
        B_CYAN = 46,
        B_WHITE = 47,

        // custom back color
        B_CUSTOM_COLOR = 48,
        // default back color
        B_DEFAULT_COLOR = 49,

        // 3/4-bit front bright color
        F_BRIGHT_BLACK = 90,
        F_BRIGHT_RED = 91,
        F_BRIGHT_GREEN = 92,
        F_BRIGHT_YELLOW = 93,
        F_BRIGHT_BLUE = 94,
        F_BRIGHT_MAGENTA = 95,
        F_BRIGHT_CYAN = 96,
        F_BRIGHT_WHITE = 97,

        // 3/4-bit back bright color
        B_BRIGHT_BLACK = 100,
        B_BRIGHT_RED = 101,
        B_BRIGHT_GREEN = 102,
        B_BRIGHT_YELLOW = 103,
        B_BRIGHT_BLUE = 104,
        B_BRIGHT_MAGENTA = 105,
        B_BRIGHT_CYAN = 106,
        B_BRIGHT_WHITE = 107,
    };

  public:
    static SGRParseCore index( ColorIndex num );

  private:
    static std::map<ColorIndex, SGRParseCore> colorTable;
};

} // namespace ANSI
