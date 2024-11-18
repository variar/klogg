
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

#ifndef KLOGG_FONTUTILS
#define KLOGG_FONTUTILS

#include "log.h"
#include <numeric>

#include <QFontDatabase>
#include <qfontdatabase.h>
#include <vector>

class FontUtils {
  public:
    static QStringList availableFonts()
    {
        // We only show the fixed fonts
        QStringList fixedFamilies;

#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
        QFontDatabase database;
        const auto families = database.families();
        for ( const auto& family : families ) {
            if ( database.isFixedPitch( family ) )
                fixedFamilies << family;
        }
#else
        const auto families = QFontDatabase::families();
        for ( const auto& family : families ) {
            if ( QFontDatabase::isFixedPitch( family ) )
                fixedFamilies << family;
        }
#endif

        return fixedFamilies;
    }

    static QList<int> availableFontSizes( const QString& family )
    {
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
        auto sizes = QFontDatabase().pointSizes( family, "" );
#else
        auto sizes = QFontDatabase::pointSizes( family, "" );
#endif

        if ( sizes.empty() ) {
            sizes = QFontDatabase::standardSizes();
        }

        std::vector<int> additionalSizes( 10 );
        std::iota( additionalSizes.begin(), additionalSizes.end(), 10 );
        for ( int size : additionalSizes ) {
            if ( !sizes.contains( size ) ) {
                sizes.append( size );
            }
        }

        std::sort( sizes.begin(), sizes.end() );
        return sizes;
    }
};

#endif