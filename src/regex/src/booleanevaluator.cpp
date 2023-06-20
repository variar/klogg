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

#include "booleanevaluator.h"

#include <string>

#include "log.h"

namespace {

static constexpr size_t MaxPrecomputedPatterns = 4;
static constexpr size_t PrecomputedCombinations[] = { 0, 2, 4, 8, 16 };

bool isBitSet( unsigned num, unsigned bit )
{
    return 1 == ( ( num >> bit ) & 1 );
}

uint32_t buildPatternCombination( std::string_view variables )
{
    uint32_t combination = 0;
    for ( auto bit = 0u; bit < variables.size(); ++bit ) {
        if ( variables[ bit ] ) {
            combination = combination | ( 1u << bit );
        }
    }

    return combination;
}

} // namespace

BooleanExpressionEvaluator::BooleanExpressionEvaluator(
    const std::string& expression, const klogg::vector<RegularExpressionPattern>& patterns )
{
    variables_.reserve( patterns.size() );

    for ( const auto& p : patterns ) {
        if ( symbols_.create_variable( p.id() ) ) {
            variables_.push_back( &symbols_.get_variable( p.id() )->ref() );
        }
    }

    expression_.register_symbol_table( symbols_ );
    isValid_ = parser_.compile( expression, expression_ );
    if ( !isValid_ && parser_.error_count() > 0 ) {
        auto error = parser_.get_error( 0 );
        exprtk::parser_error::update_error( error, expression );
        errorString_ = error.diagnostic + " at " + std::to_string( error.column_no );
    }
    else if ( patterns.size() <= MaxPrecomputedPatterns ) {
        const auto patternVariants = PrecomputedCombinations[ patterns.size() ];
        for ( auto patternCombination = 0u; patternCombination < patternVariants;
              ++patternCombination ) {
            for ( auto p = 0u; p < patterns.size(); ++p ) {
                *variables_[ p ] = isBitSet( patternCombination, p );
            }
            precomputedResults_[ patternCombination ] = expression_.value();
            LOG_INFO << "precomputed result for " << patternCombination << " is "
                     << ( precomputedResults_[ patternCombination ] > 0 );
        }
    }
}

bool BooleanExpressionEvaluator::evaluate( std::string_view variables )
{
    if ( !isValid() ) {
        return false;
    }

    if ( variables_.size() != variables.size() ) {
        LOG_ERROR << "Wrong number of matched patterns";
        return false;
    }

    if ( variables.size() <= MaxPrecomputedPatterns ) {
        const auto patternCombination = buildPatternCombination( variables );
        return precomputedResults_[ patternCombination ] > 0;
    }

    for ( auto index = 0u; index < variables_.size(); ++index ) {
        *variables_[ index ] = variables[ index ];
    }

    return expression_.value() > 0;
}
