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
#ifndef KLOGG_DECOMPRESSOR_H
#define KLOGG_DECOMPRESSOR_H

#include <QFile>
#include <QFuture>
#include <QFutureWatcher>


enum class DecompressAction {None, Extract, Decompress};
class Decompressor : public QObject {
    Q_OBJECT
  public:
    explicit Decompressor( QObject* parent = nullptr );

    bool decompress( const QString& path, QFile* outputFile );
    bool extract( const QString& archiveFilePath, const QString& destination );


    static DecompressAction action(const QString& archiveFilePath );

  signals:
    void finished( bool );

  private:
    QFuture<bool> future_;
    QFutureWatcher<bool> watcher_;
};

#endif // KLOGG_DECOMPRESSOR_H
