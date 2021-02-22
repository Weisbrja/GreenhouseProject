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
	S.begin(9600);

	S.println("\n### Setup ###");

	initModules();

	S.println("\n### Loop ###");
}

void initModules() {
	S.println("\n=== Initializing Modules ===");

	if (!(initRTC() && initBME280() && initSDCard()))
	{
		delay(5000);
		initModules();
	}
}

void loop()
{
	// write data to file
	S.print("\nWriting data to SD-Card: ");
	file = SD.open("file.csv", FILE_WRITE);
	if (file)
	{
		// calculate date
		DateTime now = rtc.now();
		const int year = now.year();
		const int month = now.month();
		const int day = now.day();
		int hour = now.hour();

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
		sprintf(output, "%d.%d.%d %d:%02d:%02d,%s,%s", day, month, year, hour, now.minute(), now.second(), temperature, humidity);
		file.println(output);

		file.close();

		S.println("Done");

		S.println(">>> " + String(output));
	}
	else
	{
		S.println("Failed");
		initModules();
		return;
	}

	// wait for 30 seconds
	delay(30000);
}

bool initRTC()
{
	// initialize real time clock
	S.print("RTC: ");
	if (rtc.begin())
	{
		rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // save current time to real time clock
		S.println("Done");
		return true;
	}
	else
	{
		S.println("Failed");
		return false;
	}
}

bool initBME280()
{
	// initialize temperature and air humidity sensor
	S.print("BME280: ");
	if (bme280.begin(0x76))
	{
		S.println("Done");
		return true;
	}
	else
	{
		S.println("Failed");
		return false;
	}
}

bool initSDCard()
{
	// initialize sd-card
	S.print("SD-Card: ");
	if (SD.begin(5))
	{
		// write table header to file
		file = SD.open("file.csv", FILE_WRITE);
		String header = "Day,Month,Year,Hour,Minute,Temperature,Humidity";
		file.println(header);
		file.close();

		S.println("Done");

		S.println(">>> " + header);

		return true;
	}
	else
	{
		S.println("Failed");
		return false;
	}
}
