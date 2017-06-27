/* Jar of Primes - Persistence
 *  
 * Code to load and store persistent settings. We store stuff in SPIFFS
 * to piggy-back on its wear-leveling. 
 *  
 * Marian Aldenhövel <marian.aldenhoevel@marian-aldenhoevel.de>
 * Published under the MIT license. See license.html or 
 * https://opensource.org/licenses/MIT
*/

const String NUMBER_FILE = "/var/number";
const String SETUP_FILE = "/var/wifi";

// Debug: Output a list of all the files stored in SPIFFS.
void dumpFileSystem() {
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("  %s %d bytes\n", fileName.c_str(), fileSize);
  }
}

void reportError(String filename, String operation, String mode) {
  Serial.printf("SPIFFS ERROR: %s file %s in mode %s.\n", operation.c_str(), filename.c_str(), mode.c_str());
}

void writeNumberToSPIFFS(int number) {
  // Serial.printf("writeNumberToSPIFFS()\n");

  File numberfile = SPIFFS.open(NUMBER_FILE, "w");
  if (!numberfile) {
    reportError(NUMBER_FILE, "open()", "w");
  } else {
    numberfile.print(number);
    numberfile.close();
    // Serial.printf("writeNumberToSPIFFS() - done\n");
  }
}

int readNumberFromSPIFFS() {
  // Serial.printf("readNumberFromSPIFFS()\n");

  File numberfile = SPIFFS.open(NUMBER_FILE, "r");
  if (!numberfile) {
    reportError(NUMBER_FILE, "open()", "r");
    return 0;
  } else {
    int result = numberfile.parseInt();
    // Serial.printf("readNumberFromSPIFFS() - %d\n", result);

    numberfile.close();
    return result;
  }
}

void writeSetupToSPIFFS() {
  // Serial.printf("writeSetupToSPIFFS()\n");

  File setupfile = SPIFFS.open(SETUP_FILE, "w");
  if (!setupfile) {
    reportError(SETUP_FILE, "open()", "w");
  } else {
    setupfile.println(ssid);
    setupfile.println(passphrase);
    setupfile.close();
    // Serial.printf("writeSetupToSPIFFS() - done\n");
  }
}

void readSetupFromSPIFFS() {
  // Serial.printf("readSetupFromSPIFFS()\n");

  File setupfile = SPIFFS.open(SETUP_FILE, "r");
  if (!setupfile) {
    reportError(SETUP_FILE, "open()", "r");
  } else {
    ssid = setupfile.readStringUntil('\n'); ssid.trim();
    passphrase = setupfile.readStringUntil('\n'); passphrase.trim();

    /*
    Serial.printf("readSetupFromSPIFFS() - ssid=\"%s\" (%d)\n", ssid.c_str(), ssid.length());
    Serial.printf("readSetupFromSPIFFS() - passphrase=\"%s\" (%d)\n", passphrase.c_str(), passphrase.length());
    */
  
    setupfile.close();
  }
}

void setupPersistence() {
  // Set up filesystem.
  if (SPIFFS.begin()) {
    Serial.printf("SPIFFS initialized.\n");
  } else {
    Serial.printf("Error initializing SPIFFS.\n");
  }
  
  // dumpFileSystem();
  
  Serial.printf("\n");
}

