/*
 * Copyright (C) 2011 Nicolas Bonnefon and other contributors
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

#ifndef MARKS_H
#define MARKS_H

#include <QChar>
#include <QList>

#include <vector>
#include "linetypes.h"

// Class encapsulating a single mark
// Contains the line number the mark is identifying.
class Mark {
  public:
    Mark( LineNumber line ) : lineNumber_{line} {}

    // Accessors
    LineNumber lineNumber() const { return lineNumber_; }

    bool operator <( const Mark& other ) const
    { return lineNumber_ < other.lineNumber_; }
  private:
    LineNumber lineNumber_;
};

// A list of marks, i.e. line numbers optionally associated to an
// identifying character.
class Marks {
  public:
    // Create an empty Marks
    Marks();

    // Add a mark at the given line, optionally identified by the given char
    // If a mark for this char already exist, the previous one is replaced.
    // It will happily add marks anywhere, even at stupid indexes.
    void addMark( LineNumber line, QChar mark = QChar() );
    // Get the (unique) mark identified by the passed char.
    LineNumber getMark( QChar mark ) const;
    // Returns wheither the passed line has a mark on it.
    bool isLineMarked( LineNumber line ) const;
    // Delete the mark identified by the passed char.
    void deleteMark( QChar mark );
    // Delete the mark present on the passed line or do nothing if there is
    // none.
    void deleteMark( LineNumber line );
    // Get the line marked identified by the index (in this list) passed.
    LineNumber getLineMarkedByIndex( int index ) const
    { return marks_[index].lineNumber(); }
    // Return the total number of marks
    unsigned size() const
    { return static_cast<unsigned>( marks_.size() ); }
    // Completely clear the marks list.
    void clear();

    // Iterator
    // Provide a const_iterator for the client to iterate through the marks.
    class const_iterator
    {
      public:
        using iterator_category = std::vector<Mark>::const_iterator::iterator_category;
        using value_type = std::vector<Mark>::const_iterator::value_type;
        using difference_type = std::vector<Mark>::const_iterator::difference_type;
        using pointer = std::vector<Mark>::const_iterator::pointer;
        using reference = std::vector<Mark>::const_iterator::reference;

        const_iterator( std::vector<Mark>::const_iterator iter )
        { internal_iter_ = iter; }

        const Mark& operator*() const
        { return *internal_iter_; }

        const Mark* operator->() const
        { return &(*internal_iter_); }

        bool operator!=( const const_iterator& other ) const
        { return ( internal_iter_ != other.internal_iter_ ); }

        const_iterator& operator++()
        { ++internal_iter_ ; return *this; }
        const_iterator& operator--()
        { --internal_iter_ ; return *this; }

        const_iterator::difference_type operator-( const const_iterator& other ) const
        { return ( internal_iter_ - other.internal_iter_ ); }

        const_iterator operator+(const_iterator::difference_type n) const
        {
            return const_iterator(internal_iter_ + n);
        }
        const_iterator& operator+=(const_iterator::difference_type n)
        {
            internal_iter_+=n ; return *this;
        }

        const_iterator operator-(const_iterator::difference_type n) const
        {
            return const_iterator(internal_iter_ - n);
        }
        const_iterator& operator-=(const_iterator::difference_type n)
        {
            internal_iter_-=n ; return *this;
        }

		bool operator<(const const_iterator& other) const
		{
			return internal_iter_ < other.internal_iter_;
		}

      private:
        std::vector<Mark>::const_iterator internal_iter_;
    };

    const_iterator begin() const
    { return const_iterator( marks_.begin() ); }
    const_iterator end() const
    { return const_iterator( marks_.end() ); }

  private:
    // List of marks.
    std::vector<Mark> marks_;
};

#endif
