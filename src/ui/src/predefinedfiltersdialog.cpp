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

#include <iostream>

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

    populateFiltersTable( PredefinedFiltersCollection::getSynced().getFilters());

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

QString PredefinedFiltersDialog::getUniqueGroupName(QString name)
{
    if ( not filtersTreeWidget->findItems(name, Qt::MatchFlag::MatchExactly).empty() ) {
        name += QString("_new");
        name = getUniqueGroupName(name);
    }

    return name;
}

void PredefinedFiltersDialog::populateFiltersTable(
    const PredefinedFiltersCollection::GroupCollection& filters)
{

    filtersTreeWidget->setColumnCount(4);
    filtersTreeWidget->setHeaderLabels( QStringList() << "Group"
                                                      << "Name"
                                                      << "Pattern"
                                                      << "Regex" );

    for ( const auto& group : filters ) {
        QString groupName = getUniqueGroupName(group.name);

        QTreeWidgetItem *item = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList( QString(groupName)));
        item->setFlags( item->flags() | Qt::ItemIsEditable );
        filtersTreeWidget->addTopLevelItem(item);


        for ( const auto& filter : group.filters ) {
            QTreeWidgetItem *row = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr),
                                                                   QStringList( {
                                                                                    QString(""),
                                                                                    filter.name,
                                                                                    filter.pattern
                                                                                }
                                                                                ));
            row->setFlags( row->flags() | Qt::ItemIsEditable );
            item->addChild( row );
            row->setCheckState( 3, filter.useRegex ? Qt::Checked : Qt::Unchecked );
        }
    }
}

void PredefinedFiltersDialog::saveSettings() const
{
    PredefinedFiltersCollection::getSynced().saveToStorage( readFiltersTable() );
}

PredefinedFiltersCollection::GroupCollection PredefinedFiltersDialog::readFiltersTable(std::optional<QSet<QString>> selection) const
{
    PredefinedFiltersCollection::GroupCollection currentFilters;

    for (int i = 0; i < filtersTreeWidget->topLevelItemCount(); ++i ) {
        QTreeWidgetItem* item = filtersTreeWidget->topLevelItem(i);
        QString group = item->text(0);
        if (selection && not selection->contains(group)) {
            continue;
        } else {
            currentFilters.push_back( { group, {} });

            for(int j = 0; j < item->childCount(); ++j) {
                QTreeWidgetItem* child = item->child(j);
                QString name = child->text(1);
                QString value = child->text(2);
                bool useRegex = child->checkState(3) == Qt::Checked ? true : false;

                if ( !name.isEmpty() && !value.isEmpty() ) {
                    currentFilters.back().filters.push_back( { name, value, useRegex } );
                }
            }
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
    QList<QTreeWidgetItem*> selection = filtersTreeWidget->selectedItems();
    QTreeWidgetItem* sel = nullptr;

    if ( not selection.empty() ) {
        for ( auto item: selection ) {
            sel = item;
            if (item->parent() ) {
                sel = item->parent();
            }
            break;
        }
    } else {
        QString filter = newFilter;
        if (not filter.size() ) {
            filter = QString("filter group");
        }
        QString groupName = getUniqueGroupName(filter);
        QTreeWidgetItem *item = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList( QString(groupName)));
        item->setFlags( item->flags() | Qt::ItemIsEditable );
        filtersTreeWidget->addTopLevelItem(item);
        sel = item;
    }
        QTreeWidgetItem *row = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr),
                                                               QStringList( {
                                                                                QString(""),
                                                                                QString("filter name"),
                                                                                QString("filter")
                                                                            }
                                                                            ));
        row->setFlags( row->flags() | Qt::ItemIsEditable );
        sel->addChild( row );
        row->setCheckState( 3, Qt::Unchecked );
}

void PredefinedFiltersDialog::removeFilter()
{
    for ( const auto item : filtersTreeWidget->selectedItems() ) {
        delete item;
    }
}

void PredefinedFiltersDialog::moveFilterUp()
{
        QTreeWidgetItem *item = filtersTreeWidget->currentItem();

        if (not item ) {
            return;
        }

        if ( not item ->isSelected() ) {
            return;
        }

        int index = filtersTreeWidget->currentIndex().row();
        QTreeWidgetItem *s = nullptr;

        if( item->childCount() ) {
            s = filtersTreeWidget->takeTopLevelItem(index);
            filtersTreeWidget->insertTopLevelItem(index - 1 >=0 ? index - 1 : 0, s);
        } else {
            item = item->parent();
            s = item->takeChild(index);
            item->insertChild(index - 1 >=0 ? index - 1 : 0, s);
        }

        s->setSelected(true);
        filtersTreeWidget->setCurrentItem(s);
}

void PredefinedFiltersDialog::moveFilterDown()
{
    QTreeWidgetItem *item = filtersTreeWidget->currentItem();

    if (not item ) {
        return;
    }

    if ( not item ->isSelected() ) {
        return;
    }


    int index = filtersTreeWidget->currentIndex().row();
    QTreeWidgetItem *s = nullptr;

    if( item->childCount() ) {
        s = filtersTreeWidget->takeTopLevelItem(index);
        filtersTreeWidget->insertTopLevelItem(index + 1 <= filtersTreeWidget->topLevelItemCount() ? index + 1 : index, s);
    } else {
        item = item->parent();
        s = item->takeChild(index);
        item->insertChild(index + 1 <= item->childCount() ? index + 1 : index, s);
    }

    s->setSelected(true);
    filtersTreeWidget->setCurrentItem(s);
}

void PredefinedFiltersDialog::importFilters()
{
    const auto files = QFileDialog::getOpenFileNames( this, "Select file to import", "",
                                                    "Predefined filters (*.conf)" );

    if ( not files.size() ) {
        return;
    }

    PredefinedFiltersCollection collection;

    for (QString file: files) {
        LOG_DEBUG << "Loading predefined filters from " << file;
        QSettings settings{ file, QSettings::IniFormat };

        collection.retrieveFromStorage( settings, QFileInfo(file).baseName(), false);
    }

    populateFiltersTable( collection.getFilters());
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
    QList<QTreeWidgetItem*> selection = filtersTreeWidget->selectedItems();
    std::optional<QSet<QString>> selectedGroups = {};

    if ( not selection.empty() ) {
        selectedGroups = QSet<QString>();
        for ( auto item: selection ) {
            auto sel = item;
            if (item->parent() ) {
                sel = item->parent();
            }
            std::cout << sel->text(0).toStdString() << "\n";
            selectedGroups->insert(sel->text(0));
        }
    }

    PredefinedFiltersCollection::GroupCollection filters = readFiltersTable(selectedGroups);

    collection.setFilters( filters );
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
