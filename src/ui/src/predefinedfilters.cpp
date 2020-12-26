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
#include "crawlerwidget.h"
#include "log.h"
#include <qnamespace.h>
#include <qstandarditemmodel.h>

void PredefinedFiltersCollection::retrieveFromStorage( QSettings& settings )
{
    LOG( logDEBUG ) << "PredefinedFiltersCollection::retrieveFromStorage";

    if ( settings.contains( "PredefinedFiltersCollection/version" ) ) {
        settings.beginGroup( "PredefinedFiltersCollection" );
        if ( settings.value( "version" ).toInt() <= PredefinedFiltersCollection_VERSION ) {
            filters.clear();

            int size = settings.beginReadArray( "filters" );

            for ( int i = 0; i < size; ++i ) {
                settings.setArrayIndex( i );

                filters.insert( QString( settings.value( "name" ).toString() ),
                                QString( settings.value( "filter" ).toString() ) );
            }
        }
        else {
            LOG( logERROR ) << "Unknown version of PredefinedFiltersCollection, ignoring it...";
        }
        settings.endGroup();
    }
}

QMap<QString, QString> PredefinedFiltersCollection::getFilters() const
{
    return this->filters;
}

QMap<QString, QString> PredefinedFiltersCollection::getSyncedFilters()
{
    return this->getSynced().getFilters();
}

PredefinedFiltersComboBox::PredefinedFiltersComboBox( CrawlerWidget* p )
    : parent( p )
{
    setup();
}

void PredefinedFiltersComboBox::setup()
{
    const auto filters = filtersCollection.getSyncedFilters();

    this->setFocusPolicy( Qt::NoFocus );

    model = new QStandardItemModel();

    setTitle( "Predefined filters" );
    insertFilters( filters );

    this->setModel( model );

    connectFilters( filters );
}

void PredefinedFiltersComboBox::setTitle( const QString& title )
{
    auto* titleItem = new QStandardItem( title );
    titleItem->setFlags( titleItem->flags() & ~Qt::ItemIsSelectable );
    model->insertRow( 0, titleItem );
}

void PredefinedFiltersComboBox::insertFilters( const QMap<QString, QString>& filters )
{
    auto i = model->rowCount();

    QMapIterator<QString, QString> iter( filters );

    while ( iter.hasNext() ) {
        iter.next();
        auto* item = new QStandardItem( iter.key() );

        item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
        item->setData( Qt::Unchecked, Qt::CheckStateRole );

        model->insertRow( i++, item );
    }
}

void PredefinedFiltersComboBox::connectFilters( const QMap<QString, QString>& filters )
{
    connect( model, &QStandardItemModel::itemChanged, [ = ]( const QStandardItem* changed_item ) {
        (void)changed_item;

        const auto size = model->rowCount();

        /* If multiple filters are selected connect those with "|" */

        QString filter;
        parent->setSearchLineEditText( "" );

        for ( auto j = 0; j < size; ++j ) {
            const auto item = model->item( j );

            if ( item->checkState() == Qt::Checked ) {
                const auto new_filter = filters.find( item->text() );

                if ( filters.end() != new_filter ) {
                    auto current_filter = parent->currentSearchLineEditText();

                    if ( current_filter != "" )
                        current_filter += "|";

                    parent->setSearchLineEditText( current_filter + new_filter.value() );
                }
            }
        }
    } );
}
