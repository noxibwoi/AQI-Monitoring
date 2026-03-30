#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BUTTON_SENS 2
#define BUTTON_MOD 3
#define DHTPIN 4
#define DHTTYPE DHT11
#define GASPIN A0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHTPIN, DHTTYPE);

bool sensorError = false;
bool buttonSensFlag = false;
bool buttonDataFlag = false;
int sensMod = 0;
int dataMod = 0;
unsigned long lastUpdate = 0;

uint8_t graph[64];
uint8_t graphTemp[64];
uint8_t graphGas[64];

float temp;
float humid;
int gasVal;

void setup() {
    Serial.begin(9600);
    dht.begin();

    pinMode(BUTTON_SENS, INPUT_PULLUP);
    pinMode(BUTTON_MOD, INPUT_PULLUP);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED missing");
        while (1);
    }

    // wait for DHT11 and get first reading
    delay(2000);
    temp = dht.readTemperature();
    humid = dht.readHumidity();

    // average gas reading
    int gasSum = 0;
    for (int i = 0; i < 8; i++) {
        gasSum += analogRead(GASPIN);
        delay(1);
    }
    gasVal = gasSum / 8;

    if (isnan(temp) || isnan(humid)) {
        sensorError = true;
        temp = 0;
        humid = 0;
    }

    // initialize all graphs to actual first values
    uint8_t initHum  = (uint8_t)constrain(map((int)humid, 0, 100,  62, 18), 18, 62);
    uint8_t initTemp = (uint8_t)constrain(map((int)temp,  0, 50,   62, 18), 18, 62);
    uint8_t initGas  = (uint8_t)constrain(map(gasVal,     0, 1023, 62, 18), 18, 62);

    for (int i = 0; i < 64; i++) graph[i]     = initHum;
    for (int i = 0; i < 64; i++) graphTemp[i] = initTemp;
    for (int i = 0; i < 64; i++) graphGas[i]  = initGas;

    updateDisplay();
}

void loop() {
    unsigned long now = millis();

    if (now - lastUpdate >= 2000) {
        lastUpdate = now;

        temp = dht.readTemperature();
        humid = dht.readHumidity();

        // average gas reading
        int gasSum = 0;
        for (int i = 0; i < 8; i++) {
            gasSum += analogRead(GASPIN);
            delay(1);
        }
        gasVal = gasSum / 8;

        if (isnan(temp) || isnan(humid)) {
            sensorError = true;
        } else {
            sensorError = false;
        }

        // shift and update all graphs
        for (int i = 0; i < 63; i++) graph[i]     = graph[i + 1];
        for (int i = 0; i < 63; i++) graphTemp[i] = graphTemp[i + 1];
        for (int i = 0; i < 63; i++) graphGas[i]  = graphGas[i + 1];

        graph[63]     = (uint8_t)constrain(map((int)humid, 0, 100,  62, 18), 18, 62);
        graphTemp[63] = (uint8_t)constrain(map((int)temp,  0, 50,   62, 18), 18, 62);
        graphGas[63]  = (uint8_t)constrain(map(gasVal,     0, 1023, 62, 18), 18, 62);

        updateDisplay();
    }

    bool buttonStateSens = digitalRead(BUTTON_SENS);
    bool buttonStateData = digitalRead(BUTTON_MOD);

    if (buttonStateSens == LOW && !buttonSensFlag) {
        sensMod++;
        if (sensMod > 2) sensMod = 0;
        buttonSensFlag = true;
        updateDisplay();
    }
    if (buttonStateSens == HIGH) buttonSensFlag = false;

    if (buttonStateData == LOW && !buttonDataFlag) {
        dataMod++;
        if (dataMod > 1) dataMod = 0;
        buttonDataFlag = true;
        updateDisplay();
    }
    if (buttonStateData == HIGH) buttonDataFlag = false;
}

void updateDisplay() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    if (sensorError) {
        display.setCursor(0, 0);
        display.println("DHT11 Sensor Error :(");
        display.display();
        return;
    }

    // sensMod 0 — All sensors
    if (sensMod == 0) {
        display.setCursor(0, 0);
        display.println("All");
        if (dataMod == 0) {
            display.setCursor(0, 20);
            display.print("Temp: ");
            display.print(temp, 1);
            display.println(" C");
            display.setCursor(0, 30);
            display.print("Humid: ");
            display.print(humid, 1);
            display.println(" %");
            display.setCursor(0, 40);
            display.print("Gas: ");
            display.println(gasVal);
        } else {
            for (int x = 1; x < 64; x++) {
                display.drawLine((x-1)*2, graph[x-1],     x*2, graph[x],     WHITE);
                display.drawLine((x-1)*2, graphTemp[x-1], x*2, graphTemp[x], WHITE);
                display.drawLine((x-1)*2, graphGas[x-1],  x*2, graphGas[x],  WHITE);
            }
            int humY  = constrain((int)graph[63]     - 4, 18, 54);
            int tempY = constrain((int)graphTemp[63] - 4, 18, 54);
            int gasY  = constrain((int)graphGas[63]  - 4, 18, 54);

            if (abs(tempY - humY)  < 10) tempY = humY  + 10;
            if (abs(gasY  - tempY) < 10) gasY  = tempY + 10;
            if (abs(gasY  - humY)  < 10) gasY  = humY  + 10;

            tempY = constrain(tempY, 18, 54);
            gasY  = constrain(gasY,  18, 54);

            display.setCursor(100, humY);
            display.print((int)humid);
            display.print("%");

            display.setCursor(100, tempY);
            display.print((int)temp);
            display.print("C");

            display.setCursor(100, gasY);
            display.print(gasVal);
        }
    }

    // sensMod 1 — Gas sensor
    else if (sensMod == 1) {
        display.setCursor(0, 0);
        display.print("Gas ");
        display.println(gasVal);
        if (dataMod == 0) {
            display.setCursor(0, 20);
            display.print("Raw: ");
            display.println(gasVal);
            display.setCursor(0, 30);
            display.print("Voltage: ");
            display.print(gasVal * (5.0 / 1023.0), 2);
            display.println("V");
        } else {
            for (int x = 1; x < 64; x++) {
                display.drawLine((x-1)*2, graphGas[x-1], x*2, graphGas[x], WHITE);
            }
            int gasY = constrain((int)graphGas[63] - 4, 18, 54);
            display.setCursor(100, gasY);
            display.println(gasVal);
        }
    }

    // sensMod 2 — Temperature + Humidity
    else if (sensMod == 2) {
        display.setCursor(0, 0);
        display.println("Temp+Humid");
        if (dataMod == 0) {
            display.setCursor(0, 20);
            display.print("Temp: ");
            display.print(temp, 1);
            display.println(" C");
            display.setCursor(0, 30);
            display.print("Humid: ");
            display.print(humid, 1);
            display.println(" %");
            int barWidth = map(humid, 0, 100, 0, SCREEN_WIDTH);
            display.drawRect(0, 56, SCREEN_WIDTH, 8, WHITE);
            display.fillRect(0, 56, barWidth, 8, WHITE);
        } else {
            for (int x = 1; x < 64; x++) {
                display.drawLine((x-1)*2, graph[x-1],     x*2, graph[x],     WHITE);
                display.drawLine((x-1)*2, graphTemp[x-1], x*2, graphTemp[x], WHITE);
            }
            int humY  = constrain((int)graph[63]     - 4, 18, 54);
            int tempY = constrain((int)graphTemp[63] - 4, 18, 54);
            if (abs(tempY - humY) < 10) tempY = humY + 10;
            tempY = constrain(tempY, 18, 54);

            display.setCursor(100, humY);
            display.print((int)humid);
            display.println("%");

            display.setCursor(100, tempY);
            display.print((int)temp);
            display.println("C");
        }
    }

    display.display();
}
