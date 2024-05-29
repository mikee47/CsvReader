/**
 * Reader.h
 *
 * Copyright 2021 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the Reader Library
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

#include "Parser.h"
#include <memory>

namespace CSV
{
/**
 * @brief Class to read a CSV file
 *
 * This class 
 * @see   See Parser for details
 *
 */
class Reader : private Parser
{
public:
	/**
	 * @brief Construct a CSV reader
	 * @param source Stream to read CSV text from
	 * @param fieldSeparator
	 * @param headings Required if source data does not contain field headings as first row
	 * @param maxLineLength Limit size of buffer to guard against malformed data
	 */
	Reader(IDataSourceStream* source, char fieldSeparator = ',', const CStringArray& headings = nullptr,
		   uint16_t maxLineLength = 2048);

	/**
	 * @brief Reset reader to start of CSV file
	 *
	 * Cursor is set to 'before start'.
	 * Call 'next()' to fetch first record.
	 */
	void reset()
	{
		seek(BOF);
	}

	/**
	 * @brief Seek to next record
	 * @retval bool true on success, false if there are no more records
	 */
	bool next()
	{
		return source ? readRow(*source) : false;
	}

	/**
	 * @brief Get number of columns
	 */
	unsigned count() const
	{
		return headings.count();
	}

	using Parser::getRow;

	/**
	 * @brief Get a value from the current row
	 * @param index Column index, starts at 0
	 * @retval const char* nullptr if index is not valid
	 */
	const char* getValue(unsigned index) const
	{
		return getRow()[index];
	}

	/**
	 * @brief Get a value from the current row
	 * @param index Column name
	 * @retval const char* nullptr if name is not found
	 */
	const char* getValue(const char* name) const
	{
		return getValue(getColumn(name));
	}

	/**
	 * @brief Get index of column given its name
	 * @param name Column name to find
	 * @retval int -1 if name is not found
	 */
	int getColumn(const char* name) const
	{
		return headings.indexOf(name);
	}

	/**
	 * @brief Determine if reader is valid
	 */
	explicit operator bool() const
	{
		return bool(source);
	}

	/**
	 * @brief Get headings
	 */
	const CStringArray& getHeadings() const
	{
		return headings;
	}

	using Parser::tell;

	/**
	 * @brief Set reader to previously noted position
	 * @param cursor Value obtained via `tell()`
	 * @retval bool true on success, false on failure or end of records
	 * @note Source stream must support random seeking (seekFrom)
	 *
	 * If cursor is BOF then there will be no current record until `next()` is called.
	 * This is the same as if `next()` were called.
	 *
	 * Otherwise the corresponding row will be available via `getRow()`.
	 */
	bool seek(int cursor);

	bool seek(Cursor cursor)
	{
		return seek(cursor.start);
	}

private:
	std::unique_ptr<IDataSourceStream> source;
	CStringArray headings;
};

} // namespace CSV
