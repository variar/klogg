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

#include "predefinedfilterscombobox.h"

#include <QStandardItemModel>

#include "log.h"

PredefinedFiltersComboBox::PredefinedFiltersComboBox( QWidget* parent )
    : QComboBox( parent )
    , model_( new QStandardItemModel() )
{
    setFocusPolicy( Qt::ClickFocus );
    populatePredefinedFilters();
}

void PredefinedFiltersComboBox::populatePredefinedFilters()
{
    model_->clear();
    const auto filters = filtersCollection_.getSyncedFilters();

    setTitle( "Predefined filters" );

    insertFilters( filters );

    this->setModel( model_ );

    connect( model_, &QStandardItemModel::itemChanged,
             [ this ]( const QStandardItem* changedItem ) {
                 Q_UNUSED( changedItem );
                 collectFilters();
             } );
}

void PredefinedFiltersComboBox::setTitle( const QString& title )
{
    auto* titleItem = new QStandardItem( title );
    model_->insertRow( 0, titleItem );
}

void PredefinedFiltersComboBox::insertFilters(
    const PredefinedFiltersCollection::Collection& filters )
{
    auto row = model_->rowCount();

    for ( const auto& filter : filters ) {
        auto* item = new QStandardItem( filter.first );

        item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
        item->setData( Qt::Unchecked, Qt::CheckStateRole );
        item->setData( filter.second, Qt::UserRole );

        model_->insertRow( row, item );

        row++;
    }
}

void PredefinedFiltersComboBox::collectFilters()
{
    const auto totalRows = model_->rowCount();

    /* If multiple filters are selected connect those with "|" */

    QString fullFilter;
    for ( auto filterIndex = 0; filterIndex < totalRows; ++filterIndex ) {
        const auto item = model_->item( filterIndex );

        if ( item->checkState() == Qt::Checked ) {
            const auto filter = item->data( Qt::UserRole ).toString();

            if ( fullFilter != "" ) {
                fullFilter += "|";
            }

            fullFilter.append( filter );
        }
    }

    emit filterChanged( fullFilter );
}
