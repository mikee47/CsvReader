/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/SmingHub/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 *
 * CsvReader.cpp
 *
 * @author: 2021 - Mikee47 <mike@sillyhouse.net>
 *
 ****/

#include "CsvReader.h"
#include <debug_progmem.h>

#define DEBUG_READER 0

CsvReader::CsvReader(IDataSourceStream* source, char fieldSeparator, const CStringArray& headings, size_t maxLineLength)
	: CsvParser(nullptr, fieldSeparator, headings, maxLineLength), source(source)
{
	if(source && !getHeadings()) {
		readRow(*source, false);
		setHeadings();
	}
	cursor = BOF;
}

bool CsvReader::seek(int cursor)
{
	int curpos = sourcePos;
	CsvParser::reset();
	if(!source) {
		return false;
	}
	auto newpos = std::max(cursor, int(start));
	if(newpos != curpos) {
		int pos = source->seekFrom(newpos, SeekOrigin::Start);
		if(pos != newpos) {
			return false;
		}
		sourcePos = newpos;
	}
	this->cursor = cursor;
	if(cursor < int(start)) {
		// Before first record has been read
		return true;
	}
	return readRow(*source, source->isFinished());
}
