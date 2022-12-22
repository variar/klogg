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

#include <QList>
#include <QString>

#include "persistable.h"

struct PredefinedFilter {
    QString name;
    QString pattern;
    bool useRegex;
};

struct PredefinedFilterGroup {
    QString name;
    QList<PredefinedFilter> filters;
};

// Represents collection of filters read from settings file.
class PredefinedFiltersCollection final : public Persistable<PredefinedFiltersCollection> {
  public:
    using GroupCollection = QList< PredefinedFilterGroup >;

    static const char* persistableName()
    {
        return "PredefinedFiltersCollection";
    }

    GroupCollection getSyncedFilters();
    GroupCollection getFilters() const;
    void setFilters(const GroupCollection &filters );

    void retrieveFromStorage(QSettings& settings, QString fileName="default", bool clear=true);
    void saveToStorage( QSettings& settings ) const;
    void saveToStorage( const GroupCollection& filters );

  private:
    static constexpr int PredefinedFiltersCollection_VERSION = 2;
    static constexpr int MultiPredefinedFiltersCollection_VERSION = 3;

    GroupCollection filterGroups_;
};

Q_DECLARE_METATYPE(PredefinedFilter)

#endif
