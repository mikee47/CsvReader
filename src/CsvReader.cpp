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
	: source(source), maxLineLength(maxLineLength), headings(headings), fieldSeparator(fieldSeparator)
{
	if(source && !headings) {
		readRow();
		this->headings = row;
		start = sourcePos - taillen;
	}
	cursor = BOF;
}

bool CsvReader::seek(int cursor)
{
	row = "";
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
		eof = false;
		taillen = 0;
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

	/*
	 * Ensure readpos > writepos.
	 * Row data is always <= source length, but when result is converted (in-situ) to CStringArray
	 * an additional '\0' (NUL) could overwrite our tail data.
	 */
	unsigned writepos{0};
	unsigned readpos{1};

	String buffer(std::move(reinterpret_cast<String&>(row)));
	auto bufptr = buffer.begin();
	if(taillen) {
		memmove(bufptr + readpos, bufptr + tailpos, taillen);
#if DEBUG_READER
		m_putc('\n');
		m_printHex("++", bufptr + readpos, taillen);
#endif
	}
	cursor = int(sourcePos - taillen);
	unsigned buflen = readpos + taillen;
	taillen = 0;

	enum class FieldKind {
		unknown,
		quoted,
		unquoted,
	};
	FieldKind fieldKind{};
	bool escape{false};
	bool quote{false};
	char lastChar{'\0'};
	do {
		if(!eof) {
			size_t newbuflen = std::max(minBufSize, maxLineLength);
			if(!buffer.setLength(newbuflen)) {
				debug_e("[CSV] Out of memory %u", newbuflen);
				return false;
			}
			bufptr = buffer.begin();
			auto len = source->readBytes(bufptr + buflen, newbuflen - buflen);
			if(len == 0) {
				// End of input
				eof = true;
			} else {
				sourcePos += len;
				buflen += len;
			}
		}

		for(; readpos < buflen; ++readpos) {
			char c = bufptr[readpos];
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
						lastChar = '\0';
						continue;
					}
					fieldKind = FieldKind::unquoted;
				}
				if(c == quoteChar) {
					quote = !quote;
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
					escape = true;
					continue;
				} else if(!quote) {
					if(c == fieldSeparator) {
						c = '\0';
						fieldKind = FieldKind::unknown;
					} else if(c == '\r') {
						continue;
					} else if(c == '\n') {
						buffer.setLength(writepos);
						row = std::move(buffer);
#if DEBUG_READER
						m_putc('\n');
						m_printHex(">>", row.c_str(), writepos);
#endif
						tailpos = readpos + 1;
						taillen = buflen - readpos - 1;
						return true;
					}
				}
			}
			bufptr[writepos++] = c;
			lastChar = c;
		}
		readpos = writepos + 1;

		if(eof) {
			if(writepos) {
				buffer.setLength(writepos);
				row = std::move(buffer);
#if DEBUG_READER
				m_putc('\n');
				m_printHex(">L", row.c_str(), writepos);
#endif
				return true;
			}
			row = "";
			return false;
		}
	} while(buflen < maxLineLength);

	debug_w("[CSV] Line buffer limit reached %u", maxLineLength);
	return false;
}
