#include <SmingTest.h>
#include <CsvReader.h>

DEFINE_FSTR_LOCAL(
	test1_csv,
	"\"field1\",field2,field3,\"field four\"\n"
	"Something \"awry\",\"datavalue 2\",\"where,are,\"\"the,\nbananas\",sausages abound,\"never surrender\"\n"
	"one,two,three,four\n"
	"a,b,c,d,e,f")

DEFINE_FSTR_LOCAL(csv_headings, "field1;field2;field3;field four")
DEFINE_FSTR_LOCAL(csv_row1, "Something \"awry\";datavalue 2;where,are,\"the,\nbananas;sausages abound;never surrender")
DEFINE_FSTR_LOCAL(csv_row2, "one;two;three;four")
DEFINE_FSTR_LOCAL(csv_row3, "a;b;c;d;e;f")

class BasicTest : public TestGroup
{
public:
	BasicTest() : TestGroup(_F("Basic Tests"))
	{
	}

	void execute() override
	{
		TEST_CASE("Basic")
		{
			CsvReader reader(new FSTR::Stream(test1_csv, ','));

			const char* sep = ";";

			Serial << "Cursor " << reader.tell() << endl;
			auto headings = reader.getHeadings();
			Serial.println(headings.join(sep));
			CHECK(csv_headings == headings.join(sep));

			REQUIRE(reader.next());
			auto cursor1 = reader.tell();
			Serial << "Cursor " << cursor1 << endl;
			auto row = reader.getRow();
			Serial.println(row.join(sep));
			CHECK(csv_row1 == row.join(sep));

			REQUIRE(reader.next());
			auto cursor2 = reader.tell();
			Serial << "Cursor " << cursor2 << endl;
			row = reader.getRow();
			REQUIRE(csv_row2 == row.join(sep));

			REQUIRE(reader.next());
			auto cursor3 = reader.tell();
			Serial << "Cursor " << cursor3 << endl;
			row = reader.getRow();
			REQUIRE(csv_row3 == row.join(sep));

			REQUIRE(!reader.next());
			Serial << "Cursor " << reader.tell() << endl;

			TEST_CASE("reset")
			{
				reader.reset();
				REQUIRE(reader.next());
				row = reader.getRow();
				REQUIRE(csv_row1 == row.join(sep));
			}

			TEST_CASE("seek")
			{
				reader.seek(cursor2);
				row = reader.getRow();
				REQUIRE(csv_row2 == row.join(sep));

				reader.seek(cursor1);
				row = reader.getRow();
				REQUIRE(csv_row1 == row.join(sep));

				reader.seek(cursor3);
				row = reader.getRow();
				REQUIRE(csv_row3 == row.join(sep));
			}
		}
	}
};

void REGISTER_TEST(basic)
{
	registerGroup<BasicTest>();
}
