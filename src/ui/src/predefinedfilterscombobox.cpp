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

#include <QListView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <qabstractitemview.h>

#include "log.h"

class QCheckListStyledItemDelegate : public QStyledItemDelegate {
  public:
    QCheckListStyledItemDelegate( QObject* parent = 0 )
        : QStyledItemDelegate( parent )
    {
    }

    void paint( QPainter* painter, const QStyleOptionViewItem& option,
                const QModelIndex& index ) const
    {
        QStyleOptionViewItem refToNonConstOption = option;
        refToNonConstOption.showDecorationSelected = false;
        QStyledItemDelegate::paint( painter, refToNonConstOption, index );
    }
};

PredefinedFiltersComboBox::PredefinedFiltersComboBox( QWidget* parent )
    : QComboBox( parent )
    , model_( new QStandardItemModel() )
{
    setFocusPolicy( Qt::ClickFocus );
    setItemDelegate( new QCheckListStyledItemDelegate( this ) );
    populatePredefinedFilters();

    connect( model_, &QStandardItemModel::itemChanged, this,
             [ this ]( const QStandardItem *changedItem ) {
                 collectFilters( *changedItem );
             } );

    const auto changeCheckState = [ this ]( const QModelIndex& index ) {
        auto item = model_->itemFromIndex( index );
        if ( !item || !item->isCheckable() ) {
            return;
        }
        if ( item->checkState() == Qt::Checked ) {
            item->setCheckState( Qt::Unchecked );
        }
        else {
            item->setCheckState( Qt::Checked );
        }
    };

    connect( view(), &QAbstractItemView::pressed, this, changeCheckState );
    connect( view(), &QAbstractItemView::doubleClicked, this, changeCheckState );

    QPalette palette = this->palette();
    palette.setColor( QPalette::Base, palette.color( QPalette::Window ) );
    view()->setPalette( palette );
}

void PredefinedFiltersComboBox::populatePredefinedFilters()
{
    model_->clear();
    const auto filters = filtersCollection_.getSyncedFilters();

    setTitle( "Predefined filters" );

    insertFilters( filters );

    this->setModel( model_ );
}

void PredefinedFiltersComboBox::setTitle( const QString& title )
{
    auto* titleItem = new QStandardItem( title );
    model_->insertRow( 0, titleItem );
}

void PredefinedFiltersComboBox::insertFilters(
    const PredefinedFiltersCollection::Collection& filters )
{
    for ( const auto& filter : filters ) {
        auto* item = new QStandardItem( filter.name );

        item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
        item->setData( Qt::Unchecked, Qt::CheckStateRole );

        item->setData( filter.pattern, PatternRole );
        item->setData( filter.useRegex, RegexRole );

        model_->insertRow( model_->rowCount(), item );
    }
}

void PredefinedFiltersComboBox::collectFilters( const QStandardItem &changedItem )
{
    const auto totalRows = model_->rowCount();

    /* If multiple filters are selected connect those with "|" */

    QList<PredefinedFilter> selectedPatterns;
    selectedPatterns.reserve( totalRows );
    for ( auto filterIndex = 0; filterIndex < totalRows; ++filterIndex ) {
        const auto item = model_->item( filterIndex );

        if ( item->checkState() != Qt::Checked ) {
            continue;
        }

        selectedPatterns.append( { item->text(), item->data( PatternRole ).toString(),
                                   item->data( RegexRole ).toBool(), filterIndex } );
    }

    Q_EMIT filterChanged( selectedPatterns, changedItem );
}
