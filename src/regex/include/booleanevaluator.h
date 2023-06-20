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

#include <exprtk.hpp>
#include <string_view>

#include "containers.h"

#include "regularexpressionpattern.h"

class BooleanExpressionEvaluator {
  public:
    BooleanExpressionEvaluator( const std::string& expression,
                                const klogg::vector<RegularExpressionPattern>& patterns );

    bool isValid() const
    {
        return isValid_;
    }

    std::string errorString() const
    {
        return errorString_;
    }

    bool evaluate( std::string_view variables );

  private:
    bool isValid_ = true;
    std::string errorString_;

    

    exprtk::symbol_table<double> symbols_;
    exprtk::expression<double> expression_;
    exprtk::parser<double> parser_;

    klogg::vector<double*> variables_;

    double precomputedResults_[16];
};