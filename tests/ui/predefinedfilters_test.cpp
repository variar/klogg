/*
 * Copyright (C) 2026 klogg contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <catch2/catch.hpp>

#include <QSettings>
#include <QTemporaryFile>

#include "predefinedfilters.h"

SCENARIO( "Top filter settings are persisted", "[ui]" )
{
    QTemporaryFile file{ "predefined_filters_test_XXXXXX.conf" };
    REQUIRE( file.open() );
    file.close();

    GIVEN( "a saved top filter" )
    {
        QSettings settings{ file.fileName(), QSettings::IniFormat };
        PredefinedFiltersCollection savedCollection;
        savedCollection.setFilters( { { "Noise", "group change option", false, true } } );
        savedCollection.saveToStorage( settings );
        settings.sync();

        WHEN( "the filters are loaded" )
        {
            PredefinedFiltersCollection loadedCollection;
            loadedCollection.retrieveFromStorage( settings );
            const auto filters = loadedCollection.getFilters();

            THEN( "the top filter flag is restored" )
            {
                REQUIRE( filters.size() == 1 );
                REQUIRE( filters.front().filterTop );
            }
        }
    }
}

SCENARIO( "Older predefined filters remain compatible", "[ui]" )
{
    QTemporaryFile file{ "predefined_filters_legacy_test_XXXXXX.conf" };
    REQUIRE( file.open() );
    file.close();

    QSettings settings{ file.fileName(), QSettings::IniFormat };
    settings.beginGroup( "PredefinedFiltersCollection" );
    settings.setValue( "version", 2 );
    settings.beginWriteArray( "filters" );
    settings.setArrayIndex( 0 );
    settings.setValue( "name", "Existing filter" );
    settings.setValue( "filter", "ShipTrail.lua" );
    settings.setValue( "regex", false );
    settings.endArray();
    settings.endGroup();
    settings.sync();

    PredefinedFiltersCollection collection;
    collection.retrieveFromStorage( settings );
    const auto filters = collection.getFilters();

    REQUIRE( filters.size() == 1 );
    REQUIRE_FALSE( filters.front().filterTop );
}
