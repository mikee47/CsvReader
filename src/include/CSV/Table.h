/****
 * Table.h
 *
 * Copyright 2024 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the CsvReader Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ****/

#pragma once

#include <CSV/Reader.h>

namespace CSV
{
/**
 * @brief Base class for interpreting a record (line) in a CSV file
 */
struct Record {
	CStringArray row;

	Record()
	{
	}

	Record(const CStringArray& row) : row(row)
	{
	}

	explicit operator bool() const
	{
		return row;
	}

	const char* operator[](unsigned index) const
	{
		return row[index];
	}
};

/**
 * @brief Class template for accessing CSV file as set of records
 * @tparam Record Class inherited from Record
 */
template <class Record = Record> class Table : public Reader
{
public:
	class Iterator
	{
	public:
		static constexpr unsigned end{UINT32_MAX};

		Iterator(Reader* reader, unsigned index) : reader(reader), index(index)
		{
		}

		Iterator& operator++()
		{
			if(reader && reader->next()) {
				++index;
			} else {
				index = end;
			}
			return *this;
		}

		Record operator*() const
		{
			return (reader && index != end) ? Record(reader->getRow()) : Record();
		}

		bool operator==(const Iterator& other) const
		{
			return reader == other.reader && index == other.index;
		}

		bool operator!=(const Iterator& other) const
		{
			return !operator==(other);
		}

	private:
		Reader* reader;
		unsigned index;
	};

	using Reader::Reader;

	/**
	 * @brief Fetch next record
	 */
	Record next()
	{
		return Reader::next() ? getRow() : Record();
	}

	Iterator begin()
	{
		reset();
		return Table::next() ? Iterator(this, 0) : end();
	}

	Iterator end()
	{
		return Iterator(this, Iterator::end);
	}
};

} // namespace CSV
