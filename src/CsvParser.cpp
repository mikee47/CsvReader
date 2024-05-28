/**
 * CsvParser.cpp
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

#include "CsvParser.h"
#include <Data/Stream/LimitedMemoryStream.h>
#include <debug_progmem.h>

#define DEBUG_PARSER 0

bool CsvParser::parse(Stream& source, bool isFinished)
{
	while(readRow(source, isFinished)) {
		if(!row) {
			continue;
		}
		if(!headings) {
			setHeadings();
			continue;
		}
		if(rowCallback && !rowCallback(*this, row)) {
			return false;
		}
	}
	return true;
}

void CsvParser::setHeadings()
{
	this->headings = row;
	start = sourcePos - taillen;
	cursor = BOF;
}

bool CsvParser::parse(const char* data, size_t length)
{
	LimitedMemoryStream stream(const_cast<char*>(data), length, length, false);
	return parse(stream, length == 0);
}

void CsvParser::reset()
{
	if(!buffer) {
		buffer = std::move(reinterpret_cast<String&>(row));
	}
	buffer = "";
	cursor = BOF;
	sourcePos = start;
	taillen = 0;
}

bool CsvParser::readRow(Stream& source, bool eof)
{
	constexpr size_t minBufSize{512};
	constexpr char quoteChar{'"'};
	const size_t maxbuflen = std::max(minBufSize, maxLineLength);

	// Fields separated by whitespace and ignore leading/trailing whitespace
	bool wssep = (fieldSeparator == '\0');

	/*
	 * Ensure readpos > writepos.
	 * Row data is always <= source length, but when result is converted (in-situ) to CStringArray
	 * an additional '\0' (NUL) could overwrite our tail data.
	 */
	unsigned writepos{0};
	unsigned readpos{1};

	struct Flags {
		bool escape : 1;
		bool quote : 1;
		bool comment : 1;
		bool error : 1;
	};
	Flags flags{};

	enum class FieldKind {
		unknown,
		quoted,
		unquoted,
	};
	FieldKind fieldKind{};

	char lastChar{'\0'};

	if(!buffer) {
		buffer = std::move(reinterpret_cast<String&>(row));
	}

	auto bufptr = buffer.begin();
	if(taillen) {
		memmove(bufptr + readpos, bufptr + tailpos, taillen);
#if DEBUG_PARSER
		m_putc('\n');
		m_printHex("++", bufptr + readpos, taillen);
#endif
	}
	cursor = int(sourcePos - taillen);
	unsigned buflen = readpos + taillen;
	buffer.setLength(buflen);
	taillen = 0;

	if(buflen < maxbuflen) {
		if(!buffer.reserve(maxbuflen)) {
			debug_e("[CSV] Out of memory %u", maxbuflen);
			flags.error = true;
			return false;
		}
		bufptr = buffer.begin();
		auto len = source.readBytes(bufptr + buflen, maxbuflen - buflen);
		if(len) {
			sourcePos += len;
			buflen += len;
			buffer.setLength(buflen);
		} else if(!eof) {
			tailpos = readpos;
			taillen = buflen - readpos;
			return false;
		}
	}

	for(; readpos < buflen; ++readpos) {
		char c = bufptr[readpos];
		if(flags.comment) {
			if(c == '\n') {
				flags.comment = false;
				break;
			}
			continue;
		}
		if(flags.escape) {
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
			flags.escape = false;
		} else {
			if(fieldKind == FieldKind::unknown) {
				if(wssep && isspace(c)) {
					continue;
				}
				if(wssep && c == '#') {
					flags.comment = true;
					continue;
				}
				if(c == quoteChar) {
					fieldKind = FieldKind::quoted;
					flags.quote = true;
					lastChar = '\0';
					continue;
				}
				fieldKind = FieldKind::unquoted;
			}
			if(c == quoteChar) {
				flags.quote = !flags.quote;
				if(fieldKind == FieldKind::quoted) {
					if(lastChar == quoteChar) {
						buffer[writepos++] = c;
						lastChar = '\0';
					} else {
						lastChar = c;
					}
					continue;
				}
			} else if(c == '\\') {
				flags.escape = true;
				continue;
			} else if(!flags.quote) {
				if(c == '\r') {
					continue;
				} else if(c == '\n') {
					break;
				} else if((wssep && isspace(c)) || c == fieldSeparator) {
					c = '\0';
					fieldKind = FieldKind::unknown;
				}
			} else if(wssep && isspace(c)) {
				continue;
			}
		}
		bufptr[writepos++] = c;
		lastChar = c;
	}

	buffer.setLength(writepos);
	if(readpos < buflen) {
		tailpos = readpos + 1;
		taillen = buflen - readpos - 1;
	} else {
		taillen = 0;
	}

	// Ignore blank lines
	if(writepos == 0) {
		return !eof || readpos < buflen;
	}

	row = std::move(buffer);
#if DEBUG_PARSER
	m_putc('\n');
	m_printHex(">>", row.c_str(), writepos);
#endif
	return true;
}
