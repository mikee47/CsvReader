/****
 * Reader.cpp
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

#include "include/CSV/Reader.h"

namespace CSV
{
Reader::Reader(IDataSourceStream* source, char fieldSeparator, const CStringArray& headings, uint16_t maxLineLength)
	: Parser(Options{
		  .lineLength = maxLineLength,
		  .fieldSeparator = fieldSeparator,
	  }),
	  source(source), headings(headings)
{
	if(source && !headings) {
		readRow(*source);
		this->headings = getRow();
		start = getCursor().end;
	}
	cursor = BOF;
}

bool Reader::seek(int cursor)
{
	int curpos = sourcePos;
	Parser::reset();
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
	return readRow(*source);
}

} // namespace CSV
