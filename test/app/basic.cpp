#include <SmingTest.h>
#include <CsvReader.h>

DEFINE_FSTR_LOCAL(
	test1_csv, "\"field1\",field2,field3,\"field four\"\n"
			   "Something \"awry\",\"datavalue 2\",\"where,are,\"\"the,bananas\",sausages abound,\"never surrender\"")
DEFINE_FSTR_LOCAL(csv_headings, "field1;field2;field3;field four;")
DEFINE_FSTR_LOCAL(csv_row1, "Something \"awry\";datavalue 2;where,are,\"the,bananas;sausages abound;never surrender;")

class BasicTest : public TestGroup
{
public:
	BasicTest() : TestGroup(_F("Basic Tests"))
	{
	}

	void execute() override
	{
		TEST_CASE("CSV Reader")
		{
			auto str = [](const CStringArray& cs) {
				String s = reinterpret_cast<const String&>(cs);
				s.replace('\0', ';');
				return s;
			};

			CsvReader reader(new FSTR::Stream(test1_csv));
			String headings = str(reader.getHeadings());
			Serial.println(headings);
			CHECK(reader.next());
			String row1 = str(reader.getRow());
			Serial.println(row1);
			CHECK(!reader.next());

			CHECK(csv_headings == headings);
			CHECK(csv_row1 == row1);
		}
	}
};

void REGISTER_TEST(basic)
{
	registerGroup<BasicTest>();
}
