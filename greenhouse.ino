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
#define LCD_WIDTH                  16
#define SD_CARD_PIN                5
#define PUMP_PIN                   10
#define SOIL_MOISTURE_SENSOR_PIN   A0
#define LOOP_DELAY                 30000
#define PUMP_DELAY                 20000
#define SOIL_MOISTURE_MIN          200
#define SOIL_MOISTURE_MAX          800
#define PUMP_THRESHOLD             30

RTC_DS3231 rtc; // real time clock
Adafruit_BME280 bme280; // temperature and air humidity sensor
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // i2c lcd
File file; // file on sd-card

void setup()
{
	// declare input and output pins
	pinMode(SOIL_MOISTURE_SENSOR_PIN, INPUT);
	pinMode(PUMP_PIN, OUTPUT);

	// initialize serial monitor
	S.begin(9600);

	// initialize lcd
	lcd.begin(LCD_WIDTH, 2);
	lcd.backlight();

	delay(1000);

	initModules();
}

void loop()
{
	S.println("\n=== Loop ===");

	// measure soil moisture in %
	const int soilMoistureValue = map(analogRead(SOIL_MOISTURE_SENSOR_PIN), SOIL_MOISTURE_MIN, SOIL_MOISTURE_MAX, 100, 0);

#define SOIL_MOISTURE_LENGTH 7
	const char soilMoisture[SOIL_MOISTURE_LENGTH];
	snprintf(soilMoisture, SOIL_MOISTURE_LENGTH, "M=%d%%", soilMoistureValue);

	// measure air humidity in %
#define AIR_HUMIDITY_RAW_LENGTH 7
	const char airHumidityRaw[AIR_HUMIDITY_RAW_LENGTH];
	dtostrf(bme280.readHumidity(), 1, 2, airHumidityRaw);

#define AIR_HUMIDITY_LENGTH AIR_HUMIDITY_RAW_LENGTH + 3
	const char airHumidity[AIR_HUMIDITY_LENGTH];
	snprintf(airHumidity, AIR_HUMIDITY_LENGTH, "H=%s%%", airHumidityRaw);

	// measure temperature in °C
#define TEMPERATURE_RAW_LENGTH 7
	const char temperatureRaw[TEMPERATURE_RAW_LENGTH];
	dtostrf(bme280.readTemperature(), 1, 2, temperatureRaw);

#define TEMPERATURE_LENGTH TEMPERATURE_RAW_LENGTH + 3
	const char temperature[TEMPERATURE_LENGTH];
	snprintf(temperature, TEMPERATURE_LENGTH, "T=%sC", temperatureRaw);

	// check if the automatic irrigation needs to be recalibrated
	if (soilMoistureValue < 0 || soilMoistureValue > 100)
		S.println("!!! Recalibrate the automatic irrigation");

	// print soil moisture, air humidity and temperature on lcd
	S.print("Printing soil moisture, air humidity and temperature on LCD: ");
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print(soilMoisture);
	lcd.setCursor(LCD_WIDTH - strlen(airHumidity), 0);
	lcd.print(airHumidity);
	lcd.setCursor(LCD_WIDTH - strlen(temperature), 1);
	lcd.print(temperature);
	S.println("Done");

	// write date, soil moisture, air humidity and temperature to file on sd-card
	S.print("Writing data to SD-Card: ");
	file = SD.open("file.csv", FILE_WRITE);
	if (file)
	{
		// calculate date
		DateTime now = rtc.now();
		const int year = now.year();
		const int month = now.month();
		const int day = now.day();
		const int hour = now.hour() % 24;

		// write to file on sd-card
#define OUTPUT_LENGTH 50
		const char output[OUTPUT_LENGTH];
		snprintf(output, OUTPUT_LENGTH, "%02d.%02d.%d %02d:%02d:%02d,%d,%s,%s", day, month, year, hour, now.minute(), now.second(), soilMoistureValue, airHumidityRaw, temperatureRaw);
		file.println(output);
		file.close();

#define PRINT_OUTPUT_LENGTH OUTPUT_LENGTH + 8
		const char printOutput[PRINT_OUTPUT_LENGTH];
		snprintf(printOutput, PRINT_OUTPUT_LENGTH, "Done\n>>> %s", output);
		S.println(printOutput);
	}
	else
	{
		// reinitialze modules because file could not be written
		S.println("Failed");
		initModules();
		return;
	}

	// turn pump on if the soil moisture is below the pump threshold
	if (soilMoistureValue < PUMP_THRESHOLD)
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

void initModules()
{
	S.println("\n=== Initializing ===");

	if (!(initRTC() && initBME280() && initSDCard()))
	{
		delay(1000);
		initModules();
	}
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
#define HEADER "Time,Soil moisture in %,Air humidity in %,Temperature in C"
		file.println(HEADER);
		file.close();

		S.println("Done\n>>> " HEADER);
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
