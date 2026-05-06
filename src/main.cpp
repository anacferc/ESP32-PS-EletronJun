#include <Arduino.h>
#include <Arduino.h> // Obrigatório para o PlatformIO
#include <SPI.h>     // Necessário para comunicação com o RFID e Display
#include <Wire.h>    // Necessário para o Display I2C

// --- Inclusão das Bibliotecas Instaladas ---
#include <Adafruit_GFX.h>      // Biblioteca base de gráficos
#include <Adafruit_SSD1306.h>  // Para o Display OLED
#include <MFRC522.h>           // Para o Leitor RFID
#include <DHT.h>               // Para o Sensor de Temperatura e Umidade
#include <ESPAsyncWebServer.h> // Para o Servidor Web
#include <AsyncTCP.h>          // Suporte para o Servidor Web no ESP32
#include <WiFi.h>              // Para conexão Wi-Fi do ESP32

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

// --- Configurações de Wi-Fi ---
const char *ssid = "NOME_DA_SUA_REDE";
const char *password = "SENHA_DA_REDE";

// --- Definições de Hardware ---
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define LED_OFF 13 // Exemplo de pinos
#define LED_AUTO 12
#define LED_ON 14

// --- Variáveis de Controle ---
enum ModoOperacao
{
  DESLIGADO = 0,
  AUTOMATICO = 1,
  LIGADO = 2
};
ModoOperacao modoAtual = DESLIGADO;

AsyncWebServer server(80);

// --- Função para atualizar os LEDs baseado no modo ---
void atualizarLEDs()
{
  digitalWrite(LED_OFF, modoAtual == DESLIGADO);
  digitalWrite(LED_AUTO, modoAtual == AUTOMATICO);
  digitalWrite(LED_ON, modoAtual == LIGADO);
}

void setup()
{
  Serial.begin(115200);

  // Configura Pinos
  pinMode(LED_OFF, OUTPUT);
  pinMode(LED_AUTO, OUTPUT);
  pinMode(LED_ON, OUTPUT);

  dht.begin();
  atualizarLEDs();

  // Conecta Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.localIP());

  // --- ROTA 1: Enviar o arquivo HTML para o navegador ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        // Quando a placa chegar, usaremos LittleFS. Por enquanto, 
        // o ESP pode enviar uma mensagem confirmando que está ativo.
        request->send(200, "text/html", "O ESP32 está rodando. Aguardando LittleFS..."); });

  // --- ROTA 2: Receber o comando de mudança de modo ---
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        if (request->hasParam("state")) {
            int estado = request->getParam("state")->value().toInt();
            modoAtual = (ModoOperacao)estado;
            atualizarLEDs();
            Serial.printf("Modo alterado para: %d\n", estado);
        }
        request->send(200, "text/plain", "OK"); });

  // Rota para o site ler a temperatura e umidade
  server.on("/read", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    // Cria um JSON simples para o navegador entender
    String json = "{";
    json += "\"temp\":\"" + String(t, 1) + "\",";
    json += "\"humi\":\"" + String(h, 1) + "\"";
    json += "}";
    
    request->send(200, "application/json", json); });

  server.begin();
}

void loop()
{
  if (modoAtual == AUTOMATICO)
  {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t))
    {
      Serial.printf("Temp: %.1f | Umid: %.1f\n", t, h);
    }
    delay(2000); // No modo automático, lê a cada 2 segundos
  }
}