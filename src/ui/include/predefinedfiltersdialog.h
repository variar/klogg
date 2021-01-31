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

#ifndef PREDEFINEDFILTERSDIALOG_H_
#define PREDEFINEDFILTERSDIALOG_H_

#include <QDialog>

#include "predefinedfilters.h"
#include "predefinedfiltersdialog.h"
#include "ui_predefinedfiltersdialog.h"

class PredefinedFiltersDialog : public QDialog, public Ui::PredefinedFiltersDialog {
    Q_OBJECT

  public:
    explicit PredefinedFiltersDialog( QWidget* parent = nullptr );

  private slots:
    void addFilter() const;
    void removeFilter() const;
    void exportFilters();
    void importFilters();

    void resolveStandardButton( QAbstractButton* button );

  signals:
    void optionsChanged();

  private:
    void populateFiltersTable() const;
    void saveSettings();
    void readFiltersTable();

  private:
    PredefinedFiltersCollection::Collection filters_;
};

#endif