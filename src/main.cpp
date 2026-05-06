#include <Arduino.h>
#include <LittleFS.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// --- Configurações de Wi-Fi ---
const char *ssid = "NOME_DA_SUA_REDE";
const char *password = "SENHA_DA_REDE";

// --- Definições de Hardware ---
#define DHTPIN 4
#define DHTTYPE DHT11

#define LED_OFF 13 
#define LED_AUTO 12
#define LED_ON 14

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// --- Instâncias ---
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
AsyncWebServer server(80);

// --- Variáveis de Controle ---
enum ModoOperacao { DESLIGADO = 0, AUTOMATICO = 1, LIGADO = 2 };
ModoOperacao modoAtual = DESLIGADO;

float temperatura = 0;
float umidade = 0;

// --- FUNÇÕES ---

void atualizarLEDs() {
  digitalWrite(LED_OFF, modoAtual == DESLIGADO);
  digitalWrite(LED_AUTO, modoAtual == AUTOMATICO);
  digitalWrite(LED_ON, modoAtual == LIGADO);
}

void atualizarDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("FCTE - EletronJun");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0, 15);
  display.print("Modo: ");
  if(modoAtual == DESLIGADO) display.println("DESLIGADO");
  else if(modoAtual == AUTOMATICO) display.println("AUTO");
  else display.println("LIGADO");

  if(modoAtual != DESLIGADO) {
    display.setTextSize(2);
    display.setCursor(0, 32);
    display.printf("%.1f C", temperatura);
    display.setCursor(0, 50);
    display.printf("%.1f %%", umidade);
  } else {
    display.setTextSize(1);
    display.setCursor(15, 40);
    display.println("SISTEMA INATIVO");
  }
  display.display();
}

void setup() {
  Serial.begin(115200);

  // Inicializa LEDs
  pinMode(LED_OFF, OUTPUT);
  pinMode(LED_AUTO, OUTPUT);
  pinMode(LED_ON, OUTPUT);

  // Inicializa Sensores e Display
  dht.begin();
  Wire.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("Falha no OLED");
  }

  // Inicializa o Sistema de Arquivos (LittleFS)
  if(!LittleFS.begin(true)){
    Serial.println("Erro ao montar o LittleFS");
  }

  atualizarLEDs();
  atualizarDisplay();

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado! IP: " + WiFi.localIP().toString());

  // --- ROTAS DO SERVIDOR ---

  // Rota principal: Envia o arquivo index.html que está na pasta data
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Rota para mudar modo
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("state")) {
      modoAtual = (ModoOperacao)request->getParam("state")->value().toInt();
      atualizarLEDs();
      atualizarDisplay();
    }
    request->send(200, "text/plain", "OK");
  });

  // Rota para o site ler os dados do sensor
  server.on("/read", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"temp\":\"" + String(temperatura, 1) + "\",\"humi\":\"" + String(umidade, 1) + "\"}";
    request->send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  static unsigned long lastMeasure = 0;
  
  // Atualiza sensores a cada 2 segundos se não estiver desligado
  if (millis() - lastMeasure > 2000) {
    lastMeasure = millis();
    
    if (modoAtual != DESLIGADO) {
      float t = dht.readTemperature();
      float h = dht.readHumidity();

      if (!isnan(t) && !isnan(h)) {
        temperatura = t;
        umidade = h;
      }
    }
    atualizarDisplay();
  }
}