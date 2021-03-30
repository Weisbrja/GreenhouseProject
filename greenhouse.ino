#include <Wire.h>

#include <RTClib.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <SPI.h>
#include <SD.h>

#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define S Serial

// global constants
#define LOOP_DELAY                 30000
#define LCD_WIDTH                  16
#define SOIL_MOISTURE_SENSOR_PIN   A0
#define SD_CARD_PIN                5
#define PUMP_PIN                   10
#define PUMP_MIN                   425
#define PUMP_MAX                   1023
#define PUMP_THRESHOLD             50
#define PUMP_DELAY                 5000

RTC_DS3231 rtc; // real time clock
Adafruit_BME280 bme280; // temperature and air humidity sensor
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
File file;

void setup()
{
	pinMode(SOIL_MOISTURE_SENSOR_PIN, INPUT);
	pinMode(PUMP_PIN, OUTPUT);

	S.begin(9600);

	lcd.begin(LCD_WIDTH, 2);
	lcd.backlight();

	delay(1000);

	initModules();
}

void initModules()
{
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

	// measure temperature in C
	const char temperature_c[10];
	dtostrf(bme280.readTemperature(), 1, 2, temperature_c);

	// measure air humidity in %
	const char airHumidity_c[10];
	dtostrf(bme280.readHumidity(), 1, 2, airHumidity_c);

	// measure soil moisture in %
	const int soilMoisture = map(analogRead(SOIL_MOISTURE_SENSOR_PIN), PUMP_MIN, PUMP_MAX, 100, 0);

	// print temperature, air humidity and soil moisture on lcd
	S.print("Printing temperature, air humidity and soil moisture on LCD: ");
	String temperature_s = "T=" + String(temperature_c) + 'C';
	String airHumidity_s = "H=" + String(airHumidity_c) + '%';
	String soilMoisture_s = "M=" + String(soilMoisture) + '%';
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print(soilMoisture_s);
	lcd.setCursor(LCD_WIDTH - airHumidity_s.length(), 0);
	lcd.print(airHumidity_s);
	lcd.setCursor(LCD_WIDTH - temperature_s.length(), 1);
	lcd.print(temperature_s);
	S.println("Done");

	// write date, temperature, air humidity and soil moisture to file
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

		if (year == 2021 && month > 3 || month == 3 && day >= 28 && hour >= 2)
			hour = (hour + 1) % 24;

		// write to file
		const char output[50];
		sprintf(output, "%d.%d.%d %d:%02d:%02d,%s,%s,%d", day, month, year, hour, now.minute(), now.second(), temperature_c, airHumidity_c, soilMoisture);
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

	// turn pump on if the soil moisture is below the pump threshold
	if (soilMoisture < PUMP_THRESHOLD)
	{
		S.print("Pumping water: ");
		digitalWrite(PUMP_PIN, HIGH);
		delay(PUMP_DELAY);
		digitalWrite(PUMP_PIN, LOW);
		S.println("Done");
		delay(LOOP_DELAY - PUMP_DELAY);
	}
	else
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
	if (SD.begin(SD_CARD_PIN))
	{
		// write table header to file
		file = SD.open("file.csv", FILE_WRITE);
		String header = "Time,Temperature in C,Humidity in %,Soil moisture in %";
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
