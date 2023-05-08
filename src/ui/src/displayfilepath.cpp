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

#include "displayfilepath.h"

#include <QDir>

constexpr const int MaxPathLength = 128;
namespace {
// inspired by http://chadkuehn.com/shrink-file-paths-with-an-ellipsis-in-c/
template <int limit>
QString shrinkPath( const QString& fullPath )
{
    if ( fullPath.isEmpty() ) {
        return fullPath;
    }

    constexpr const auto delimiter = "…";
    constexpr auto delimiterLen =
        static_cast<qsizetype>( std::char_traits<char>::length( delimiter ) );

    const auto fileInfo = QFileInfo( fullPath );
    const auto fileName = fileInfo.fileName();
    const auto absoluteNativePath = QDir::toNativeSeparators( fileInfo.absolutePath() );

    const auto idealMinLength = fileName.length() + delimiterLen;

    // less than the minimum amt
    if constexpr ( limit < ( ( 2 * delimiterLen ) + 1 ) ) {
        return "";
    }

    // fullpath
    if ( limit >= fullPath.length() ) {
        return QDir::toNativeSeparators( fullPath );
    }

    // file name condensing
    if ( limit < idealMinLength ) {
        constexpr auto n = ( limit - ( 2 * delimiterLen ) );
        return delimiter + fileName.mid( 0, n ) + delimiter;
    }

    // whole name only, no folder structure shown
    if ( limit == idealMinLength ) {
        return delimiter + fileName;
    }

    return absoluteNativePath.mid( 0, ( static_cast<qsizetype>( limit ) - ( idealMinLength + 1 ) ) ) + delimiter
           + QDir::separator() + fileName;
}

} // namespace

DisplayFilePath::DisplayFilePath( const QString& fullPath )
    : fullPath_( fullPath )
    , nativeFullPath_( QDir::toNativeSeparators( fullPath ) )
    , displayName_( shrinkPath<MaxPathLength>( fullPath ) )
{
}

QString DisplayFilePath::fullPath() const
{
    return fullPath_;
}

QString DisplayFilePath::nativeFullPath() const
{
    return nativeFullPath_;
}

QString DisplayFilePath::displayName() const
{
    return displayName_;
}

FullPathComparator::FullPathComparator( const QString& path )
    : fullPath_( path )
{
}

bool FullPathComparator::operator()( const DisplayFilePath& f ) const
{
    return f.fullPath() == fullPath_;
}

FullPathNativeComparator::FullPathNativeComparator( const QString& path )
    : fullPathNative_( path )
{
}

bool FullPathNativeComparator::operator()( const DisplayFilePath& f ) const
{
    return f.nativeFullPath() == fullPathNative_;
}