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

#include <algorithm>
#include <exception>
#include <memory>
#include <qregularexpression.h>
#include <string>
#include <variant>

#include "configuration.h"
#include "containers.h"
#include "log.h"
#include "regularexpressionpattern.h"
#include "uuid.h"

#include "booleanevaluator.h"
#include "regularexpression.h"

namespace {
klogg::vector<RegularExpressionPattern>
parseBooleanExpressions( QString& pattern, bool isCaseSensitive, bool isPlainText )
{
    if ( !pattern.contains( '"' ) ) {
        throw std::runtime_error( "Patterns must be enclosed in quotes" );
    }

    klogg::vector<RegularExpressionPattern> subPatterns;
    subPatterns.reserve( static_cast<size_t>( pattern.size() ) );

    int currentIndex = 0;
    int leftQuote = -1;
    int rightQuote = -1;

    while ( currentIndex < pattern.size() ) {
        leftQuote = type_safe::narrow_cast<int>( pattern.indexOf( QChar( '"' ), currentIndex ) );
        if ( leftQuote < 0 ) {
            break;
        }

        currentIndex = leftQuote + 1;
        if ( leftQuote > 0 && pattern[ leftQuote - 1 ] == QChar( '\\' ) ) {
            leftQuote = -1;
            continue;
        }

        while ( currentIndex < pattern.size() ) {
            rightQuote
                = type_safe::narrow_cast<int>( pattern.indexOf( QChar( '"' ), currentIndex ) );
            if ( rightQuote < 0 ) {
                break;
            }

            currentIndex = rightQuote + 1;
            if ( rightQuote > 0 && pattern[ rightQuote - 1 ] == QChar( '\\' ) ) {
                rightQuote = -1;
                continue;
            }

            break;
        }

        if ( rightQuote < 0 ) {
            break;
        }

        const auto subPatternLength = rightQuote - leftQuote - 1;
        auto subPattern = pattern.mid( leftQuote + 1, subPatternLength );
        subPattern.replace( "\\\"", "\"" );

        subPatterns.emplace_back( subPattern, isCaseSensitive, false, false, isPlainText );

        pattern.replace( leftQuote, subPatternLength + 2,
                         QString::fromStdString( subPatterns.back().id() ) );

        currentIndex = 0;
        leftQuote = -1;
        rightQuote = -1;
    }

    if ( pattern.contains( '"' ) ) {
        throw std::runtime_error( "Pattern has unmatched quotes" );
    }

    pattern = pattern.toLower();
    LOG_INFO << "Parsed pattern: " << pattern;
    QRegularExpression finalPatternCheck( "^(and|nand|or|nor|xor|xnor|not|[ ()!|&]|p_[0-9]+)+$" );
    if ( !finalPatternCheck.match( pattern ).hasMatch() ) {
        throw std::runtime_error( "Sub-patterns must be enclosed in quotes" );
    }

    return subPatterns;
}

} // namespace

RegularExpression::RegularExpression( const RegularExpressionPattern& pattern )
    : isInverse_( pattern.isExclude )
    , isBooleanCombination_( pattern.isBoolean )
    , expression_( pattern.pattern )
{
    try {
        if ( pattern.isBoolean ) {
            subPatterns_ = parseBooleanExpressions( expression_, pattern.isCaseSensitive,
                                                    pattern.isPlainText );

            BooleanExpressionEvaluator evaluator{ expression_.toStdString(), subPatterns_ };
            if ( !evaluator.isValid() ) {
                isValid_ = false;
                errorString_ = QString::fromStdString( evaluator.errorString() );
                return;
            }
        }
        else {
            subPatterns_.emplace_back( pattern );
            expression_ = QString::fromStdString( subPatterns_.front().id() );
        }

        hsExpression_ = HsRegularExpression( subPatterns_ );
        isValid_ = hsExpression_.isValid();
        errorString_ = hsExpression_.errorString();

    } catch ( std::exception& err ) {
        isValid_ = false;
        errorString_ = err.what();
    }
}

bool RegularExpression::isValid() const
{
    return isValid_;
}

QString RegularExpression::errorString() const
{
    return errorString_;
}

std::unique_ptr<PatternMatcher> RegularExpression::createMatcher() const
{
    return std::make_unique<PatternMatcher>( *this );
}

namespace matching {

bool hasSingleMatch( std::string_view line, const MatcherVariant& matcher,
                     BooleanExpressionEvaluator* )
{
    const auto result
        = std::visit( [ &line ]( const auto& m ) { return m.match( line ); }, matcher );

    return !result.empty() && result[ 0 ] > 0;
}

bool hasCombinedMatch( std::string_view line, const MatcherVariant& matcher,
                       BooleanExpressionEvaluator* evaluator )
{
    auto result = std::visit( [ &line ]( const auto& m ) { return m.match( line ); }, matcher );
    return evaluator && evaluator->evaluate( result );
}

bool hasInverseSingleMatch( std::string_view line, const MatcherVariant& matcher,
                            BooleanExpressionEvaluator* evaluator )
{
    return !hasSingleMatch( line, matcher, evaluator );
}

bool hasInverseCombinedMatch( std::string_view line, const MatcherVariant& matcher,
                              BooleanExpressionEvaluator* evaluator )
{
    return !hasCombinedMatch( line, matcher, evaluator );
}

} // namespace matching

PatternMatcher::PatternMatcher( const RegularExpression& expression )
    : isInverse_( expression.isInverse_ )
    , isBooleanCombination_( expression.isBooleanCombination_ )
    , mainPatternId_( expression.subPatterns_.front().id() )
    , matcher_( expression.hsExpression_.createMatcher() )
{
    const auto& config = Configuration::get();
    const auto useHyperscanEngine = config.regexpEngine() == RegexpEngine::Hyperscan;
    if ( !useHyperscanEngine ) {
        matcher_ = DefaultRegularExpressionMatcher( expression.subPatterns_ );
    }

    if ( expression.isBooleanCombination_ ) {
        evaluator_ = std::make_unique<BooleanExpressionEvaluator>(
            expression.expression_.toStdString(), expression.subPatterns_ );
    }

    if ( !isBooleanCombination_ ) {
        hasMatchImpl_ = isInverse_ ? matching::hasInverseSingleMatch : matching::hasSingleMatch;
    }
    else {
        hasMatchImpl_ = isInverse_ ? matching::hasInverseCombinedMatch : matching::hasCombinedMatch;
    }
}

PatternMatcher::~PatternMatcher() = default;

bool PatternMatcher::hasMatch( std::string_view line ) const
{
    return hasMatchImpl_( line, matcher_, evaluator_.get() );
}

MultiRegularExpression::MultiRegularExpression(
    const klogg::vector<RegularExpressionPattern>& patterns )
    : patterns_( patterns )
{
    try {
        hsExpression_ = HsRegularExpression( patterns_ );
        isValid_ = hsExpression_.isValid();
        errorString_ = hsExpression_.errorString();

    } catch ( std::exception& err ) {
        isValid_ = false;
        errorString_ = err.what();
    }
}

std::unique_ptr<MultiPatternMatcher> MultiRegularExpression::createMatcher() const
{
    return std::make_unique<MultiPatternMatcher>( *this );
}

MultiPatternMatcher::MultiPatternMatcher( const MultiRegularExpression& expression )
    : matcher_( expression.hsExpression_.createMatcher() )
    , patterns_( expression.patterns_ )
{
}

MultiPatternMatcher::~MultiPatternMatcher() = default;

klogg::vector<std::pair<RegularExpressionPattern, bool>>
MultiPatternMatcher::match( std::string_view line ) const
{
    const auto result
        = std::visit( [ &line ]( const auto& m ) { return m.match( line ); }, matcher_ );

    klogg::vector<std::pair<RegularExpressionPattern, bool>> matchedPatterns;
    for ( size_t i = 0u; i < result.size(); ++i ) {
        matchedPatterns.emplace_back( patterns_[ i ], result[ i ] );
    }

    return matchedPatterns;
}
