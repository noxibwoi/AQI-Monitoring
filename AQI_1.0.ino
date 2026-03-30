#include <Wire.h>                       // I2C communication library
#include <Adafruit_GFX.h>               // graphics library
#include <Adafruit_SSD1306.h>           // OLED Display Driver library
#include <DHT.h>                        // DHT library

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BUTTON_SENS 2
#define BUTTON_MOD 3
#define DHTPIN 4
#define DHTTYPE DHT11

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);       // creating OLED display object
DHT dht(DHTPIN, DHTTYPE);

bool sensorError = false;
bool buttonSensFlag = false;
bool buttonDataFlag = false;
int sensMod = 0;
int dataMod = 0;
unsigned long lastUpdate = 0;
const int interval = 1000;
int graph[128] = {0};

float temp;
float humid;

void setup() {
    Serial.begin(9600);
    dht.begin();

    pinMode(BUTTON_SENS, INPUT_PULLUP);
    pinMode(BUTTON_MOD, INPUT_PULLUP);

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED missing");
        while(1);
    }
    updateDisplay();
}

void loop() {
    temp = dht.readTemperature();
    humid = dht.readHumidity();

    if (isnan(temp) || isnan(humid)) {
        sensorError = true;
    } else {
        sensorError = false;
    }

    bool buttonStateSens = digitalRead(BUTTON_SENS);
    bool buttonStateData = digitalRead(BUTTON_MOD);

    if (buttonStateSens == LOW && !buttonSensFlag) {
        sensMod++;
        if (sensMod > 2) sensMod = 0;
        buttonSensFlag = true;

        updateDisplay();
    }
    if (buttonStateSens == HIGH){
        buttonSensFlag = false;
    }

    if (buttonStateData == LOW && !buttonDataFlag) {
        dataMod++;
        if (dataMod > 1) dataMod = 0;
        buttonDataFlag = true;

        updateDisplay();
    }
    if (buttonStateData == HIGH){
        buttonDataFlag = false;
    }

        updateDisplay();
}

void updateDisplay() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    if (sensorError) {
        display.clearDisplay();
        display.setCursor(0, 20);
        display.setTextSize(1);
        display.println("DHT11 Sensor Error :(");
        display.display();
        return;
    }

    if (sensMod == 0) {            
        display.setCursor(0, 0);
        display.println("All");
        if (dataMod == 0) {
            display.setCursor(0, 40);
            display.print("Temp: ");
            display.print(temp);
            display.println(" C");

            display.setCursor(0, 50);
            display.print("Humidity: ");
            display.print(humid);
            display.println(" %");
        }
        else drawGraph();
    }

    else if (sensMod == 1) {
        display.setCursor(0, 0);
        display.println("Gas");
        if (dataMod == 0) {

        }
        else drawGraph();
    }

    else if (sensMod == 2) {
        display.setCursor(0, 0);
        display.println("Temperature");
        if (dataMod == 0) {
            display.setCursor(0, 20);
            display.print("Temp: ");
            display.print(temp);
            display.println(" C");

            display.setCursor(0, 30);
            display.print("Humidity: ");
            display.print(humid);
            display.println(" %");
            
            int barWidth = map(humid, 0, 100, 0, SCREEN_WIDTH);
            display.drawRect(0, 54, SCREEN_WIDTH, 10, WHITE);
            display.fillRect(0, 54, barWidth, 10, WHITE);
        }
        else drawGraph();
    }

    display.display();
}

void drawGraph(){
    display.setCursor(0, 25);
    display.println("Graph");
}