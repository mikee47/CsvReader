#include <SmingCore.h>
#include <CSV/Table.h>

namespace
{
/*
 * Pull in file containing source data to parse
 */
IMPORT_FSTR(backward, PROJECT_DIR "/backward")

/*
 * Define the structure of each record in our source data
 */
struct Link : public CSV::Record {
	enum ColumnIndex {
		col_type,
		col_target,
		col_link,
	};

	using Record::Record;

	const char* type() const
	{
		return row[col_type];
	}

	const char* target() const
	{
		return row[col_target];
	}

	const char* link() const
	{
		return row[col_link];
	}
};

void demoReader()
{
	// Construct the reader as a table so we can use iteration
	CSV::Table<Link> table(new FlashMemoryStream(backward), //
						   {
							   .commentChars = "#",
							   .fieldSeparator = '\0', // Whitespace separated
							   .wantComments = false,  // Discard comments
						   });
	// Iterate and filter records, print a selection
	for(auto rec : table) {
		if(!String(rec.target()).startsWith(_F("Europe"))) {
			continue;
		}
		Serial << rec.type() << ": " << rec.link() << " -> " << rec.target() << endl;
	}
}

} // namespace

void init()
{
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true); // Allow debug print to serial

	demoReader();

	Serial << endl << _F("OK, end of demo. That's it.") << endl;
}
