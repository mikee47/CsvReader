#include <SmingTest.h>
#include <CSV/Parser.h>
#include <malloc_count.h>
#include <WVector.h>

// 0: Normal
// 1: Generate output comparison files (host only)
#define GENERATE_OUT_FILES 0

// 0: Normal
// 1: Store list of record lines and print them after parsing
#define PRINT_SOURCE_LINES 0

namespace
{
void printHeap()
{
	Serial << _F("Heap statistics") << endl;
	Serial << _F("  Free bytes:  ") << system_get_free_heap_size() << endl;
#ifdef ENABLE_MALLOC_COUNT
	Serial << _F("  Used:        ") << MallocCount::getCurrent() << endl;
	Serial << _F("  Peak used:   ") << MallocCount::getPeak() << endl;
	Serial << _F("  Allocations: ") << MallocCount::getAllocCount() << endl;
	Serial << _F("  Total used:  ") << MallocCount::getTotal() << endl;
#endif
}

} // namespace

using Options = CSV::Parser::Options;

class ParserTest : public TestGroup
{
public:
	ParserTest() : TestGroup(_F("Parser test"))
	{
	}

	void execute() override
	{
		parseFile(F("backward"),
				  Options{
					  .commentChars = "#",
					  .fieldSeparator = '\0',
				  },
				  Mode::print);
		parseFile(F("backward"),
				  Options{
					  .fieldSeparator = '\t',
				  },
				  Mode::timed);

		parseFile(F("zone1970.tab"),
				  Options{
					  .commentChars = "#",
					  .fieldSeparator = '\t',
				  },
				  Mode::print);

		parseFile(F("antarctica"),
				  Options{
					  .commentChars = "#",
					  .fieldSeparator = '\0',
				  },
				  Mode::print);

		parseFile(F("addresses.csv"),
				  Options{
					  .fieldSeparator = ',',
				  },
				  Mode::dump);

		parseFile(F("test.csv"),
				  Options{
					  .fieldSeparator = ',',
				  },
				  Mode::dump);
	}

private:
	enum class Mode {
		print,
		dump,
		timed,
	};

	void parseFile(const String& filename, const Options& options, Mode mode)
	{
		this->mode = mode;
#if PRINT_SOURCE_LINES
		lines.clear();
#endif

		Serial << endl << _F(">> Parse file '") << filename << '\'' << endl;

		if(!file.open(filename)) {
			Serial << filename << ": " << file.getLastErrorString() << endl;
			TEST_ASSERT(false);
		}
		parser = std::make_unique<CSV::Parser>(options);

#ifdef ENABLE_MALLOC_COUNT
		size_t allocCount = MallocCount::getAllocCount();
#endif
		if(mode == Mode::timed) {
			printHeap();
		} else {
#if GENERATE_OUT_FILES
			auto& fs = IFS::Host::getFileSystem();
			out = IFS::File(&fs);
			CHECK(out.open(F("files/") + filename + ".out", File::CreateNewAlways | File::WriteOnly));
#else
			if(!out.open(filename + ".out")) {
				Serial << filename << ".out: " << out.getLastErrorString() << endl;
				TEST_ASSERT(false);
			}
#endif
		}

		CpuCycleTimer timer;

		char buffer[55];
		int len;
		while((len = file.read(buffer, sizeof(buffer))) > 0) {
			size_t offset{0};
			while(parser->push(buffer, len, offset)) {
				handleRow();
			}
			CHECK(int(offset) == len);
		}
		while(parser->flush()) {
			handleRow();
		}

		if(mode == Mode::timed) {
			auto elapsed = timer.elapsedTicks();
			printHeap();
			Serial << _F("Elapsed ticks ") << elapsed << endl;
#ifdef ENABLE_MALLOC_COUNT
			// CSV parser should make exactly one heap allocation, but for some reason we get 2 in Windows!
			auto newAllocCount = MallocCount::getAllocCount();
			CHECK(newAllocCount <= allocCount + 2);
#endif
		} else {
#if !GENERATE_OUT_FILES
			CHECK(out.eof());
#endif
#if PRINT_SOURCE_LINES
			Serial << endl << _F("Source lines:") << endl;
			for(auto& cursor : lines) {
				file.seek(cursor.start, SeekOrigin::Start);
				String s;
				s.setLength(cursor.length());
				CHECK_EQ(file.read(s.begin(), s.length()), int(s.length()));
				// m_printHex("LINE", s.c_str(), s.length());
				Serial << "> " << s << endl;
			}
#endif
		}
	}

	bool handleRow()
	{
		auto& row = parser->getRow();
		auto& cursor = parser->getCursor();

		if(mode != Mode::timed) {
#if PRINT_SOURCE_LINES
			lines.add(cursor);
#endif
#if GENERATE_OUT_FILES
			uint16_t len = row.length();
			out.write(&len, sizeof(len));
			out.write(row.c_str(), len);
#else
			uint16_t len;
			CHECK(out.read(&len, sizeof(len)) == sizeof(len));
			String s;
			s.setLength(len);
			CHECK(out.read(s.begin(), s.length()) == len);
			CHECK(row == s);
#endif
		}

		switch(mode) {
		case Mode::print:
			Serial << "@" << cursor << " " << row.count() << " COLS: " << row.join("; ") << endl;
			break;
		case Mode::dump:
			Serial << cursor << " " << row.count() << " COLS:" << endl;
			for(auto cell : row) {
				m_printHex("  CELL", cell, strlen(cell));
			}
			break;
		case Mode::timed:
			break;
		}
		totalRowSize += row.length();
		return true;
	}

	File file;
	File out;
	std::unique_ptr<CSV::Parser> parser;
	size_t totalRowSize{0};
	Mode mode{};
#if PRINT_SOURCE_LINES
	Vector<CSV::Cursor> lines;
#endif
};

void REGISTER_TEST(parser)
{
	registerGroup<ParserTest>();
}
