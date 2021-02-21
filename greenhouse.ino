#include <RTClib.h>

#include <SPI.h>
#include <SD.h>

#include <Seeed_BME280.h>
#include <Wire.h>

#define S Serial // replace every 'S' with 'Serial'

RTC_DS3231 rtc; // real time clock
BME280 bme280; // temperature, air humidity and air pressure sensor
File file;

void setup()
{
	// initialize serial monitor
	S.begin(9600);

	// initialize real time clock
	S.print("Initializing RTC module: ");
	if (!rtc.begin())
	{
		S.println("Failed");
		return;
	}
	rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // save current time to rtc
	S.println("Done");

	// initialize sd-card
	S.print("Initializing SD-Card module: ");
	if (!SD.begin(5))
	{
		S.println("Failed");
		return;
	}
	S.println("Done");

	file = SD.open("file.txt", FILE_WRITE);
	file.println("Day,Month,Year,Hour,Minute");
	file.close();

	// initialize temperature, air humidity and air pressure sensor
	S.print("Initializing BME280 module: ");
	if(!bme280.init())
	{
		S.println("Failed");
		return;
	}
	S.println("Done");
}

void loop()
{
	// write data to sd-card
	S.print("Writing data to SD-Card: ");
	file = SD.open("file.txt", FILE_WRITE);

	String output;

	DateTime now = rtc.now();
	addToOutput(output, now.day());
	addToOutput(output, now.month());
	addToOutput(output, now.year());
	addToOutput(output, now.hour());
	addToOutput(output, now.minute());

//	file.print(now.day());
//	file.print(',');
//	file.print(now.month());
//	file.print(',');
//	file.print(now.year());
//	file.print(',');
//	file.print(now.hour());
//	file.print(',');
//	file.print(now.minute());
//	file.println();

	float pressure = bme280.getPressure();
	float bar = pressure / 100000;
	addToOutput(output, pressure);
	addToOutput(output, bar);

	file.println(output);
	file.close();

	S.println("Done");
	S.println(output);

	delay(5000);
}

void addToOutput(String& output, String addition) {
	if (output != "") {
		output += ",";
	}
	output += addition;
}

void addToOutput(String& output, uint16_t addition) {
	addToOutput(output, String(addition));
}

void addToOutput(String& output, int addition) {
	addToOutput(output, String(addition));
}

void addToOutput(String& output, float addition) {
	addToOutput(output, String(addition));
}
