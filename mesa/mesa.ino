/********************************************************
 * CANAL INTERNET E COISAS
 * youtube.com/@internetecoisas
 * ESP32/ESP8266 - Controle de LED WS2812 via MQTT
 * 06/2025 - André Michelon
 */

// Bibliotecas ------------------------------------------
#ifdef ESP32
  #include <WiFi.h>         // Biblioteca para ESP32
#elif defined ESP8266
  #include <ESP8266WiFi.h>  // Biblioteca para ESP8266
#else
  #error Placa inválida     // Erro, placa incorreta
#endif
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

// Wi-Fi ------------------------------------------------
const char*   SSID                = "home";
const char*   PASSWORD            = "Home@135711";

// Constantes -------------------------------------------
// LED WS2812 - pino
const byte    LED_PIN             = 13;
// LED WS2812 - quantidade
const byte    LED_QTD             = 24;
// Broker
const char*   MQTT_SERVER         = "test.mosquitto.org";
// Porta
const int     MQTT_PORT           = 1883;
// Cliente
const char*   MQTT_CLIENT         = "IeCLEDWS2812";
// Tópicos MQTT
const char*   MQTT_COR            = "IeC/led/cor";
const char*   MQTT_BRILHO         = "IeC/led/brilho";

// Instâncias -------------------------------------------
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient, MQTT_SERVER, MQTT_PORT);
Adafruit_NeoPixel leds(LED_QTD, LED_PIN, NEO_GRB + NEO_KHZ800);

// Funcoes Genericas ------------------------------------
uint64_t deviceID() {
  // Obtém ID único do ESP
  uint8_t mac[6];
  WiFi.macAddress(mac);
  uint64_t id = 0;
  for (int i = 0; i < 6; i++) {
    id = (id << 8) | mac[i];
  }
  return id;
}

const char* platform() {
  // Obtém a plataforma de hardware em uso
  #if defined(__AVR_ATmega328P__)
    return "UNO/NANO";
  #elif defined(__AVR_ATmega2560__)
    return "MEGA";
  #elif defined(__AVR_ATmega32U4__)
    return "LEONARDO/MICRO";
  #elif defined(ARDUINO_BOARD)
    return ARDUINO_BOARD;
  #else
    return "DESCONHECIDO";
  #endif
}

void reconnect() {
  // Conecta ao Broker
  while (!mqttClient.connected()) {
    Serial.println("Connectando Broker...");

    // Obtém ID do cliente
    String s = MQTT_CLIENT;
    s += "-" + String(deviceID(), HEX);

    // Conecta
    if (mqttClient.connect(s)) {
      Serial.println("Conectado como " +s);
      mqttClient.subscribe(MQTT_COR);
      mqttClient.subscribe(MQTT_BRILHO);
    } else {
      // Falha na conexao
      Serial.println("Falha");
      delay(5000);
    }
  }
}

// Processa requisoes MQTT ------------------------------
void mqttCallback(const MQTT::Publish& pub) {
  // Trata topicos MQTT recebidos
  String topic = pub.topic();
  String payload = pub.payload_string();
  payload.toLowerCase();
  uint32_t cor = 0x000000;
  Serial.println("[" + topic + "] " + payload);
  if (topic == MQTT_COR) {
    if (payload == "arcoiris") {
      // Preenche LEDs com arco-íris
      leds.rainbow();
    } else {
      // Preenche LEDs com uma cor
      if (payload.startsWith("#") && payload.length() == 7) {
          // Cor informada em formato #RRGGBB
          cor = strtol(payload.substring(1).c_str(), NULL, 16);
      } else if (payload == "vermelho") {
          cor = 0xff0000;
      } else if (payload == "verde"   ) {
          cor = 0x00ff00;
      } else if (payload == "azul"    ) {
          cor = 0x0000ff;
      } else if (payload == "ciano"   ) {
          cor = 0x00ffff;
      } else if (payload == "magenta" ) {
          cor = 0xff00ff;
      } else if (payload == "amarelo" ) {
          cor = 0xffff00;
      } else if (payload == "branco"  ) {
          cor = 0xffffff;
      }
      // Aplica a cor
      leds.fill(cor);
    }
  } else if (topic == MQTT_BRILHO) {
    // Define o brilho (0 a 255)
    leds.setBrightness(constrain(payload.toInt(), 0, 255));
  }
  leds.show();
}

// Setup ------------------------------------------------
void setup() {
  // Incializa
  #ifdef ESP32
    Serial.begin(115200); // Velocidade padrão para ESP32
  #else
    Serial.begin(74880);  // Velocidade padrão para ESP8266
  #endif

  Serial.println("\n*** CANAL INTERNET E COISAS ***\n"
                  "Controle de LED WS2812 via MQTT");
  Serial.print("Plataforma: "); Serial.println(platform());
  Serial.print("Quantidade de LEDs: "); Serial.println(LED_QTD);

  // Prepara LEDs
  leds.begin();
  leds.setBrightness(25); // Define brilho em 10%
  leds.show(); // Apaga todos os LEDs

  // Define função de tratamento as mensagens MQTT
  mqttClient.set_callback(mqttCallback);

  // Conecta Wi-Fi
  Serial.print("Conectando WiFi");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi conectado, IP "); Serial.println(WiFi.localIP());
}

// Loop -------------------------------------------------
void loop() {
  // Processa conexao ao Broker
  if (mqttClient.connected()) {
    mqttClient.loop();
  } else {
    reconnect();
  }
}
