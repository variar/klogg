/*
 * Copyright (C) 2020 Anton Filimonov and other contributors
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

#ifndef KLOGG_DISPLAYFILEPATH_H
#define KLOGG_DISPLAYFILEPATH_H

#include <QString>

class DisplayFilePath {
  public:
    explicit DisplayFilePath( const QString& fullPath );

    QString fullPath() const;
    QString nativeFullPath() const;
    QString displayName() const;

  private:
    QString fullPath_;
    QString nativeFullPath_;
    QString displayName_;
};

class FullPathComparator {
  public:
    explicit FullPathComparator( const QString& path );
    bool operator()( const DisplayFilePath& f ) const;

  private:
    QString fullPath_;
};

#endif // KLOGG_DISPLAYFILEPATH_H
