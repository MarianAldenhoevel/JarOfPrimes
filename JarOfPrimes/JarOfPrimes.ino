/* Jar of Primes
 *  
 * This is the code to drive a knock-off version of the Primer,
 * a kinetic sculpture by Karl Lautman. For the original see:
 * 
 *   http://www.karllautman.com/primer.html
 *   https://www.youtube.com/watch?v=8UqCyepX3AI
 * 
 * The program is written for a NodeMCU board. Additional 
 * hardware is a push-button connected to GND and a physical 
 * counter on a digital output.
 * 
 * We also use both the LED on the ESP-12 and the one on the
 * NodeMCU as status indicators. 
 *
 * The (blue) LED on the ESP blinks
 * while connecting to an access point in station mode and lights
 * when a client request is being served.
 *
 * The (red) LED on the NodeMCU board blinks when the Jar is ready
 * and working.
 * s
 * Marian Aldenhövel <marian.aldenhoevel@marian-aldenhoevel.de>
 * Published under the MIT license. See license.html or 
 * https://opensource.org/licenses/MIT
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <Ticker.h>

// #define UNIT_TESTS

// ***************************************************************************
// Setup:
// ***************************************************************************

// What is the largest number the counter can display before it wraps?
const int MAXCOUNTER = 9999999;

// There are two LEDs on my NodeMCU board, a blue one on the ESP module
// itself and a red one as part of the added NodeMCU hardware. Declare constants
// for each pin and then select LED_PIN for the rest of the code to use. 
const int LED_PIN_ESP = 2; 
const int LED_PIN_NODEMCU = 16; 

// Output Pin the counter is connected to. We want to pull it down and set
// if HIGH for pulsing the counter.
const int COUNTER_PIN = 4; 

// Configure the pin the push-button is connected to.
const int BUTTON_PIN = 5;

// The debounce-delay for the pushbutton in milliseconds. Choose long
// enough to filter any glitches and bounces, but not significantly longer 
// than that.
const unsigned long BUTTON_DEBOUNCE_DELAY = 50;

// Blink-interval of the LED that indicates alive in milliseconds.
const long LED_BLINK_INTERVAL = 1000;       

// Max counting frequency given as minimum delay between steps 
// in milliseconds.
const long COUNTER_DELAY = 1000/5; // 5Hz

// How long does the relay stay activated to advance the counter?
const unsigned long RELAY_ON_MILLIS = 25; // 20Hz max frequency at 50% duty-cycle.

// ***************************************************************************
// Global variables:
// ***************************************************************************

// SSID and passphrase of an AP to connect to as client. If these are left
// empty, or the button is held down at startup the jar will instead
// create an AP ad-hoc to allow these settings to be changed.
String ssid = "";
String passphrase = "";

// Global variables to command a step to the next number and to
// limit counting-frequency.
bool commandNextNumber = false;
unsigned long nextNumberAfter = 0;

// Global variables to debounce the button and track its state.
int btnStateDebouncing; 
int btnStateDebounced;   
unsigned long btnLastBounceTime = 0;  // the last time the output pin was toggled

// Ticker object for the alive-indication.
Ticker alive;

// Current number displayed in the counter display. Initialization here is 
// unimportant. In actual operation the value is read from SPIFFS (and stored
// on each change) and set externally via the sync-operation on the WebAPI.
int currentNumber = 13;

// Time after which we will turn off the relay again. Used to create pulses
// of minimum width without calling delay().
unsigned long RelayOffAfter = 0;

// Handles blinking of the "I am alive" LED. Called from a Ticker object.
void toggleLED() {  
  digitalWrite(LED_PIN_NODEMCU, !digitalRead(LED_PIN_NODEMCU));     
}

// Handles debouncing of the button. Eventually decides that the button has
// physically changed state from up to down or vice versa.
void checkButton() {
  // Read instant state of the button.
  int btnStateInstant = digitalRead(BUTTON_PIN);

  // Has the state changed with respect to the last instant 
  // reading?
  if (btnStateInstant != btnStateDebouncing) {
    // Reset the debouncing timer, we'll wait for it to settle again.
    btnLastBounceTime = millis();
  }

  if ((millis() - btnLastBounceTime) > BUTTON_DEBOUNCE_DELAY) {
    // Whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // Record new current state and create events if it changed.
    if (btnStateInstant != btnStateDebounced) {
      btnStateDebounced = btnStateInstant;
      if (btnStateDebounced == LOW) {
        Serial.printf("Button pressed\n");
        commandNextNumber = true;
      } else {
        // Serial.printf("Button released\n");
      }
    }
  }

  btnStateDebouncing = btnStateInstant;
}

// Energize the relay and record the time at which we want to deenergize it
// again.
void setRelayOn() {
  // Serial.printf("Relay ON\n");
  digitalWrite(COUNTER_PIN, HIGH);
  RelayOffAfter = millis() + RELAY_ON_MILLIS;
}

// De-energize the relay if the pulse has been on for long enough.
void checkRelayOff() {
  if (RelayOffAfter && (millis() >= RelayOffAfter)) {
    // Serial.printf("Relay OFF\n");
    digitalWrite(COUNTER_PIN, LOW);
    RelayOffAfter = 0;
  }
}

// Brute-force to check wether a candidate number is prime.
bool isPrime(int candidate) {
  if (candidate < 2) {
    return false;
  } else if (candidate == 2) {
    return true;
  } else if (candidate == 8648640) {
    return true;
  } else if ((candidate % 2) == 0) {
    return false;
  } else {  
    int testto = sqrt(candidate);
    for (int divisor = 3; divisor <= testto; divisor = divisor+2){
      if ((candidate % divisor) == 0) {
        return false;
      }
    }
    return true;
  } 
}

#ifdef UNIT_TESTS
  int numberOfTests = 0;
  int numberOfFailures = 0;
  
  void test_isPrimeTestCase(int candidate, bool expect) {
    numberOfTests++;
    
    Serial.printf("    %d expected %s ", candidate, expect ? "true" : "false");

    unsigned long startmicros = micros();
    bool result = isPrime(candidate);
    unsigned long endmicros = micros();
    
    Serial.printf("(%dµs) got %s ", endmicros - startmicros, result ? "true" : "false");
    if (result == expect) {
      Serial.printf("PASS\n");
    } else {
      numberOfFailures++;
      Serial.printf("FAIL\n");
    }
  }
  
  void test_isPrime() {
    Serial.printf("  Testing isPrime()\n");
    
    test_isPrimeTestCase(0, false);
    test_isPrimeTestCase(1, false);
    test_isPrimeTestCase(2, true);
    test_isPrimeTestCase(3, true);
    test_isPrimeTestCase(4, false);
    test_isPrimeTestCase(11, true);
    test_isPrimeTestCase(8648640, true);
    test_isPrimeTestCase(9999990, false);
    test_isPrimeTestCase(9999991, true);
    test_isPrimeTestCase(9999992, false);
    test_isPrimeTestCase(9999999, false);
    
    Serial.printf("  Tests for isPrime() completed.\n");
  }

  void executeUnitTests() {
    Serial.printf("Unit Testing\n");
    test_isPrime();
    Serial.printf("%d tests completed, %d failures.\n\n", numberOfTests, numberOfFailures);
  }
#endif

// Service the command to step to the next number.
void nextNumber() {
  // Reset command to step the counter.
  commandNextNumber = false;

  // Limit counting-frequency.
  nextNumberAfter = millis() + COUNTER_DELAY;

  // Step the physical counter. It will turn off after an appropriate delay.
  setRelayOn();
  
  // Step currentNumber, wrap around if needed.
  currentNumber = currentNumber + 1;
  if (currentNumber > MAXCOUNTER) {
    currentNumber = 0;
  }

  // Announce new number to connected network clients. 
  broadcastNumber(currentNumber);
  
  // If the new currentNumber fails the primality-test command
  // another step. If it passes store the new number persistently,
  // We hope we do not lose power while counting up to the next
  // prime.
  if (!isPrime(currentNumber)) {
    Serial.printf("[%d]\n", currentNumber);
    commandNextNumber = true;
  } else {
    Serial.printf("[%d] is prime\n", currentNumber);
    writeNumberToSPIFFS(currentNumber);
  }
}

void checkNextNumber() {
  if (commandNextNumber && (millis() >= nextNumberAfter)) {
    nextNumber();
  }
}

void setup() {
  // Setup serial comms for logging.
  Serial.begin(115200);
  Serial.printf("\nJar of Primes reporting for duty!\n\n");
  
  // Serial.setDebugOutput(true);

  #ifdef UNIT_TESTS
    executeUnitTests();
  #endif

  // Setup LED-pin to output, pull LOW.
  pinMode(LED_PIN_NODEMCU, OUTPUT);
  digitalWrite(LED_PIN_NODEMCU, LOW);
  
  setupPersistence();
  
  Serial.printf("Restoring settings from persistent memory...\n");
  readSetupFromSPIFFS();
  currentNumber = readNumberFromSPIFFS();
  Serial.printf("\n");

  // Setup Relay-pin.
  pinMode(COUNTER_PIN, OUTPUT);
  digitalWrite(COUNTER_PIN, LOW);
   
  // Setup Button-pin.
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  btnStateDebouncing = digitalRead(BUTTON_PIN);
  btnStateDebounced = btnStateDebouncing;
  if (btnStateDebounced == LOW) {
    Serial.printf("Button initial state is DOWN.\n\n");
  } else {
    Serial.printf("Button initial state is UP.\n\n");
  } 
  
  setupNetwork((btnStateDebounced == LOW));

  // Enable ticker to blink the "am alive" LED.
  alive.attach_ms(LED_BLINK_INTERVAL, toggleLED);

  Serial.printf("Setup finished, at your service.\n\n");
}

void loop() {
  loopNetwork();

  checkButton();
  checkRelayOff();
  checkNextNumber();
}

