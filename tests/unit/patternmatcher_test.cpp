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

#include <catch2/catch_all.hpp>

#include "regularexpression.h"

SCENARIO( "Pattern matcher in boolean mode", "[patternmatcher]" )
{
    std::string_view matchLine = "\"This\" is matching pattern";

    WHEN( "Using single pattern" )
    {
        RegularExpression expression(
            RegularExpressionPattern( "\"matching\"", false, false, true, true ) );
        const auto matcher = expression.createMatcher();
        REQUIRE( matcher->hasMatch( matchLine ) );
    }

    WHEN( "Using complex pattern" )
    {
        RegularExpression expression(
            RegularExpressionPattern( "\"not_match\" | \"match\"", false, false, true, true ) );
        const auto matcher = expression.createMatcher();
        REQUIRE( matcher->hasMatch( matchLine ) );
    }

    WHEN( "Using complex pattern with ()" )
    {
        RegularExpression expression( RegularExpressionPattern(
            "(\"not_match\" | \"match\") & !(\"pattern\")", false, false, true, false ) );
        const auto matcher = expression.createMatcher();
        REQUIRE_FALSE( matcher->hasMatch( matchLine ) );
    }

    WHEN( "Using pattern with escaped quotes" )
    {
        RegularExpression expression(
            RegularExpressionPattern( "\"\\\"This\\\"\"", false, false, true, false ) );
        const auto matcher = expression.createMatcher();
        REQUIRE( matcher->hasMatch( matchLine ) );
    }

    WHEN( "Using pattern with not matched quotes" )
    {
        RegularExpression expression(
            RegularExpressionPattern( "\"not_match\" | \"match", false, false, true, false ) );

        REQUIRE_FALSE( expression.isValid() );
    }
}
