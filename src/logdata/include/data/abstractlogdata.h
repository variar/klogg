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

#ifndef ABSTRACTLOGDATA_H
#define ABSTRACTLOGDATA_H

#include <QObject>
#include <QString>
#include <QStringList>

#include "linetypes.h"

// Base class representing a set of data.
// It can be either a full set or a filtered set.
class AbstractLogData : public QObject {
  Q_OBJECT

  public:
    // Returns the line passed as a QString
    QString getLineString( LineNumber line ) const;
    // Returns the line passed as a QString, with tabs expanded
    QString getExpandedLineString( LineNumber line ) const;
    // Returns a set of lines as a QStringList
    std::vector<QString> getLines( LineNumber first_line, LinesCount number ) const;
    // Returns a set of lines with tabs expanded
    std::vector<QString> getExpandedLines( LineNumber first_line, LinesCount number ) const;
    // Returns the total number of lines
    LinesCount getNbLine() const;
    // Returns the visible length of the longest line
    // Tabs are expanded
    LineLength getMaxLength() const;
    // Returns the visible length of the passed line
    // Tabs are expanded
    LineLength getLineLength( LineNumber line ) const;

    // Set the view to use the passed encoding for display
    void setDisplayEncoding( const char* encoding_name );
    // Configure how the view shall interpret newline characters
    // this should be non zero for encodings where \n is encoded
    // in multiple bytes (e.g. UTF-16)
    void setMultibyteEncodingOffsets( int before_cr, int after_cr );

    QTextCodec* getDisplayEncoding() const;

    void attachReader() const;
    void detachReader() const;

    // Length of a tab stop
    static const int tabStop = 8;

    static inline LineLength getUntabifiedLength( const QString& line ) {
        int total_spaces = 0;

        const auto dataLength = line.length() * 2;
        auto tab = reinterpret_cast<const char*>( std::memchr( line.utf16(), '\t', dataLength  ) );
        while ( tab != nullptr ) {

            const auto tabPosition = tab -  reinterpret_cast<const char*>(line.utf16());
            const auto spaces = tabStop - ( ( tabPosition + total_spaces ) % tabStop );
            total_spaces += spaces - 1;

            tab++;
            tab = reinterpret_cast<const char*>( std::memchr( tab, '\t', dataLength - tabPosition ) );
        }

        return LineLength( line.length() + total_spaces );
    }

    // The "type" of a line, which will appear in the FilteredView
    enum class LineTypeFlags
    {
        Plain = 0,      // 0 can be checked like a proper flag in QFlags
        Match = 1 << 0,
        Mark  = 1 << 1,
    };
    Q_DECLARE_FLAGS( LineType, LineTypeFlags )

  protected:
    // Internal function called to get a given line
    virtual QString doGetLineString( LineNumber line ) const = 0;
    // Internal function called to get a given line
    virtual QString doGetExpandedLineString( LineNumber line ) const = 0;
    // Internal function called to get a set of lines
    virtual std::vector<QString> doGetLines( LineNumber first_line, LinesCount number ) const = 0;
    // Internal function called to get a set of expanded lines
    virtual std::vector<QString> doGetExpandedLines( LineNumber first_line, LinesCount number ) const = 0;
    // Internal function called to get the number of lines
    virtual LinesCount doGetNbLine() const = 0;
    // Internal function called to get the maximum length
    virtual LineLength doGetMaxLength() const = 0;
    // Internal function called to get the line length
    virtual LineLength doGetLineLength( LineNumber line ) const = 0;
    // Internal function called to set the encoding
    virtual void doSetDisplayEncoding( const char* encoding ) = 0;
    virtual QTextCodec* doGetDisplayEncoding() const = 0;

    virtual void doAttachReader() const = 0;
    virtual void doDetachReader() const = 0;

    static QString untabify( const QString& line ) {
        QString untabified_line;
        int total_spaces = 0;
        untabified_line.reserve(line.size());

        for ( int j = 0; j < line.length(); j++ ) {
            if ( line[j] == QChar::Tabulation ) {
                int spaces = tabStop - ( ( j + total_spaces ) % tabStop );
                // LOG(logDEBUG4) << "Replacing tab at char " << j << " (" << spaces << " spaces)";
                QString blanks( spaces, QChar::Space );
                untabified_line.append( blanks );
                total_spaces += spaces - 1;
            }
            else if ( line[j] == QChar::Null ) {
                untabified_line.append( QChar::Space );
            }
            else {
                untabified_line.append( line[j] );
            }
        }

        return untabified_line;
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS( AbstractLogData::LineType )

#endif
