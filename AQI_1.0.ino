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
#define RL 1.0
#define R0 35.0
#define WARMUP_TIME 5000
#define LED_READY 5   
#define LED_ALERT 6 
#define BUZZER 7  

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHTPIN, DHTTYPE);

bool sensorError = false;
bool buttonSensFlag = false;
bool buttonDataFlag = false;
int sensMod = 0;
int dataMod = 0;
unsigned long lastUpdate = 0;
unsigned long startTime = 0;
bool warmupDone = false;
unsigned long lastBeep = 0;
bool beepState = false;

uint8_t graph[64];
uint8_t graphTemp[64];
uint8_t graphGas[64];

float temp;
float humid;
float co2ppm;

float readCO2() {
    int gasSum = 0;
    for (int i = 0; i < 8; i++) {
        gasSum += analogRead(GASPIN);
        delay(1);
    }
    int raw = gasSum / 8;
    float voltage = raw * (5.0 / 1023.0);
    float RS = ((5.0 - voltage) / voltage) * RL;
    float ratio = RS / R0;
    return 110.47 * pow(ratio, -2.862);
}

void setup() {
    Serial.begin(9600);
    Serial.println(F("Started"));
    dht.begin();

    pinMode(BUTTON_SENS, INPUT_PULLUP);
    pinMode(BUTTON_MOD, INPUT_PULLUP);
    pinMode(LED_READY, OUTPUT);
    pinMode(LED_ALERT, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED_READY, LOW);
    digitalWrite(LED_ALERT, LOW);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("OLED missing"));
        while (1);
    }
    delay(250);
    display.clearDisplay();
    display.display();

    delay(2000);
    temp = dht.readTemperature();
    humid = dht.readHumidity();
    if (isnan(temp) || isnan(humid)) {
        sensorError = true;
        temp = 0;
        humid = 0;
    }

    co2ppm = 0;

    uint8_t initHum  = (uint8_t)constrain(map((int)humid, 0, 100, 62, 18), 18, 62);
    uint8_t initTemp = (uint8_t)constrain(map((int)temp,  0, 50,  62, 18), 18, 62);

    for (int i = 0; i < 64; i++) graph[i]     = initHum;
    for (int i = 0; i < 64; i++) graphTemp[i] = initTemp;
    for (int i = 0; i < 64; i++) graphGas[i]  = 40;

    startTime = millis();
    warmupDone = false;

    updateDisplay();
}

void loop() {
    unsigned long now = millis();

    if (!warmupDone && now - startTime >= WARMUP_TIME) {
        warmupDone = true;
        co2ppm = readCO2();
        uint8_t initGas = (uint8_t)constrain(map((int)co2ppm, 0, 2000, 62, 18), 18, 62);
        for (int i = 0; i < 64; i++) graphGas[i] = initGas;
        Serial.print(F("Warmup done. CO2: "));
        Serial.println(co2ppm);
        updateDisplay();
    }

    if (now - lastUpdate >= 1000) {
        lastUpdate = now;

        float newTemp = dht.readTemperature();
        float newHumid = dht.readHumidity();

        if (!isnan(newTemp) && !isnan(newHumid)) {
            sensorError = false;
            temp = newTemp;
            humid = newHumid;
        } else {
            sensorError = true;
        }

        digitalWrite(LED_READY, warmupDone ? HIGH : LOW);
        if (warmupDone) {
            co2ppm = readCO2();
            digitalWrite(LED_ALERT, int(co2ppm) > 2000? HIGH : LOW);
        }

        for (int i = 0; i < 63; i++) graph[i]     = graph[i + 1];
        for (int i = 0; i < 63; i++) graphTemp[i] = graphTemp[i + 1];
        for (int i = 0; i < 63; i++) graphGas[i]  = graphGas[i + 1];

        graph[63]     = (uint8_t)constrain(map((int)humid,  0, 100,  62, 18), 18, 62);
        graphTemp[63] = (uint8_t)constrain(map((int)temp,   0, 50,   62, 18), 18, 62);
        graphGas[63]  = warmupDone ? (uint8_t)constrain(map((int)co2ppm, 0, 2000, 62, 18), 18, 62) : 40;

        updateDisplay();
    }

    if (warmupDone) {
        if ((int)co2ppm > 2000) {
            if (beepState && now - lastBeep >= 200) {
                digitalWrite(BUZZER, LOW);
                beepState = false;
                lastBeep = now;
            } else if (!beepState && now - lastBeep >= 400) {
                digitalWrite(BUZZER, HIGH);
                beepState = true;
                lastBeep = now;
            }
        } else {
            digitalWrite(BUZZER, LOW);
            beepState = false;
        }
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

    if (!warmupDone) {
        int remaining = constrain((int)((WARMUP_TIME - (millis() - startTime)) / 1000), 0, 180);
        display.setCursor(90, 0);
        display.print(remaining);
        display.print(F("s"));
        int barWidth = map(remaining, 180, 0, 0, SCREEN_WIDTH);
        display.fillRect(0, 63, barWidth, 1, WHITE);
    }

    if (sensorError) {
        display.setCursor(0, 0);
        display.println(F("DHT11 Sensor Error :("));
        display.display();
        return;
    }

    if (sensMod == 0) {
        display.setCursor(0, 0);
        display.println(F("AQI Monitoring"));
        if (dataMod == 0) {
            display.setCursor(0, 20);
            display.print(F("Temp: "));
            display.print(temp, 1);
            display.println(F(" C"));
            display.setCursor(0, 30);
            display.print(F("Humid: "));
            display.print(humid, 1);
            display.println(F(" %"));
            display.setCursor(0, 40);
            display.print(F("CO2: "));
            if (warmupDone) {
                display.print((int)co2ppm);
                display.println(F(" PPM"));
            } else {
                display.println(F("warming..."));
            }
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
            display.print(F("%"));

            display.setCursor(100, tempY);
            display.print((int)temp);
            display.print(F("C"));

            display.setCursor(100, gasY);
            if (warmupDone) display.print((int)co2ppm);
            else display.print(F("--"));
        }
    }

    else if (sensMod == 1) {
        display.setCursor(0, 0);
        display.print(F("CO2 "));
        if (warmupDone) {
            display.print((int)co2ppm);
            display.println(F("PPM"));
        } else {
            display.println(F("warming..."));
        }
        if (dataMod == 0) {
            display.setCursor(0, 20);
            display.drawRect(0, 53, SCREEN_WIDTH, 8, WHITE);
            if (warmupDone) {
                display.print(F("CO2: "));
                display.print((int)co2ppm);
                display.println(F(" PPM"));
                display.setCursor(0, 30);
                if (co2ppm < 500)       display.println(F("Air: Good"));
                else if (co2ppm < 1000) display.println(F("Air: Moderate"));
                else if (co2ppm < 2000) display.println(F("Air: Poor"));
                else                    display.println(F("Air: Bad"));
                int barWidth = constrain(map((int)co2ppm, 0, 2000, 0, SCREEN_WIDTH), 0, SCREEN_WIDTH);
                display.fillRect(0, 53, barWidth, 8, WHITE);
            } else {
                display.println(F("Waiting for"));
                display.setCursor(0, 30);
                display.println(F("MQ135 warmup..."));
            }
        } else {
            for (int x = 1; x < 64; x++) {
                display.drawLine((x-1)*2, graphGas[x-1], x*2, graphGas[x], WHITE);
            }
            int gasY = constrain((int)graphGas[63] - 4, 18, 54);
            display.setCursor(100, gasY);
            if (warmupDone) display.print((int)co2ppm);
            else display.print(F("--"));
        }
    }

    else if (sensMod == 2) {
        display.setCursor(0, 0);
        display.println(F("Temp+Humid"));
        if (dataMod == 0) {
            display.setCursor(0, 20);
            display.print(F("Temp: "));
            display.print(temp, 1);
            display.println(F(" C"));
            display.setCursor(0, 30);
            display.print(F("Humid: "));
            display.print(humid, 1);
            display.println(F(" %"));
            int barWidth = map(humid, 0, 100, 0, SCREEN_WIDTH);
            display.drawRect(0, 53, SCREEN_WIDTH, 8, WHITE);
            display.fillRect(0, 53, barWidth, 8, WHITE);
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
            display.println(F("%H"));

            display.setCursor(100, tempY);
            display.print((int)temp);
            display.println(F("CT"));
        }
    }

    display.display();
}
