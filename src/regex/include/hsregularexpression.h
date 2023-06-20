/*
 * Copyright (C) 2021 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KLOGG_HS_REGULAR_EXPRESSION
#define KLOGG_HS_REGULAR_EXPRESSION

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <QRegularExpression>
#include <QString>

#include "containers.h"

#ifdef KLOGG_HAS_HS
#include <hs.h>

#include "resourcewrapper.h"
#endif

#include "regularexpressionpattern.h"

using MatchedPatterns = std::string;

class DefaultRegularExpressionMatcher {
  public:
    explicit DefaultRegularExpressionMatcher(
        const klogg::vector<RegularExpressionPattern>& patterns )
    {
        std::transform(
            patterns.cbegin(), patterns.cend(), std::back_inserter( regexp_ ),
            []( const auto& pattern ) { return static_cast<QRegularExpression>( pattern ); } );
    }

    MatchedPatterns match( const std::string_view& utf8Data ) const
    {
        MatchedPatterns matchedPatterns( regexp_.size(), 0 );
        std::transform( regexp_.cbegin(), regexp_.cend(), matchedPatterns.begin(),
                        [ utf8Data ]( const auto& regexp ) {
                            return regexp
                                .match(
                                    QString::fromUtf8( utf8Data.data(), klogg::isize( utf8Data ) ) )
                                .hasMatch();
                            ;
                        } );

        return matchedPatterns;
    }

  private:
    klogg::vector<QRegularExpression> regexp_;
};

#ifdef KLOGG_HAS_HS

using HsScratch = UniqueResource<hs_scratch_t, hs_free_scratch>;
using HsDatabase = SharedResource<hs_database_t>;

struct HsMatcherContext {

    HsMatcherContext( std::size_t numberOfPatterns = 1 );

    void reset();

    MatchedPatterns matchingPatterns;

  private:
    MatchedPatterns matchingPatternsTemplate_;
};

class HsMatcher {
  public:
    HsMatcher() = default;
    HsMatcher( HsDatabase database, HsScratch scratch, std::size_t numberOfPatterns );

    HsMatcher( const HsMatcher& ) = delete;
    HsMatcher& operator=( const HsMatcher& ) = delete;

    HsMatcher( HsMatcher&& other ) = default;
    HsMatcher& operator=( HsMatcher&& other ) = default;

  protected:
    HsDatabase database_;
    HsScratch scratch_;

    mutable HsMatcherContext context_;
};

class HsSingleMatcher : public HsMatcher {
  public:
    HsSingleMatcher() = default;
    HsSingleMatcher( HsDatabase database, HsScratch scratch );

    MatchedPatterns match( const std::string_view& utf8Data ) const;
};

class HsMultiMatcher : public HsMatcher {
  public:
    HsMultiMatcher() = default;
    HsMultiMatcher( HsDatabase database, HsScratch scratch, std::size_t numberOfPatterns );

    MatchedPatterns match( const std::string_view& utf8Data ) const;
};

class HsNoopMatcher {
  public:
    MatchedPatterns match( const std::string_view& utf8Data ) const;
};

using MatcherVariant
    = std::variant<DefaultRegularExpressionMatcher, HsNoopMatcher, HsSingleMatcher, HsMultiMatcher>;

class HsRegularExpression {
  public:
    HsRegularExpression() = default;
    explicit HsRegularExpression( const RegularExpressionPattern& includePattern );
    explicit HsRegularExpression( const klogg::vector<RegularExpressionPattern>& patterns );

    HsRegularExpression( const HsRegularExpression& ) = delete;
    HsRegularExpression& operator=( const HsRegularExpression& ) = delete;

    HsRegularExpression( HsRegularExpression&& other ) = default;
    HsRegularExpression& operator=( HsRegularExpression&& other ) = default;

    bool isValid() const;
    QString errorString() const;

    MatcherVariant createMatcher() const;

  private:
    bool isHsValid() const;

  private:
    HsDatabase database_;
    HsScratch scratch_;

    klogg::vector<RegularExpressionPattern> patterns_;

    bool isValid_ = true;
    QString errorMessage_;
};
#else

using MatcherVariant = std::variant<DefaultRegularExpressionMatcher>;

class HsRegularExpression {
  public:
    HsRegularExpression() = default;

    explicit HsRegularExpression( const RegularExpressionPattern& includePattern )
        : HsRegularExpression( klogg::vector<RegularExpressionPattern>{ includePattern } )
    {
    }

    explicit HsRegularExpression( const klogg::vector<RegularExpressionPattern>& patterns )
        : patterns_( patterns )
    {
        for ( const auto& pattern : patterns_ ) {
            const auto& regex = static_cast<QRegularExpression>( pattern );
            if ( !regex.isValid() ) {
                isValid_ = false;
                errorString_ = regex.errorString();
                break;
            }
        }
    }

    bool isValid() const
    {
        return isValid_;
    }

    QString errorString() const
    {
        return errorString_;
    }

    MatcherVariant createMatcher() const
    {
        return MatcherVariant{ DefaultRegularExpressionMatcher( patterns_ ) };
    }

  private:
    bool isValid_ = true;
    QString errorString_;

    klogg::vector<RegularExpressionPattern> patterns_;
};

#endif

#endif