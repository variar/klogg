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

#include <catch2/catch_session.hpp>

#include <QApplication>

#include <logger.h>

#include "configuration.h"
#include <persistentinfo.h>

const bool PersistentInfo::ForcePortable = true;

int main( int argc, char* argv[] )
{
    QApplication a( argc, argv );

    logging::enableLogging();

    Configuration::getSynced();

    return Catch::Session().run( argc, argv );
}
