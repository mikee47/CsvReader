#include <SmingTest.h>
#include <CSV/Parser.h>
#include <malloc_count.h>
#include <WVector.h>

// 0: Normal operation
// 1: Generate output comparison files (host only)
#define GENERATE_OUT_FILES 0

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

class ParserTest : public TestGroup
{
public:
	ParserTest() : TestGroup(_F("Parser test"))
	{
	}

	void execute() override
	{
		fwfs_mount();

		parseFile(F("backward"), '\t', Mode::print);
		parseFile(F("backward"), '\t', Mode::timed);

		parseFile(F("zone1970.tab"), '\0', Mode::print);
		parseFile(F("antarctica"), '\0', Mode::print);

		parseFile(F("addresses.csv"), ',', Mode::dump);
	}

private:
	enum class Mode {
		print,
		dump,
		timed,
	};

	void parseFile(const String& filename, char sep, Mode mode)
	{
		this->mode = mode;
		lines.clear();

#if GENERATE_OUT_FILES
		auto& fs = IFS::Host::getFileSystem();
		out = IFS::File(&fs);
		CHECK(out.open(F("files/") + filename + ".out", File::CreateNewAlways | File::WriteOnly));
#else
		CHECK(out.open(filename + ".out"));
#endif

		Serial << endl << _F(">> Parse file '") << filename << '\'' << endl;

		CHECK(file.open(filename));
		CSV::Parser::Options options{
			.commentChars = "#",
			.fieldSeparator = sep,
		};
		parser = std::make_unique<CSV::Parser>(options);

		if(mode == Mode::timed) {
			printHeap();
		}

		CpuCycleTimer timer;

		char buffer[256];
		int len;
		while((len = file.read(buffer, sizeof(buffer))) > 0) {
			unsigned offset{0};
			while(parser->push(buffer, len, offset)) {
				handleRow();
			}
		}
		while(parser->flush()) {
			handleRow();
		}

		if(mode == Mode::timed) {
			auto elapsed = timer.elapsedTicks();
			printHeap();
			Serial << F("Elapsed ticks ") << elapsed << endl;
			return;
		}

		for(auto& cursor : lines) {
			file.seek(cursor.start, SeekOrigin::Start);
			String s;
			s.setLength(cursor.length());
			CHECK(file.read(s.begin(), s.length()) == s.length());
			// m_printHex("LINE", s.c_str(), s.length());
			Serial << s << endl;
		}
	}

	bool handleRow()
	{
		auto& row = parser->getRow();
		auto& cursor = parser->getCursor();

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

#if 0
		if(mode != Mode::timed) {
			/*
			 * Read raw line data from file and print it
			 *
			 * TODO: Can we do any validation here?
			 */
			auto oldPos = file.seek(0, SeekOrigin::Current);
			file.seek(cursor.start, SeekOrigin::Start);
			String s;
			s.setLength(cursor.length());
			CHECK(file.read(s.begin(), s.length()) == s.length());
			file.seek(oldPos, SeekOrigin::Start);
			// m_printHex("LINE", s.c_str(), s.length());
			Serial << s << endl;
		}
#endif

		switch(mode) {
		case Mode::print:
			Serial << "@" << cursor << " " << row.count() << " COLS: " << row.join(", ") << endl;
			lines.add(cursor);
			break;
		case Mode::dump:
			Serial << cursor << " " << row.count() << " COLS:" << endl;
			for(auto cell : row) {
				m_printHex("  CELL", cell, strlen(cell));
			}
			lines.add(cursor);
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
	Vector<CSV::Cursor> lines;
};

void REGISTER_TEST(parser)
{
	registerGroup<ParserTest>();
}
