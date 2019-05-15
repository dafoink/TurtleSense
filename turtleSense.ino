#include <LowPower.h>
#include <Wire.h>
#include <SD.h>
#include <RTClib.h>
#include <DallasTemperature.h>
#include <SparkFun_ADXL345.h>
#include <SDConfigFile.h>

// realtime clock
RTC_DS1307 rtc;

// Dallas Temperature
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);				// Setup a oneWire instance to communicate with any OneWire device
DallasTemperature tempSensors(&oneWire);	// Pass oneWire reference to DallasTemperature library

File logFile;

int x, y, z;
float tempC, tempF;

struct parameters {
	boolean doSleep = false;
	int adxlRangeSettings = 16;
	int adxlActivityX = 1;
	int adxlActivityY = 0;
	int adxlActivityZ = 0;
	int adxlActivityThreshold = 75;
} settings;


/***********************************************/
const int chipSelect = 10;
ADXL345 adxl = ADXL345();					// USE FOR I2C COMMUNICATION

DateTime now;

void setup() 
{
	Serial.begin(115200);						// Start the serial terminal
	delay(3000);
	Serial.println("starting");
	setupSDCard();							// setup SD Card
	readConfiguration();
	setupRTC();								// setup real-time clock
	setupTempSensors();						// setup 1-wire temperature sensors
	setupADXL();							// setup accelerometer

	writeHeader();
}

void loop() 
{
	getTime();
	getTemperature();
	//sleepIfNeeded();
	readAccelerometer();
}

// read accelerometer into global variables.  also call triggered action method
void readAccelerometer()
{
	// Accelerometer Readings
	adxl.readAccel(&x, &y, &z);				// Read the accelerometer values and store them in variables declared above x,y,z

	ADXL_ISR();
}


/********************* ISR *********************/
/* Look for Interrupts and Triggered Action    */
void ADXL_ISR() 
{
	// getInterruptSource clears all triggered actions after returning value
	// Do not call again until you need to recheck for triggered actions
	byte interrupts = adxl.getInterruptSource();

	// Free Fall Detection
	if (adxl.triggered(interrupts, ADXL345_FREE_FALL)) 
	{
		//add code here to do when free fall is sensed
	}

	// Inactivity
	if (adxl.triggered(interrupts, ADXL345_INACTIVITY)) 
	{
		//add code here to do when inactivity is sensed
	}

	// Activity
	if (adxl.triggered(interrupts, ADXL345_ACTIVITY)) 
	{
		//add code here to do when activity is sensed
	}

	// Double Tap Detection
	if (adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)) 
	{
		//add code here to do when a 2X tap is sensed
	}

	// Tap Detection
	if (adxl.triggered(interrupts, ADXL345_SINGLE_TAP)) 
	{
		writeSensorData("TAP");

	}
}

// setup realtime clock
void setupRTC()
{
	if (!rtc.begin()) 
	{
		Serial.println("Couldn't find RTC");
		while (1);
	}
	rtc.adjust(DateTime(2019, 5, 9, 12, 56, 0));
	if (!rtc.isrunning()) 
	{
		Serial.println("RTC is NOT running!");
		// following line sets the RTC to the date & time this sketch was compiled
		// rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
		// This line sets the RTC with an explicit date & time, for example to set
		// January 21, 2014 at 3am you would call:
		rtc.adjust(DateTime(2019, 5, 10, 10, 25, 0));
	}
	now = rtc.now();
}

// initialize SD card drive
void setupSDCard()
{
	Serial.print("Initializing SD card...");
	if (!SD.begin(10)) {
		Serial.println("SD initialization failed!");
		return;
	}
	Serial.println("Done initializing SD card.");
}

// setup temperature sensor
void setupTempSensors()
{
	tempSensors.begin();
}

// setup accelerometer
void setupADXL()
{
	adxl.powerOn();                     // Power on the ADXL345

	adxl.setRangeSetting(settings.adxlRangeSettings);           // Give the range settings
										// Accepted values are 2g, 4g, 8g or 16g
										// Higher Values = Wider Measurement Range
										// Lower Values = Greater Sensitivity

	adxl.setSpiBit(0);                  // Configure the device to be in 4 wire SPI mode when set to '0' or 3 wire SPI mode when set to 1
										// Default: Set to 1
										// SPI pins on the ATMega328: 11, 12 and 13 as reference in SPI Library 

	adxl.setActivityXYZ(settings.adxlActivityX, settings.adxlActivityY, settings.adxlActivityZ);       // Set to activate movement detection in the axes "adxl.setActivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
	adxl.setActivityThreshold(settings.adxlActivityThreshold);      // 62.5mg per increment   // Set activity   // Inactivity thresholds (0-255)

	adxl.setInactivityXYZ(1, 0, 0);     // Set to detect inactivity in all the axes "adxl.setInactivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
	adxl.setInactivityThreshold(75);    // 62.5mg per increment   // Set inactivity // Inactivity thresholds (0-255)
	adxl.setTimeInactivity(10);         // How many seconds of no activity is inactive?

	adxl.setTapDetectionOnXYZ(0, 0, 1); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)

	// Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
	adxl.setTapThreshold(10);           // 62.5 mg per increment
	adxl.setTapDuration(15);            // 625 μs per increment
	adxl.setDoubleTapLatency(80);       // 1.25 ms per increment
	adxl.setDoubleTapWindow(200);       // 1.25 ms per increment

	// Set values for what is considered FREE FALL (0-255)
	adxl.setFreeFallThreshold(7);       // (5 - 9) recommended - 62.5mg per increment
	adxl.setFreeFallDuration(30);       // (20 - 70) recommended - 5ms per increment

	// Setting all interupts to take place on INT1 pin
	//adxl.setImportantInterruptMapping(1, 1, 1, 1, 1);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);" 
														  // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
														  // This library may have a problem using INT2 pin. Default to INT1 pin.

	// Turn on Interrupts for each mode (1 == ON, 0 == OFF)
	adxl.InactivityINT(1);
	adxl.ActivityINT(1);
	adxl.FreeFallINT(1);
	adxl.doubleTapINT(1);
	adxl.singleTapINT(1);

	//attachInterrupt(digitalPinToInterrupt(interruptPin), ADXL_ISR, RISING);   // Attach Interrupt
}

// write out sensor data to SD card file.
void writeSensorData(char* eventName)
{
	logFile = SD.open("data.txt", FILE_WRITE);
	if (logFile) 
	{
		logFile.print(eventName);
		logFile.print(",");
		logFile.print(x);
		logFile.print(",");
		logFile.print(y);
		logFile.print(",");
		logFile.print(z);
		logFile.print(",");
		logFile.print(tempC);
		logFile.print(",");
		logFile.print(tempF);
		logFile.print(",");
		logFile.print(now.year(), DEC);
		logFile.print('/');
		logFile.print(now.month(), DEC);
		logFile.print('/');
		logFile.print(now.day(), DEC);
		logFile.print(' ');
		logFile.print(now.hour(), DEC);
		logFile.print(':');
		logFile.print(now.minute(), DEC);
		logFile.print(':');
		logFile.print(now.second(), DEC);
		logFile.println();
		Serial.println("finished writing to data.txt");
		logFile.close();
	}
	else 
	{
		Serial.println("error opening data.txt");
	}

}

// write the header of the output file.
void writeHeader()
{
	DateTime future(now + TimeSpan(7, 12, 30, 6));

	logFile = SD.open("data.txt", FILE_WRITE);

	if (logFile) 
	{
		logFile.println("eventName,x,y,z,tempC,tempF,dateTime");
		logFile.close();
	}
	else 
	{
		Serial.println("error opening data.txt");
	}
}

// used to read configuration file
bool readConfiguration()
{
	const uint8_t CONFIG_LINE_LENGTH = 35;

	// The open configuration file.
	SDConfigFile cfg;

	// Open the configuration file.
	if (!cfg.begin("CONFIG.TXT", CONFIG_LINE_LENGTH)) {
		Serial.print("Failed to open configuration file: ");
		Serial.println("CONFIG.TXT");
		return false;
	}

	// Read each setting from the file.
	while (cfg.readNextSetting()) {
		if (cfg.nameIs("doSleep")) {
			settings.doSleep = cfg.getBooleanValue();
		}
		if (cfg.nameIs("adxlRangeSettings")) {
			settings.adxlRangeSettings = cfg.getIntValue();
		}
		if (cfg.nameIs("adxlActivityX")) {
			settings.adxlActivityX = cfg.getIntValue();
		}
		if (cfg.nameIs("adxlActivityY")) {
			settings.adxlActivityY = cfg.getIntValue();
		}
		if (cfg.nameIs("adxlActivityZ")) {
			settings.adxlActivityY = cfg.getIntValue();
		}
	}
}


// get current time and store in global variable: now
void getTime()
{
	now = rtc.now();

	Serial.print(now.year(), DEC);
	Serial.print('/');
	Serial.print(now.month(), DEC);
	Serial.print('/');
	Serial.print(now.day(), DEC);
	Serial.print(' ');
	Serial.print(now.hour(), DEC);
	Serial.print(':');
	Serial.print(now.minute(), DEC);
	Serial.print(':');
	Serial.print(now.second(), DEC);
	Serial.println();
}

// will get called and put device to sleep if settings say it should
void sleepIfNeeded()
{
	if(settings.doSleep)
	{
		////sleep routine here.  TODO:  needs some work
		Serial.println("Going to Sleep");
		LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
		Serial.print("I'm awake now!");
		Serial.println();
	}
}


// get the current temperature
void getTemperature()
{
	tempSensors.requestTemperatures();
	tempC = (tempSensors.getTempCByIndex(0));
	tempF = (tempSensors.getTempCByIndex(0) * 9.0) / 5.0 + 32.0;
}
