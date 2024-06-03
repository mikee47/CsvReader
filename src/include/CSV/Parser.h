/****
 * Parser.h
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

namespace CSV
{
/**
 * @brief Contains location details of the current record in the source stream
 */
struct Cursor {
	int start;	///< BOF if there is no current record
	unsigned end; ///< One-past end of record

	/**
	 * @brief Get number of source characters in record data
	 */
	size_t length() const
	{
		return (start < 0) ? 0 : end - unsigned(start);
	}

	/**
	 * @brief Convenience operator for debugging, etc.
	 */
	operator String() const
	{
		String s;
		s += '[';
		s += start;
		s += ',';
		s += length();
		s += ']';
		return s;
	}
};

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
class Parser
{
public:
	struct Options {
		/**
		 * Optional list of characters matching start of comment line
		 */
		const char* commentChars = nullptr;
		/**
		 * Maximum number of characters in line, including any escapes
		 */
		uint16_t lineLength = 256;
		/**
		 * Single character such as ',', '\t'
		 * or '\0' for whitespace-separated fields with leading/trailing whitespace discarded
		 */
		char fieldSeparator = ',';
		/**
		 * @brief Set to true to return comment lines, otherwise they're discarded
		 */
		bool wantComments = false;
	};

	/**
	 * @brief Construct a CSV parser
	 * @param options
	 */
	Parser(const Options& options) : options(options)
	{
	}

	/**
	 * @brief Read a single data row, taking data if required from provided Stream
	 * @param source
	 * @retval bool true if record available, false otherwise
	 * @note Call `flush()` after all data pushed
	 */
	bool push(Stream& source);

	/**
	 * @brief Read a single data row, taking data if required from provided buffer
	 * @param data Buffer containing data to read
	 * @param length Number of characters in data
	 * @param offset Read offset in buffer, updated on return
	 * @retval bool true if record available, false otherwise.
	 * @note Call `flush()` after all data pushed
	 */
	bool push(const char* data, size_t length, size_t& offset);

	/**
	 * @brief Call to read additional rows after all data pushed
	 * @retval bool true if record available, false otherwise.
	 * @note Call repeatedly until returns false
	 */
	bool flush();

	/**
	 * @brief Read a single data row using data from provided DataSourceStream
	 * @retval bool false when there are no more rows
	 * @note Returns false only on error or when source.isFinished() returns true.
	 */
	bool readRow(IDataSourceStream& source);

	/**
	 * @brief Reset parser to initial conditions
	 *
	 * Call this method if re-parsing the same data source.
	 * Cursor is set to 'before start'.
	 * Headings are preserved.
	 */
	void reset();

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
		return cursor.start;
	}

	/**
	 * @brief Get cursor position for current row
	 */
	const Cursor& getCursor() const
	{
		return cursor;
	}

protected:
	size_t fillBuffer(Stream& source);
	bool parseRow(bool eof);

	static constexpr int BOF{-1}; ///< Indicates 'Before First Record'

	Options options;
	unsigned start{0};	 ///< Stream position of first record
	Cursor cursor{BOF};	///< Stream position for start of current row
	unsigned sourcePos{0}; ///< Stream position for (one-past) end of current row

private:
	CStringArray row;
	String buffer;
	uint16_t tailpos{0};
	uint16_t taillen{0};
};

} // namespace CSV
