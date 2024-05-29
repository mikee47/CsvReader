/**
 * CsvReader.h
 *
 * Copyright 2021 mikee47 <mike@sillyhouse.net>
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

#include "CsvParser.h"
#include <memory>

class CsvReader : public CsvParser
{
public:
	/**
	 * @brief Construct a CSV reader
	 * @param source Stream to read CSV text from
	 * @param fieldSeparator
	 * @param headings Required if source data does not contain field headings as first row
	 * @param maxLineLength Limit size of buffer to guard against malformed data
	 */
	CsvReader(IDataSourceStream* source, char fieldSeparator = ',', const CStringArray& headings = nullptr,
			  size_t maxLineLength = 2048);

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
	 * @brief Determine if reader is valid
	 */
	explicit operator bool() const
	{
		return bool(source);
	}

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

private:
	std::unique_ptr<IDataSourceStream> source;
};
