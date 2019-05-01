/**
 * application.cpp - CsvReader library integration tests
 *
 * Copyright 2024 mikee47 <mike@sillyhouse.net>
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

#include <SmingTest.h>
#include "modules.h"

#define XX(t) extern void REGISTER_TEST(t);
TEST_MAP(XX)
#undef XX

namespace
{
void registerTests()
{
#define XX(t) REGISTER_TEST(t);
	TEST_MAP(XX)
#undef XX
}

void testsComplete()
{
#ifdef ARCH_HOST
	// In the Host Emulator, this ends the session
	System.restart();
#endif
}

void beginTests()
{
	System.onReady([]() { SmingTest::runner.execute(testsComplete); });
}

} // namespace

void init()
{
	Serial.setTxBufferSize(1024);
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true);

	registerTests();
	beginTests();
}
