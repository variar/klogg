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

#include "predefinedfiltersdialog.h"
#include "log.h"
#include "predefinedfilters.h"
#include <qdialogbuttonbox.h>
#include <qwindowdefs.h>

#include <set>

PredefinedFiltersDialog::PredefinedFiltersDialog( QWidget* parent )
    : QDialog( parent )
{
    setupUi( this );

    filters = PredefinedFiltersCollection::getSynced().getFilters();

    populateFiltersTable();

    connect( addFilterButton, &QToolButton::clicked, this, &PredefinedFiltersDialog::addFilter );
    connect( removeFilterButton, &QToolButton::clicked, this,
             &PredefinedFiltersDialog::removeFilter );

    connect( buttonBox, &QDialogButtonBox::clicked, this,
             &PredefinedFiltersDialog::resolveStandardButton );
}

void PredefinedFiltersDialog::populateFiltersTable() const
{
    filtersTableWidget->clear();

    filtersTableWidget->setRowCount( filters.size() );
    filtersTableWidget->setColumnCount( 2 );

    filtersTableWidget->setHorizontalHeaderLabels( QStringList() << "Filter name"
                                                                 << "Pattern" );

    PredefinedFiltersCollection::CollectionIterator iter( filters );
    for ( int i = 0; iter.hasNext(); ++i ) {
        iter.next();
        filtersTableWidget->setItem( i, 0, new QTableWidgetItem( iter.key() ) );
        filtersTableWidget->setItem( i, 1, new QTableWidgetItem( iter.value() ) );
    }

    filtersTableWidget->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::Stretch );
}

void PredefinedFiltersDialog::saveSettings()
{
    const auto rows = filtersTableWidget->rowCount();

    filters.clear();

    for ( auto i = 0; i < rows; ++i ) {
        if ( nullptr == filtersTableWidget->item( i, 0 )
             || nullptr == filtersTableWidget->item( i, 1 ) )
            continue;
        const auto key = filtersTableWidget->item( i, 0 )->text();
        const auto value = filtersTableWidget->item( i, 1 )->text();

        if ( key != "" && value != "" )
            filters.insert( key, value );
    }

    PredefinedFiltersCollection::getSynced().saveToStorage( filters );
}

void PredefinedFiltersDialog::addFilter() const
{
    filtersTableWidget->setRowCount( filtersTableWidget->rowCount() + 1 );
}

void PredefinedFiltersDialog::removeFilter() const
{
    filtersTableWidget->removeRow( filtersTableWidget->currentRow() );
}

void PredefinedFiltersDialog::resolveStandardButton( QAbstractButton* button )
{
    LOG( logDEBUG ) << "PredefinedFiltersDialog::resolveStandardButton";

    const auto role = buttonBox->buttonRole( button );

    if ( role == QDialogButtonBox::RejectRole ) {
        reject();
        return;
    }

    if ( role == QDialogButtonBox::AcceptRole ) {
        saveSettings();
        accept();
    }
    else if ( role == QDialogButtonBox::ApplyRole ) {
        saveSettings();
    }
    else {
        LOG( logERROR ) << "PredefinedFiltersDialog::resolveStandardButton unhandled role: "
                        << role;
        return;
    }

    emit optionsChanged();
}