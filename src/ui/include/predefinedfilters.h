/*
 * Copyright (C) 2009, 2010, 2011, 2013, 2014, 2015 Nicolas Bonnefon
 * and other contributors
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
 * Copyright (C) 2016 -- 2019 Anton Filimonov and other contributors
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

#ifndef PREDEFINEDFILTERS_H_
#define PREDEFINEDFILTERS_H_

#include <QComboBox>
#include <QStandardItemModel>
#include <qwidget.h>

#include "persistable.h"

// Represents collection of filters read from settings file.
class PredefinedFiltersCollection final : public Persistable<PredefinedFiltersCollection> {
  public:
    using Collection = QMap<QString, QString>;
    using CollectionIterator = QMapIterator<QString, QString>;

    static const char* persistableName()
    {
        return "PredefinedFiltersCollectin";
    }

    Collection getSyncedFilters();
    Collection getFilters() const;

    void retrieveFromStorage( QSettings& settings );
    void saveToStorage( QSettings& settings ) const;
    void saveToStorage( const Collection& filters );

  private:
    static constexpr int PredefinedFiltersCollection_VERSION = 1;

    Collection filters;
};

class PredefinedFiltersComboBox final : public QComboBox {
    Q_OBJECT

  public:
    PredefinedFiltersComboBox( QWidget* crawler );

    PredefinedFiltersComboBox( const PredefinedFiltersComboBox& other ) = delete;
    PredefinedFiltersComboBox( PredefinedFiltersComboBox&& other ) noexcept = delete;
    PredefinedFiltersComboBox& operator=( const PredefinedFiltersComboBox& other ) = delete;
    PredefinedFiltersComboBox& operator=( PredefinedFiltersComboBox&& other ) = delete;

    void populatePredefinedFilters();

  private:
    QWidget* crawlerWidget;
    PredefinedFiltersCollection filtersCollection;

    QStandardItemModel* model;

    void setup();

    void setTitle( const QString& title );
    void insertFilters( const PredefinedFiltersCollection::Collection& filters );
    void connectFilters( const PredefinedFiltersCollection::Collection& filters );
};

#endif