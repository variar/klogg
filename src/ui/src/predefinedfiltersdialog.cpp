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

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QTimer>
#include <QToolButton>
#include <qabstractitemmodel.h>
#include <qboxlayout.h>
#include <qcheckbox.h>
#include <qglobal.h>
#include <qwidget.h>
#include <tuple>
#include <vector>

#include "dispatch_to.h"
#include "iconloader.h"
#include "log.h"
#include "predefinedfilters.h"

class CenteredCheckbox : public QWidget
{
    public: 
        explicit CenteredCheckbox(QWidget* parent = nullptr) : QWidget(parent)
        {
            auto layout = new QHBoxLayout;
            layout->setAlignment(Qt::AlignCenter);
            checkbox_ = new QCheckBox;
            layout->addWidget(checkbox_);
            this->setLayout(layout);

            QPalette palette = this->palette();
            palette.setColor(QPalette::Base, palette.color(QPalette::Window));
            checkbox_->setPalette(palette);
        }

        bool isChecked() const {
            return checkbox_->isChecked();
        }

        void setChecked(bool isChecked) {
            checkbox_->setChecked(isChecked);
        }

    private:
        QCheckBox* checkbox_;
};

PredefinedFiltersDialog::PredefinedFiltersDialog( QWidget* parent )
    : QDialog( parent )
{
    setupUi( this );

    populateFiltersTable( PredefinedFiltersCollection::getSynced().getFilters() );

    connect( addFilterButton, &QToolButton::clicked, this, &PredefinedFiltersDialog::addFilter );
    connect( removeFilterButton, &QToolButton::clicked, this,
             &PredefinedFiltersDialog::removeFilter );
    connect( upButton, &QToolButton::clicked, this, &PredefinedFiltersDialog::moveFilterUp );
    connect( downButton, &QToolButton::clicked, this, &PredefinedFiltersDialog::moveFilterDown );
    connect( importFilterButton, &QToolButton::clicked, this,
             &PredefinedFiltersDialog::importFilters );
    connect( exportFilterButton, &QToolButton::clicked, this,
             &PredefinedFiltersDialog::exportFilters );

    connect( buttonBox, &QDialogButtonBox::clicked, this,
             &PredefinedFiltersDialog::resolveStandardButton );

    connect( filtersTableWidget, &QTableWidget::currentCellChanged, this,
             &PredefinedFiltersDialog::onCurrentCellChanged );

    dispatchToMainThread( [ this ] {
        IconLoader iconLoader( this );

        addFilterButton->setIcon( iconLoader.load( "icons8-plus-16" ) );
        removeFilterButton->setIcon( iconLoader.load( "icons8-minus-16" ) );
        upButton->setIcon( iconLoader.load( "icons8-up-16" ) );
        downButton->setIcon( iconLoader.load( "icons8-down-arrow-16" ) );
    } );
}

PredefinedFiltersDialog::PredefinedFiltersDialog( const QString& newFilter, QWidget* parent )
    : PredefinedFiltersDialog( parent )
{
    if ( !newFilter.isEmpty() ) {
        addFilterRow( newFilter );
    }
}

void PredefinedFiltersDialog::updateButtons()
{
    const auto filtersCount = filtersTableWidget->rowCount();
    removeFilterButton->setEnabled( filtersCount > 0 );

    updateUpDownButtons( filtersTableWidget->currentRow() );
}

void PredefinedFiltersDialog::onCurrentCellChanged( int currentRow, int currentColumn,
                                                    int previousRow, int previousColumn )
{
    Q_UNUSED( currentColumn )
    Q_UNUSED( previousRow )
    Q_UNUSED( previousColumn )

    updateUpDownButtons( currentRow );
}

void PredefinedFiltersDialog::updateUpDownButtons( int currentRow )
{
    upButton->setEnabled( currentRow > 0 );
    downButton->setEnabled( currentRow < filtersTableWidget->rowCount() - 1 );
}

void PredefinedFiltersDialog::populateFiltersTable(
    const PredefinedFiltersCollection::Collection& filters )
{
    filtersTableWidget->clear();

    filtersTableWidget->setRowCount( static_cast<int>( filters.size() ) );
    filtersTableWidget->setColumnCount( 3 );

    filtersTableWidget->setHorizontalHeaderLabels( QStringList() << "Name"
                                                                 << "Pattern"
                                                                 << "Regex" );

    int filterIndex = 0;
    for ( const auto& filter : filters ) {
        filtersTableWidget->setItem( filterIndex, 0, new QTableWidgetItem( filter.name ) );
        filtersTableWidget->setItem( filterIndex, 1, new QTableWidgetItem( filter.pattern ) );
        auto* regexCheckbox = new CenteredCheckbox;
        regexCheckbox->setChecked( filter.useRegex );
        filtersTableWidget->setCellWidget( filterIndex, 2, regexCheckbox );

        filterIndex++;
    }

    filtersTableWidget->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::Stretch );

    updateButtons();
}

void PredefinedFiltersDialog::saveSettings() const
{
    PredefinedFiltersCollection::getSynced().saveToStorage( readFiltersTable() );
}

PredefinedFiltersCollection::Collection PredefinedFiltersDialog::readFiltersTable() const
{
    const auto rows = filtersTableWidget->rowCount();

    PredefinedFiltersCollection::Collection currentFilters;

    for ( auto i = 0; i < rows; ++i ) {
        if ( nullptr == filtersTableWidget->item( i, 0 )
             || nullptr == filtersTableWidget->item( i, 1 ) ) {
            continue;
        }

        const auto name = filtersTableWidget->item( i, 0 )->text();
        const auto value = filtersTableWidget->item( i, 1 )->text();

        const auto useRegexCheckbox
            = static_cast<CenteredCheckbox*>( filtersTableWidget->cellWidget( i, 2 ) );
        const auto useRegex = useRegexCheckbox ? useRegexCheckbox->isChecked() : false;

        if ( !name.isEmpty() && !value.isEmpty() ) {
            currentFilters.push_back( { name, value, useRegex, i } );
        }
    }

    return currentFilters;
}

void PredefinedFiltersDialog::addFilter()
{
    addFilterRow( {} );
}

void PredefinedFiltersDialog::addFilterRow( const QString& newFilter )
{
    const auto newRow = filtersTableWidget->rowCount();
    filtersTableWidget->setRowCount( newRow + 1 );
    filtersTableWidget->setItem( newRow, 1, new QTableWidgetItem( newFilter ) );
    filtersTableWidget->setItem( newRow, 0, new QTableWidgetItem( "" ) );
    auto regexCheckBox = new CenteredCheckbox;
    filtersTableWidget->setCellWidget( newRow, 2, regexCheckBox );

    filtersTableWidget->scrollToItem( filtersTableWidget->item( newRow, 0 ) );
    filtersTableWidget->setCurrentCell( newRow, 0 );
    filtersTableWidget->editItem( filtersTableWidget->item( newRow, 0 ) );
}

void PredefinedFiltersDialog::removeFilter()
{
    filtersTableWidget->removeRow( filtersTableWidget->currentRow() );

    updateButtons();
}

void PredefinedFiltersDialog::moveFilterUp()
{
    const auto currentRow = filtersTableWidget->currentRow();
    const auto selectedColumn = filtersTableWidget->currentColumn();

    if ( currentRow >= 0 ) {
        swapFilters( currentRow, currentRow - 1, selectedColumn );
    }
}

void PredefinedFiltersDialog::moveFilterDown()
{
    const auto currentRow = filtersTableWidget->currentRow();
    const auto selectedColumn = filtersTableWidget->currentColumn();

    if ( currentRow >= 0 ) {
        swapFilters( currentRow, currentRow + 1, selectedColumn );
    }
}

void PredefinedFiltersDialog::swapFilters( int currentRow, int newRow, int selectedColumn )
{
    dispatchToMainThread( [ this, currentRow, newRow, selectedColumn ] {
        for ( int column = 0; column < filtersTableWidget->columnCount(); ++column ) {
            auto currentUseRegex
                = static_cast<CenteredCheckbox*>( filtersTableWidget->cellWidget( currentRow, column ) );
            auto newUseRegex
                = static_cast<CenteredCheckbox*>( filtersTableWidget->cellWidget( newRow, column ) );

            if ( currentUseRegex && newUseRegex ) {
                const auto currentCheckState = currentUseRegex->isChecked();
                const auto newCheckState = newUseRegex->isChecked();
                currentUseRegex->setChecked( newCheckState );
                newUseRegex->setChecked( currentCheckState );
            }
            else {
                auto currentItem = filtersTableWidget->takeItem( currentRow, column );
                auto newItem = filtersTableWidget->takeItem( newRow, column );

                filtersTableWidget->setItem( newRow, column, currentItem );
                filtersTableWidget->setItem( currentRow, column, newItem );
            }
        }
        filtersTableWidget->setCurrentCell( newRow, selectedColumn );
    } );
}

void PredefinedFiltersDialog::importFilters()
{
    const auto file = QFileDialog::getOpenFileName( this, "Select file to import", "",
                                                    "Predefined filters (*.conf)" );

    if ( file.isEmpty() ) {
        return;
    }

    LOG_DEBUG << "Loading predefined filters from " << file;
    QSettings settings{ file, QSettings::IniFormat };

    PredefinedFiltersCollection collection;
    collection.retrieveFromStorage( settings );
    populateFiltersTable( collection.getFilters() );
}

void PredefinedFiltersDialog::exportFilters()
{
    const auto file = QFileDialog::getSaveFileName( this, "Export predefined filters", "",
                                                    tr( "Predefined filters (*.conf)" ) );

    if ( file.isEmpty() ) {
        return;
    }

    QSettings settings{ file, QSettings::IniFormat };

    PredefinedFiltersCollection collection;
    collection.setFilters( readFiltersTable() );
    collection.saveToStorage( settings );
}

void PredefinedFiltersDialog::resolveStandardButton( QAbstractButton* button )
{
    LOG_DEBUG << "PredefinedFiltersDialog::resolveStandardButton";

    const auto role = buttonBox->buttonRole( button );

    switch ( role ) {
    case QDialogButtonBox::RejectRole:
        reject();
        return;

    case QDialogButtonBox::ApplyRole:
        saveSettings();
        break;

    case QDialogButtonBox::AcceptRole:
        saveSettings();
        accept();
        break;
    default:
        LOG_ERROR << "PredefinedFiltersDialog::resolveStandardButton unhandled role: " << role;
        return;
    }

    Q_EMIT optionsChanged();
}