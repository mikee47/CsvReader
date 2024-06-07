/****
 * Parser.cpp
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

#include "include/CSV/Parser.h"
#include <Data/Stream/LimitedMemoryStream.h>
#include <debug_progmem.h>

#define DEBUG_PARSER 0

namespace CSV
{
/*
 * Ensure readpos > writepos.
 * Row data is always <= source length, but when result is converted (in-situ) to CStringArray
 * an additional '\0' (NUL) could overwrite our tail data.
 */
static const size_t READ_OFFSET = 1;

bool Parser::push(Stream& source)
{
	for(;;) {
		auto len = fillBuffer(source);
		if(len < options.lineLength) {
			return false;
		}
		if(!parseRow(false)) {
			return false;
		}
		if(row.length()) {
			return true;
		}
	}
}

bool Parser::push(const char* data, size_t length, size_t& offset)
{
	LimitedMemoryStream source(const_cast<char*>(data), length, length, false);
	source.seekFrom(offset, SeekOrigin::Start);
	bool res = push(source);
	offset = source.getStreamPointer() - data;
	return res;
}

bool Parser::flush()
{
	while(parseRow(true)) {
		if(row.length()) {
			return true;
		}
	}
	return false;
}

bool Parser::readRow(IDataSourceStream& source)
{
	for(;;) {
		auto len = fillBuffer(source);
		bool eof = source.isFinished();
		if(!eof && len < options.lineLength) {
			return false;
		}
		if(!parseRow(eof)) {
			return false;
		}
		if(row.length()) {
			return true;
		}
	}
}

void Parser::reset(int offset)
{
	if(!buffer) {
		buffer = row.release();
	}
	if(buffer) {
		buffer.setLength(READ_OFFSET);
	}
	cursor = {offset};
	sourcePos = std::max(offset, 0);
	taillen = 0;
}

size_t Parser::fillBuffer(Stream& source)
{
	const size_t minBufSize{512};
	const size_t maxbuflen = std::max(minBufSize, READ_OFFSET + options.lineLength + 2);

	char* bufptr;
	size_t buflen;

	if(buffer) {
		bufptr = buffer.begin();
		buflen = buffer.length();
	} else {
		buffer = row.release();
		if(!buffer.reserve(maxbuflen)) {
			debug_e("[CSV] Out of memory %u", maxbuflen);
			// flags.error = true;
			return 0;
		}

		bufptr = buffer.begin();
		if(taillen != 0) {
			memmove(bufptr + READ_OFFSET, bufptr + tailpos, taillen);
#if DEBUG_PARSER
			m_putc('\n');
			m_printHex("++", bufptr + READ_OFFSET, taillen);
#endif
		}
		buflen = READ_OFFSET + taillen;
		taillen = 0;
	}

	auto len = source.readBytes(bufptr + buflen, maxbuflen - buflen);
	if(len) {
		sourcePos += len;
		buflen += len;
	}

	buffer.setLength(buflen);
	return buflen - READ_OFFSET;
}

bool Parser::parseRow(bool eof)
{
	constexpr char quoteChar{'"'};

	// Fields separated by whitespace and ignore leading/trailing whitespace
	bool wssep = (options.fieldSeparator == '\0');

	/*
	 * Ensure readpos > writepos.
	 * Row data is always <= source length, but when result is converted (in-situ) to CStringArray
	 * an additional '\0' (NUL) could overwrite our tail data.
	 */
	unsigned writepos = 0;
	unsigned readpos = READ_OFFSET;

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

	auto bufptr = buffer.begin();
	auto buflen = buffer.length();

	cursor = {int(sourcePos + READ_OFFSET - buflen)};

	for(; readpos < buflen; ++readpos) {
		char c = bufptr[readpos];
		if(flags.comment) {
			if(c == '\n') {
				flags.comment = false;
				break;
			}
			if(options.wantComments) {
				bufptr[writepos++] = c;
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
				if(options.commentChars && strchr(options.commentChars, c)) {
					flags.comment = true;
					if(options.wantComments) {
						bufptr[writepos++] = c;
					}
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
					cursor.end = cursor.start + readpos - READ_OFFSET;
					break;
				} else if((wssep && isspace(c)) || c == options.fieldSeparator) {
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

	if(readpos < buflen) {
		tailpos = readpos + 1;
		taillen = buflen - readpos - 1;
	} else {
		taillen = 0;
	}

	if(cursor.end == 0) {
		cursor.end = sourcePos;
	}

	buffer.setLength(writepos);
	row = std::move(buffer);
#if DEBUG_PARSER
	m_putc('\n');
	m_printHex(">>", row.c_str(), writepos);
#endif

	// Ignore blank lines
	if(writepos == 0) {
		return !eof || readpos < buflen;
	}

	return true;
}

} // namespace CSV
