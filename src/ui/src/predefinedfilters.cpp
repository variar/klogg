/*
 * Copyright (C) 2009, 2010 Nicolas Bonnefon and other contributors
 *
 * This file is part of glogg.
 *
 * glogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * glogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with glogg.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Copyright (C) 2019 Anton Filimonov and other contributors
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

#include "predefinedfilters.h"

#include "log.h"

void PredefinedFiltersCollection::retrieveFromStorage( QSettings& settings, QString fileName, bool clear)
{
    LOG_DEBUG << "PredefinedFiltersCollection::retrieveFromStorage";

    if ( settings.contains( "PredefinedFiltersCollection/version" ) ) {
        settings.beginGroup( "PredefinedFiltersCollection" );
        int fileVersion = settings.value( "version" ).toInt();

        if ( fileVersion <= PredefinedFiltersCollection_VERSION ) {

            if (clear) {
                filterGroups_.clear();
            }

            int size = settings.beginReadArray( "filters" );
            filterGroups_.push_back( { fileName, {} });

            for ( int i = 0; i < size; ++i ) {
                settings.setArrayIndex( i );

                filterGroups_.back().filters.push_back({
                                                           settings.value( "name" ).toString(),
                                                           settings.value( "filter" ).toString(),
                                                           settings.value( "regex", true ).toBool()
                                                        });
            }
            settings.endArray();
        }
        if ( fileVersion == MultiPredefinedFiltersCollection_VERSION ) {
            if (clear) {
                filterGroups_.clear();
            }

            int size = settings.beginReadArray( "filter_groups" );
            QList<QString> filterGroups;

            for ( int i = 0; i < size; ++i ) {
                settings.setArrayIndex( i );
                filterGroups.push_back( settings.value( "name" ).toString() );
            }
            settings.endArray();

            for (const auto group: filterGroups) {
                size = settings.beginReadArray( group );

                filterGroups_.push_back( { group, {} });

                for ( int i = 0; i < size; ++i ) {
                    settings.setArrayIndex( i );
                    filterGroups_.back().filters.push_back({
                                                               settings.value( "name" ).toString(),
                                                               settings.value( "filter" ).toString(),
                                                               settings.value( "regex", true ).toBool()
                                                            });
                }

                settings.endArray();
            }
        }
        else {
            LOG_ERROR << "Unknown version of PredefinedFiltersCollection, ignoring it...";
        }
        settings.endGroup();
    }
}

void PredefinedFiltersCollection::saveToStorage( QSettings& settings ) const
{
    LOG_DEBUG << "PredefinedFiltersCollection::saveToStorage";

    settings.beginGroup( "PredefinedFiltersCollection" );
    settings.setValue( "version", MultiPredefinedFiltersCollection_VERSION );

    settings.remove( "filter_groups" );

    settings.beginWriteArray( "filter_groups" );

    int arrayIndex = 0;
    for ( const auto& group : filterGroups_ ) {
        settings.setArrayIndex( arrayIndex );
        settings.setValue( "name", group.name );
        arrayIndex++;
    }
    settings.endArray();

    for ( const auto& group : filterGroups_ ) {
        settings.remove( group.name );
        settings.beginWriteArray( group.name );
        arrayIndex = 0;
        for ( const auto& filter : group.filters ) {
            settings.setArrayIndex( arrayIndex );
            settings.setValue( "name", filter.name );
            settings.setValue( "filter", filter.pattern );
            settings.setValue( "regex", filter.useRegex );

            arrayIndex++;
        }
        settings.endArray();
    }
    settings.endGroup();
}

void PredefinedFiltersCollection::saveToStorage(const GroupCollection &filters )
{
    filterGroups_ = filters;
    this->save();
}

PredefinedFiltersCollection::GroupCollection PredefinedFiltersCollection::getFilters() const
{
    return filterGroups_;
}

PredefinedFiltersCollection::GroupCollection PredefinedFiltersCollection::getSyncedFilters()
{
    filterGroups_ = this->getSynced().getFilters();
    return filterGroups_;
}

void PredefinedFiltersCollection::setFilters( const GroupCollection& filters )
{
    filterGroups_ = filters;
}
