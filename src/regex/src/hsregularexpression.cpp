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
#include <iterator>
#include <numeric>
#include <qregularexpression.h>
#include <string_view>

#ifdef KLOGG_HAS_HS
#include "hsregularexpression.h"

#include "cpu_info.h"
#include "log.h"

namespace {

int matchSingleCallback( unsigned int id, unsigned long long from, unsigned long long to,
                         unsigned int flags, void* context )
{
    Q_UNUSED( id );
    Q_UNUSED( from );
    Q_UNUSED( to );
    Q_UNUSED( flags );

    auto* matchContext = static_cast<HsMatcherContext*>( context );

    matchContext->matchingPatterns[ 0 ] = true;
    return 1;
}

int matchMultiCallback( unsigned int id, unsigned long long from, unsigned long long to,
                        unsigned int flags, void* context )
{
    Q_UNUSED( from );
    Q_UNUSED( to );
    Q_UNUSED( flags );

    auto* matchContext = static_cast<HsMatcherContext*>( context );

    matchContext->matchingPatterns[ id ] = true;

    return 0;
}

} // namespace

HsMatcherContext::HsMatcherContext( std::size_t numberOfPatterns )
    : matchingPatterns( numberOfPatterns, 0 )
    , matchingPatternsTemplate_( numberOfPatterns, 0 )
{
}

void HsMatcherContext::reset()
{
    matchingPatterns = matchingPatternsTemplate_;
}

HsMatcher::HsMatcher( HsDatabase db, HsScratch scratch, std::size_t numberOfPatterns )
    : database_{ std::move( db ) }
    , scratch_{ std::move( scratch ) }
    , context_( numberOfPatterns )
{
}

HsSingleMatcher::HsSingleMatcher( HsDatabase db, HsScratch scratch )
    : HsMatcher( db, std::move( scratch ), 1 )
{
}

MatchedPatterns HsSingleMatcher::match( const std::string_view& utf8Data ) const
{
    context_.reset();

    hs_scan( database_.get(), utf8Data.data(), static_cast<unsigned int>( utf8Data.size() ), 0,
             scratch_.get(), matchSingleCallback, static_cast<void*>( &context_ ) );

    return std::move( context_.matchingPatterns );
}

HsMultiMatcher::HsMultiMatcher( HsDatabase db, HsScratch scratch, std::size_t numberOfPatterns )
    : HsMatcher( db, std::move( scratch ), numberOfPatterns )
{
}

MatchedPatterns HsMultiMatcher::match( const std::string_view& utf8Data ) const
{
    context_.reset();

    hs_scan( database_.get(), utf8Data.data(), static_cast<unsigned int>( utf8Data.size() ), 0,
             scratch_.get(), matchMultiCallback, static_cast<void*>( &context_ ) );

    return std::move( context_.matchingPatterns );
}

MatchedPatterns HsNoopMatcher::match( const std::string_view& ) const
{
    return {};
}

HsRegularExpression::HsRegularExpression( const RegularExpressionPattern& pattern )
    : HsRegularExpression( klogg::vector<RegularExpressionPattern>{ pattern } )
{
}

HsRegularExpression::HsRegularExpression( const klogg::vector<RegularExpressionPattern>& patterns )
    : patterns_( patterns )
{
    auto requiredInstructuins = CpuInstructions::SSE2;
    requiredInstructuins |= CpuInstructions::SSSE3;

    if ( hasRequiredInstructions( supportedCpuInstructions(), requiredInstructuins ) ) {
        database_ = HsDatabase{ makeUniqueResource<hs_database_t, hs_free_database>(
            []( const klogg::vector<RegularExpressionPattern>& expressions,
                QString& errorMessage ) -> hs_database_t* {
                hs_database_t* db = nullptr;
                hs_compile_error_t* error = nullptr;

                klogg::vector<unsigned> flags( expressions.size() );
                std::transform( expressions.cbegin(), expressions.cend(), flags.begin(),
                                []( const auto& expression ) {
                                    auto expressionFlags
                                        = HS_FLAG_UTF8 | HS_FLAG_UCP | HS_FLAG_SINGLEMATCH;
                                    if ( !expression.isCaseSensitive ) {
                                        expressionFlags |= HS_FLAG_CASELESS;
                                    }
                                    return expressionFlags;
                                } );

                klogg::vector<QByteArray> utf8Patterns( expressions.size() );
                std::transform( expressions.cbegin(), expressions.cend(), utf8Patterns.begin(),
                                []( const auto& expression ) {
                                    auto p = expression.pattern;
                                    if ( expression.isPlainText ) {
                                        p = QRegularExpression::escape( expression.pattern );
                                    }
                                    return p.toUtf8();
                                } );

                klogg::vector<const char*> patternPointers( utf8Patterns.size() );
                std::transform( utf8Patterns.cbegin(), utf8Patterns.cend(), patternPointers.begin(),
                                []( const auto& utf8Pattern ) { return utf8Pattern.data(); } );

                klogg::vector<unsigned> expressionIds( expressions.size() );
                std::iota( expressionIds.begin(), expressionIds.end(), 0u );

                const auto compileResult
                    = hs_compile_multi( patternPointers.data(), flags.data(), expressionIds.data(),
                                        static_cast<unsigned>( expressions.size() ), HS_MODE_BLOCK,
                                        nullptr, &db, &error );

                if ( compileResult != HS_SUCCESS ) {
                    LOG_ERROR << "Failed to compile pattern " << error->message;
                    errorMessage = error->message;
                    hs_free_compile_error( error );
                    return nullptr;
                }

                return db;
            },
            patterns, errorMessage_ ) };
    }
    else {
        LOG_WARNING << "Cpu doesn't have sse2 or ssse3, use qt regex engine";
    }

    if ( database_ ) {
        scratch_ = makeUniqueResource<hs_scratch_t, hs_free_scratch>(
            []( hs_database_t* db ) -> hs_scratch_t* {
                hs_scratch_t* scratch = nullptr;

                const auto scratchResult = hs_alloc_scratch( db, &scratch );
                if ( scratchResult != HS_SUCCESS ) {
                    LOG_ERROR << "Failed to allocate scratch";
                    return nullptr;
                }

                return scratch;
            },
            database_.get() );
    }

    if ( !isHsValid() ) {
        for ( const auto& pattern : patterns_ ) {
            const auto regex = static_cast<QRegularExpression>( pattern );
            if ( !regex.isValid() ) {
                isValid_ = false;
                errorMessage_ = regex.errorString();
                break;
            }
        }
    }

    LOG_INFO << "Finished creating pattern database, patterns: " << patterns_.size()
             << ", is db valid: " << isValid_;
}

bool HsRegularExpression::isValid() const
{
    return isValid_;
}

bool HsRegularExpression::isHsValid() const
{
    return database_ != nullptr && scratch_ != nullptr;
}

QString HsRegularExpression::errorString() const
{
    return errorMessage_;
}

MatcherVariant HsRegularExpression::createMatcher() const
{
    if ( !isHsValid() ) {
        return MatcherVariant{ DefaultRegularExpressionMatcher( patterns_ ) };
    }

    auto matcherScratch = makeUniqueResource<hs_scratch_t, hs_free_scratch>(
        []( hs_scratch_t* prototype ) -> hs_scratch_t* {
            hs_scratch_t* scratch = nullptr;

            const auto err = hs_clone_scratch( prototype, &scratch );
            if ( err != HS_SUCCESS ) {
                LOG_ERROR << "hs_clone_scratch failed";
                return nullptr;
            }

            return scratch;
        },
        scratch_.get() );

    if ( !database_ || !scratch_ ) {
        return HsNoopMatcher();
    }
    else if ( patterns_.size() == 1 ) {
        return HsSingleMatcher{ database_, std::move( matcherScratch ) };
    }
    else {
        return HsMultiMatcher{ database_, std::move( matcherScratch ), patterns_.size() };
    }
}
#endif
