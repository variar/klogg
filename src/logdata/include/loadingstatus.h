/*
 * Copyright (C) 2014 Nicolas Bonnefon and other contributors
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

#ifndef LOADINGSTATUS_H
#define LOADINGSTATUS_H

#include <QMetaType>

// Loading status of a file
enum class LoadingStatus {
    Successful,
    Interrupted,
    NoMemory
};

// Data status (whether new, not seen, data is available)
enum class DataStatus {
    OLD_DATA,
    NEW_DATA,
    NEW_FILTERED_DATA
};

Q_DECLARE_METATYPE( DataStatus )
Q_DECLARE_METATYPE( LoadingStatus )

#endif
