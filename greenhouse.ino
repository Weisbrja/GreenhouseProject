#include <Wire.h>

#include <RTClib.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <SPI.h>
#include <SD.h>

#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define S Serial

#define LOOP_DELAY 30000
#define LCD_WIDTH  16

RTC_DS3231 rtc; // real time clock
Adafruit_BME280 bme280; // temperature and air humidity sensor
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
File file;

void setup()
{
	S.begin(9600);
	while (!S);

	lcd.begin(LCD_WIDTH, 2);
	lcd.backlight();

	initModules();
}

void initModules() {
	S.println("\n=== Initializing ===");

	if (!(initRTC() && initBME280() && initSDCard()))
	{
		delay(1000);
		initModules();
	}
}

void loop()
{
	S.println("\n=== Loop ===");

	// measure temperature
	const char temperature[10];
	dtostrf(bme280.readTemperature(), 1, 2, temperature);

	// measure humidity
	const char humidity[10];
	dtostrf(bme280.readHumidity(), 1, 2, humidity);

	// write temperature and humidity to file
	S.print("Writing data to SD-Card: ");
	file = SD.open("file.csv", FILE_WRITE);
	if (file)
	{
		// calculate current date
		DateTime now = rtc.now();
		const int year = now.year();
		const int month = now.month();
		const int day = now.day();
		int hour = now.hour();

		if (year >= 2021 && month > 3 || month == 3 && day >= 28 && hour >= 2)
			hour = (hour + 1) % 24;

		// write to file
		const char output[50];
		sprintf(output, "%d.%d.%d %d:%02d:%02d,%s,%s", day, month, year, hour, now.minute(), now.second(), temperature, humidity);
		file.println(output);
		file.close();

		S.println("Done\n>>>" + String(output));
	}
	else
	{
		S.println("Failed");
		initModules();
		return;
	}

	// print temperature and humidity to lcd
	S.print("Printing temperature and humidity to LCD: ");
	String temperature_s = String(temperature) + "C";
	String humidity_s = String(humidity) + '%';
	lcd.clear();
	lcd.setCursor(LCD_WIDTH - temperature_s.length(), 0);
	lcd.print(temperature_s);
	lcd.setCursor(LCD_WIDTH - humidity_s.length(), 1);
	lcd.print(humidity_s);
	S.println("Done");

	delay(LOOP_DELAY);
}

bool initRTC()
{
	S.print("RTC: ");
	if (rtc.begin())
	{
		// save current time to real time clock
		rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
		S.println("Done");
		return true;
	}
	else
	{
		S.println("Failed");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("RTC: Failed");
		return false;
	}
}

bool initBME280()
{
	S.print("BME280: ");
	if (bme280.begin(0x76))
	{
		S.println("Done");
		return true;
	}
	else
	{
		S.println("Failed");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("BME280: Failed");
		return false;
	}
}

bool initSDCard()
{
	S.print("SD-Card: ");
	if (SD.begin(5))
	{
		// write table header to file
		file = SD.open("file.csv", FILE_WRITE);
		String header = "Time,Temperature,Humidity";
		file.println(header);
		file.close();

		S.println("Done\n>>> " + header);
		return true;
	}
	else
	{
		S.println("Failed");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("SD-Card: Failed");
		return false;
	}
}
