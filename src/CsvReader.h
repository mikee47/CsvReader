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

#include "CsvParser.h"
#include <Data/Stream/DataSourceStream.h>
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
		return source ? readRow(*source, source->isFinished()) : false;
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
