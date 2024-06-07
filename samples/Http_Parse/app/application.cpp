#include <SmingCore.h>
#include <CSV/Reader.h>

#ifndef WIFI_SSID
#define WIFI_SSID "PleaseEnterSSID" // Put your SSID and password here
#define WIFI_PWD "PleaseEnterPass"
#endif

namespace
{
#ifdef ENABLE_SSL
// For regular CI testing avoid external websites
DEFINE_FSTR_LOCAL(ZONE1970_TAB_URL,
				  "https://raw.githubusercontent.com/mikee47/CsvReader/develop/test/files/zone1970.tab")
#else
// Non-https URLs are in short supply, but this is a good one
DEFINE_FSTR_LOCAL(ZONE1970_TAB_URL, "http://data.iana.org/time-zones/tzdb/zone1970.tab")
#endif

HttpClient downloadClient;
std::unique_ptr<CSV::Parser> parser;
size_t totalRowSize;

bool handleRow(const CStringArray& row)
{
	Serial << row.join("\t") << endl;
	totalRowSize += row.length();
	return true;
}

int onRequestBody(HttpConnection& client, const char* at, size_t length)
{
	size_t offset{0};
	while(parser->push(at, length, offset)) {
		handleRow(parser->getRow());
	}
	return 0;
}

int onDownload(HttpConnection& connection, bool success)
{
	while(parser->flush()) {
		handleRow(parser->getRow());
	}
	Serial << _F("Bytes received ") << parser->tell() << _F(", output ") << totalRowSize << endl;
	auto status = connection.getResponse()->code;
	Serial << _F("Got response code: ") << unsigned(status) << " (" << status << _F("), success: ") << success << endl;

	parser.reset();

	return 0;
}

void gotIP(IpAddress ip, IpAddress netmask, IpAddress gateway)
{
	Serial << _F("Connected. Got IP: ") << ip << endl;

	parser = std::make_unique<CSV::Parser>(CSV::Parser::Options{
		.commentChars = "#",
		.lineLength = 150,
		.fieldSeparator = '\t',
	});
	auto request = new HttpRequest(String(ZONE1970_TAB_URL));
	request->onSslInit([](auto& session, auto& request) { session.options.verifyLater = true; });
	request->onBody(onRequestBody);
	request->onRequestComplete(onDownload);
	if(downloadClient.send(request)) {
		Serial.println(_F("OK, request sent"));
	} else {
		Serial.println(_F("Error sending request"));
	}
}

void connectFail(const String& ssid, MacAddress bssid, WifiDisconnectReason reason)
{
	Serial.println(_F("I'm NOT CONNECTED!"));
}

} // namespace

void init()
{
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println(_F("Ready for SSL tests"));

	// Setup the WIFI connection
	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD); // Put your SSID and password here

	WifiEvents.onStationGotIP(gotIP);
	WifiEvents.onStationDisconnect(connectFail);
}
