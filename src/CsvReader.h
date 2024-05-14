/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/SmingHub/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 *
 * CsvReader.h
 *
 * @author: 2021 - Mikee47 <mike@sillyhouse.net>
 *
 ****/

#pragma once

#include <Data/Stream/DataSourceStream.h>
#include <Data/CStringArray.h>
#include <memory>

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
class CsvReader
{
public:
	/**
	 * @brief Construct a CSV reader
	 * @param source Stream to read CSV text from. Must support random seeking.
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
		return readRow();
	}

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

	/**
	 * @brief Set reader to previously noted position
	 * @param cursor Value obtained via `tell()`
	 * @retval bool true on success, false on failure or end of records
	 *
	 * If cursor is BOF then there will be no current record until `next()` is called.
	 * This is the same as if `next()` were called.
	 *
	 * Otherwise the corresponding row will be available via `getRow()`.
	 */
	bool seek(int cursor);

private:
	bool readRow();

	static constexpr int BOF{-1}; ///< Indicates 'Before First Record'

	std::unique_ptr<IDataSourceStream> source;
	size_t maxLineLength;
	CStringArray headings;
	CStringArray row;
	unsigned start{0}; ///< Stream position of first record
	int cursor{BOF};   ///< Stream position for start of current row
	char fieldSeparator;
};
