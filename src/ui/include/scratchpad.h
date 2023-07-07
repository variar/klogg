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

#ifndef SCRATCHPAD_H
#define SCRATCHPAD_H

#include <QWidget>

#include <functional>
#include <qobjectdefs.h>

class QPlainTextEdit;
class QStatusBar;
class QLineEdit;

namespace klogg {
class DateTimeBox;
} // namespace klogg

class ScratchPad : public QWidget {
    Q_OBJECT
  public:
    explicit ScratchPad( QWidget* parent = nullptr );

    ~ScratchPad() = default;
    ScratchPad( const ScratchPad& ) = delete;
    ScratchPad& operator=( const ScratchPad& ) = delete;

  public Q_SLOTS:
    void addData( QString data );
    void replaceData( QString data );

  Q_SIGNALS:
    void updateTransformation();

  private Q_SLOTS:
    void crc32Hex();
    void crc32Dec();
    void fileTime();
    void decToHex();
    void hexToDec();

  private:
    void decodeBase64();
    void encodeBase64();

    void decodeHex();
    void encodeHex();

    void formatJson();
    void formatXml();

    void decodeUrl();

    QString transformText( const std::function<QString( QString )>& transform );

    void transformTextInPlace( const std::function<QString( QString )>& transform );

  private:
    QPlainTextEdit* textEdit_;
    QStatusBar* statusBar_;

    QLineEdit* crc32HexBox_;
    QLineEdit* crc32DecBox_;
    QLineEdit* fileTimeBox_;
    QLineEdit* decToHexBox_;
    QLineEdit* hexToDecBox_;
    klogg::DateTimeBox* timeBox_;
};

#endif // SCRATCHPAD_H