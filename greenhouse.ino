#include <RTClib.h>

#include <SPI.h>
#include <SD.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define S Serial

RTC_DS3231 rtc; // real time clock
Adafruit_BME280 bme280; // temperature and air humidity sensor
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
	rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // save current time to real time clock
	S.println("Done");

	// initialize sd-card
	S.print("Initializing SD-Card module: ");
	if (!SD.begin(5))
	{
		S.println("Failed");
		return;
	}
	S.println("Done");

	// write table header to file
	file = SD.open("file.csv", FILE_WRITE);
	String header = "Day,Month,Year,Hour,Minute,Temperature,Humidity";
	file.println(header);
	file.close();
	S.println(header);

	// initialize temperature and air humidity sensor
	S.print("Initializing BME280 module: ");
	if(!bme280.begin(0x76))
	{
		S.println("Failed");
		return;
	}
	S.println("Done");
}

void loop()
{
	// write data to file
	S.print("Writing data to SD-Card: ");
	file = SD.open("file.csv", FILE_WRITE);
	if (file)
	{
		// calculate date
		DateTime now = rtc.now();
		const int year = now.year();
		const int month = now.month();
		const int day = now.day();
		int hour = now.hour();
		const int minute = now.minute();

		if (year >= 2021 && month > 3 || month == 3 && day >= 28 && hour >= 2)
			hour = (hour + 1) % 24;

		// measure temperature
		const char temperature[10];
		dtostrf(bme280.readTemperature(), 1, 2, temperature);

		// measure humidity
		const char humidity[10];
		dtostrf(bme280.readHumidity(), 1, 2, humidity);

		// write to file
		const char output[50];
		sprintf(output, "%d.%d.%d %d:%2d,%s,%s", day, month, year, hour, minute, temperature, humidity);
		file.println(output);

		file.close();

		S.println("Done");

		S.println(output);
	}
	else
		S.println("Failed");

	// wait for five seconds
	delay(5000);
}
