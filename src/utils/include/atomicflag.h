/*
 * Copyright (C) 2009, 2010, 2013, 2014, 2015 Nicolas Bonnefon and other
 * contributors
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

#ifndef KLOGG_ATOMICFLAG_H
#define KLOGG_ATOMICFLAG_H

#include <QAtomicInt>

class AtomicFlag {
  public:
    explicit AtomicFlag( bool initialState = false )
    {
        flag_.storeRelease( initialState ? 1 : 0 );
    }

    inline void set()
    {
        flag_.storeRelease( 1 );
    }

    inline void clear()
    {
        flag_.storeRelease( 0 );
    }

    inline explicit operator bool() const
    {
        return flag_.loadAcquire() != 0;
    }

    inline bool operator!() const
    {
        return !flag_.loadAcquire();
    }

    operator const QAtomicInt&() const
    {
        return flag_;
    }

  private:
    QAtomicInt flag_;
};

#endif // ATOMICFLAG_H
