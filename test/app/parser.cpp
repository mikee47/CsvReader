#include <SmingTest.h>
#include <CSV/Parser.h>
#include <malloc_count.h>

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
		while((len = file.read(buffer, sizeof(buffer)))) {
			size_t offset{0};
			while(offset < len && parser->readRow(buffer, len, &offset)) {
				handleRow();
			}
		}
		while(parser->readRow(nullptr, 0, nullptr)) {
			handleRow();
		}

		if(mode == Mode::timed) {
			auto elapsed = timer.elapsedTicks();
			printHeap();
			Serial << F("Elapsed ticks ") << elapsed << endl;
		}
	}

	bool handleRow()
	{
		auto& row = parser->getRow();
		auto cursor = parser->getCursor();
		switch(mode) {
		case Mode::print:
			Serial << "@" << cursor << " " << row.count() << " COLS: " << row.join(", ") << endl;
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
	std::unique_ptr<CSV::Parser> parser;
	size_t totalRowSize{0};
	Mode mode{};
};

void REGISTER_TEST(parser)
{
	registerGroup<ParserTest>();
}
