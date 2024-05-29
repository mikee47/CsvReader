/**
 * CsvParser.h
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

#include <Delegate.h>
#include <Data/CStringArray.h>
#include <Data/Stream/DataSourceStream.h>

/**
 * @brief Class to parse a CSV file
 *
 * Spec: https://www.ietf.org/rfc/rfc4180.txt
 *
 * 1. Each record is located on a separate line
 * 2. Line ending for last record in the file is optional
 * 3. Field headings are provided either in the source data or in constructor (but not both)
 * 4. Fields separated with ',' and whitespace considered part of field content
 * 5. Fields may or may not be quoted - if present, will be removed during parsing
 * 6. Fields may contain line breaks, quotes or commas
 * 7. Quotes may be escaped thus "" if field itself is quoted
 *
 * Additional features:
 *
 * - Line breaks can be \n or \r\n
 * - Escapes codes within fields will be converted: \n \r \t \", \\
 * - Field separator can be changed in constructor
 */
class CsvParser
{
public:
	/**
	 * @brief Callback invoked after a new row has been read
	 * @param parser
	 * @param row
	 * @retval bool Return true to continue parsing, false to stop
	 */
	using RowCallback = Delegate<bool(const CsvParser& parser, const CStringArray& row)>;

	/**
	 * @brief Construct a CSV parser
	 * @param fieldSeparator Single character such as ',', '\t'
	 * 	or '\0' for whitespace-separated fields with leading/trailing whitespace discarded
	 * @param headings Required if source data does not contain field headings as first row
	 * @param maxLineLength Limit size of buffer to guard against malformed data
	 */
	CsvParser(RowCallback callback, char fieldSeparator = ',', const CStringArray& headings = nullptr,
			  size_t maxLineLength = 2048)
		: rowCallback(callback), maxLineLength(maxLineLength), headings(headings), fieldSeparator(fieldSeparator)
	{
	}

	bool parse(Stream& source, bool isFinished);

	bool parse(const char* data, size_t length);

	/**
	 * @brief Reset parser to initial conditions
	 *
	 * Call this method if re-parsing the same data source.
	 * Cursor is set to 'before start'.
	 * Headings are preserved.
	 */
	void reset();

	/**
	 * @brief Get number of columns
	 */
	unsigned count() const
	{
		return headings.count();
	}

	/**
	 * @brief Get a value from the current row
	 * @param index Column index, starts at 0
	 * @retval const char* nullptr if index is not valid
	 */
	const char* getValue(unsigned index) const
	{
		return row[index];
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
	 * @brief Get headings
	 */
	const CStringArray& getHeadings() const
	{
		return headings;
	}

	/**
	 * @brief Get current row
	 */
	const CStringArray& getRow() const
	{
		return row;
	}

	/**
	 * @brief Get cursor position for current row
	 *
	 * The returned value indicates source stream offset for start of current row.
	 * After construction cursor is set to -1. This indicates 'Before first record' (BOF).
	 */
	int tell() const
	{
		return cursor;
	}

protected:
	size_t fillBuffer(Stream& source);
	bool parseRow(bool eof);
	bool readRow(IDataSourceStream& source);
	void setHeadings();

	static constexpr int BOF{-1}; ///< Indicates 'Before First Record'

	unsigned start{0}; ///< Stream position of first record
	int cursor{BOF};   ///< Stream position for start of current row
	unsigned sourcePos{0};

private:
	RowCallback rowCallback;

	size_t maxLineLength;
	CStringArray headings;
	CStringArray row;
	String buffer;
	uint16_t tailpos{0};
	uint16_t taillen{0};
	char fieldSeparator;
};
