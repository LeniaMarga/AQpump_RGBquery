/*
  WiFi Web Server LED Blink

  A simple web server that lets you blink an LED via the web.
  This sketch will create a new access point (with no password).
  It will then launch a new server and print out the IP address
  to the Serial Monitor. From there, you can open that address in a web browser
  to turn on and off the LED on pin 13.

  If the IP address of your board is yourAddress:
	http://yourAddress/H turns the LED on
	http://yourAddress/L turns it off

  created 25 Nov 2012
  by Tom Igoe
  adapted to WiFi AP by Adafruit
 */

#include <SPI.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h" 
 ///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key index number (needed only for WEP)

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

int led = LED_BUILTIN;
int pumpPin = 6;
int valvePin = 7;
int neopixel = 8;
int led_count = 30;

int status = WL_IDLE_STATUS;
WiFiServer server(80);

Adafruit_NeoPixel strip(led_count, neopixel, NEO_GRB + NEO_KHZ800);


void setup() {
	//Initialize serial and wait for port to open:
	Serial.begin(9600);
	while (!Serial) {
		; // wait for serial port to connect. Needed for native USB port only
	}

	Serial.println("Access Point Web Server");

	pinMode(led, OUTPUT);      // set the LED pin mode
	pinMode(pumpPin, OUTPUT);
	pinMode(valvePin, OUTPUT);
	pinMode(neopixel, OUTPUT);

	// check for the WiFi module:
	if (WiFi.status() == WL_NO_MODULE) {
		Serial.println("Communication with WiFi module failed!");
		// don't continue
		while (true);
	}

	String fv = WiFi.firmwareVersion();
	if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
		Serial.println("Please upgrade the firmware");
	}

	// attempt to connect to WiFi network:
	while (status != WL_CONNECTED) {
		Serial.print("Attempting to connect to WPA SSID: ");
		Serial.println(ssid);
		// Connect to WPA/WPA2 network:
		status = WiFi.begin(ssid, pass);

		// wait 10 seconds for connection:
		delay(10000);
	}
	digitalWrite(led, HIGH);               // GET /H turns the LED on

// start the web server on port 80
	server.begin();

	// you're connected now, so print out the status
	printWiFiStatus();

	//neopixel strip
	strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
	strip.show();            // Turn OFF all pixels ASAP
	strip.setBrightness(100); // Set BRIGHTNESS to about 1/5 (max = 255)

}



void loop() {
	// compare the previous status to the current status
	if (status != WiFi.status()) {
		// it has changed update the variable
		status = WiFi.status();

		if (status == WL_AP_CONNECTED) {
			// a device has connected to the AP
			Serial.println("Device connected to AP");
		}
		else {
			// a device has disconnected from the AP, and we are back in listening mode
			Serial.println("Device disconnected from AP");
		}
	}

	WiFiClient client = server.available();   // listen for incoming clients

	if (client) {                             // if you get a client,
		Serial.println("new client");           // print a message out the serial port
		String currentLine = "";                // make a String to hold incoming data from the client
		while (client.connected()) {            // loop while the client's connected
			currentLine = "";
			while (client.connected())			// loop until line is read into currentLine
			{
				delayMicroseconds(10);                // This is required for the Arduino Nano RP2040 Connect - otherwise it will loop so fast that SPI will never be served.
				if (client.available()) {             // if there's bytes to read from the client,
					char c = client.read();             // read a byte, then
					Serial.write(c);                    // print it out the serial monitor
					if (c == '\n') {                    // if the byte is a newline character
						break; // break out of the while loop:
					}
					else if (c != '\r') {    // if you got anything else but a carriage return character,
						currentLine += c;      // add it to the end of the currentLine
					}
				}
			}

			if (currentLine.length() > 0) {
				// read query parameters
				byte r = 0;
				byte g = 0;
				byte b = 0;
				int queryIndex = currentLine.indexOf("?");
				while (queryIndex >= 0 && queryIndex < currentLine.length())
				{
          queryIndex++; // move to the parameter name
					int nextIndex = currentLine.indexOf("&", queryIndex);
					if (nextIndex < 0) nextIndex = currentLine.length();
					switch (currentLine[queryIndex])
					{
					case 'r': r = (byte)currentLine.substring(queryIndex+2, nextIndex).toInt();
					case 'g': g = (byte)currentLine.substring(queryIndex+2, nextIndex).toInt();
					case 'b': b = (byte)currentLine.substring(queryIndex+2, nextIndex).toInt();
					}
					queryIndex = nextIndex;
				}

				if (currentLine.startsWith("GET /inflate")) {
					digitalWrite(led, HIGH);       // GET /H turns the LED on
					digitalWrite(pumpPin, HIGH);   // turn the pump on
				}
				if (currentLine.startsWith("GET /deflate")) {
					digitalWrite(led, LOW);
					digitalWrite(pumpPin, LOW);
				}
				if (currentLine.startsWith("GET /stop")) {
					digitalWrite(led, LOW);
					digitalWrite(pumpPin, LOW);
					digitalWrite(valvePin, LOW); // allow no air to flow
				}

				if (queryIndex > 0)
				{
					colorWipe(strip.Color(r, g, b), 200); // Red neopixel
				}
			}
				else
				{
					// if the current line is blank, you got two newline characters in a row.
					// that's the end of the client HTTP request, so send a response:
							
					// HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
					// and a content-type so the client knows what's coming, then a blank line:
					client.println("HTTP/1.1 200 OK");
					client.println("Content-type: text/html");
					client.println("Access-Control-Allow-Origin: *");
					client.println();

					// the content of the HTTP response follows the header:
					client.print("Click <a href='/inflate'>here</a> inflate<br/>");
					client.print("Click <a href='/deflate'>here</a> deflate<br/>");
					client.print("Click <a href='/stop'>here</a> stop<br/>");
					client.print("<form action='/'>");
					client.print("R=<input type='text' name='r' /><br/>");
					client.print("G=<input type='text' name='g' /><br/>");
					client.print("B=<input type='text' name='b' /><br/>");
					client.print("<input type='submit' value='colorWipe' /></form>");

					// The HTTP response ends with another blank line:
					client.println();
          break;         
				}			
		}
		// close the connection:
		client.stop();
		Serial.println("client disconnected");
	}
}

void printWiFiStatus() {
	// print the SSID of the network you're attached to:
	Serial.print("SSID: ");
	Serial.println(WiFi.SSID());

	// print your WiFi shield's IP address:
	IPAddress ip = WiFi.localIP();
	Serial.print("IP Address: ");
	Serial.println(ip);

	// print where to go in a browser:
	Serial.print("To see this page in action, open a browser to http://");
	Serial.println(ip);

}

void colorWipe(uint32_t color, int wait) {
	for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
		strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
		strip.show();                          //  Update strip to match
		delay(wait);                           //  Pause for a moment
	}
}

void rainbow(int wait) {
	for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
		strip.rainbow(firstPixelHue);
		strip.show(); // Update strip with new contents
		delay(wait);  // Pause for a moment
	}
}
