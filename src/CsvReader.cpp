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

CsvReader::CsvReader(IDataSourceStream* source, char fieldSeparator, const CStringArray& headings, size_t maxLineLength)
	: source(source), maxLineLength(maxLineLength), headings(headings), fieldSeparator(fieldSeparator)
{
	if(source && !headings) {
		readRow();
		this->headings = std::move(row);
		start = source->seekFrom(0, SeekOrigin::Current);
	}
	cursor = BOF;
}

bool CsvReader::seek(int cursor)
{
	buffer.setLength(0);
	row = nullptr;
	if(!source) {
		return false;
	}
	auto newpos = std::max(cursor, int(start));
	if(newpos != int(sourcePos)) {
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
	return readRow();
}

bool CsvReader::readRow()
{
	if(!source) {
		row = nullptr;
		return false;
	}
	constexpr size_t minBufSize{512};

	constexpr char quoteChar{'"'};
	enum class FieldKind {
		unknown,
		quoted,
		unquoted,
	};
	FieldKind fieldKind{};
	bool escape{false};
	bool quote{false};
	char lc{'\0'};
	unsigned writepos{0};

	auto buflen = buffer.length();
	cursor = sourcePos - buflen;
	unsigned readpos{0};

	while(true) {
		if(buflen == maxLineLength) {
			debug_w("[CSV] Line buffer limit reached %u", maxLineLength);
			return false;
		}
		size_t newbuflen = std::max(minBufSize, maxLineLength);
		if(!buffer.setLength(newbuflen)) {
			debug_e("[CSV] Out of memory %u", newbuflen);
			return false;
		}
		auto len = source->readBytes(buffer.begin() + buflen, newbuflen - buflen);
		if(len == 0) {
			// End of input
			if(writepos == 0) {
				return false;
			}
			row = std::move(buffer);
			return true;
		}
		sourcePos += len;
		buflen += len;

		for(; readpos < buflen; ++readpos) {
			char c = buffer[readpos];
			if(escape) {
				switch(c) {
				case 'n':
					c = '\n';
					break;
				case 'r':
					c = '\r';
					break;
				case 't':
					c = '\t';
					break;
				default:;
					// Just accept character
				}
				escape = false;
			} else {
				if(fieldKind == FieldKind::unknown) {
					if(c == quoteChar) {
						fieldKind = FieldKind::quoted;
						quote = true;
						lc = '\0';
						continue;
					}
					fieldKind = FieldKind::unquoted;
				}
				if(c == quoteChar) {
					quote = !quote;
					if(fieldKind == FieldKind::quoted) {
						if(lc == quoteChar) {
							buffer[writepos++] = c;
							lc = '\0';
						} else {
							lc = c;
						}
						continue;
					}
				} else if(c == '\\') {
					escape = true;
					continue;
				} else if(!quote) {
					if(c == fieldSeparator) {
						c = '\0';
						fieldKind = FieldKind::unknown;
					} else if(c == '\r') {
						continue;
					} else if(c == '\n') {
						auto ptr = buffer.begin();
						auto tailpos = readpos + 1;
						auto taillen = buflen - readpos - 1;
						row = CStringArray(ptr, writepos);
						memcpy(ptr, ptr + tailpos, taillen);
						buffer.setLength(taillen);
						return true;
					}
				}
			}
			buffer[writepos++] = c;
			lc = c;
		}
		readpos = writepos;
	}
}
