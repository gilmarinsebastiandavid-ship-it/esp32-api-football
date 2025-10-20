/*
 * Sistema IoT de Estadísticas de Fútbol
 * ESP32 + OLED SSD1306 + Football-Data.org + ThingSpeak
 * 
 * Funcionalidades:
 * - Selecciona liga y partido desde web
 * - Muestra estadísticas en vivo o finalizadas
 * - Corners, tiros a puerta, resultado
 * - Actualización automática para partidos en vivo
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==================== CONFIGURACIÓN ====================
// WiFi
const char* WIFI_SSID = "MOVISTAR WIFI7026";
const char* WIFI_PASSWORD = "sebas0827";

// ThingSpeak
const char* THINGSPEAK_CHANNEL_ID = "3123536";
const char* THINGSPEAK_READ_API_KEY = "AII9P7M8KAKUR0EW";

// Football-Data.org
const char* FOOTBALL_DATA_API_KEY = "6498ab2b943c4df9ab27ab91430ab8a2";

// OLED Display (128x64)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==================== DECLARACIÓN DE FUNCIONES ====================
void connectWiFi();
void checkThingSpeakData();
void getMatchStatistics(int matchId);
void displayMessage(String line1, String line2, String line3);
void displayMatchBasic();
String shortenTeamName(String name, int maxLen = 12);
String getStatusText(String status);
String formatDate(String utcDate);
String cleanSpecialChars(String text);

// ==================== ESTRUCTURAS DE DATOS ====================
struct MatchStats {
  String homeTeam;
  String awayTeam;
  int homeScore;
  int awayScore;
  int homeCorners;
  int awayCorners;
  int homeShotsOnGoal;
  int awayShotsOnGoal;
  int homeShotsTotal;
  int awayShotsTotal;
  String status; // "IN_PLAY", "FINISHED", "SCHEDULED", "PAUSED"
  String matchDate; // Fecha del partido
  int matchId;
  bool dataValid;
  unsigned long lastUpdate;
};

MatchStats currentMatch;
unsigned long lastThingSpeakCheck = 0;
unsigned long lastStatsUpdate = 0;
const unsigned long THINGSPEAK_INTERVAL = 30000; // 30 segundos (para detectar nuevos partidos rápido)
const unsigned long STATS_UPDATE_LIVE = 900000;   // 15 minutos (900000 ms) para partidos en vivo
const unsigned long STATS_UPDATE_FINISHED = 900000; // 15 minutos para finalizados también
bool wifiConnected = false;

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== Sistema de Estadísticas de Fútbol ===");
  Serial.println("API: Football-Data.org");
  
  // Inicializar I2C para OLED (GPIO 21=SDA, GPIO 22=SCL por defecto en ESP32)
  Wire.begin(21, 22);
  
  // Inicializar OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Error: No se detectó el display OLED"));
    while(1);
  }
  
  display.clearDisplay();
  displayMessage("Iniciando...", "Sistema de", "Estadisticas");
  delay(2000);
  
  // Conectar WiFi
  connectWiFi();
  
  // Inicializar estructura
  currentMatch.dataValid = false;
  currentMatch.matchId = 0;
  currentMatch.lastUpdate = 0;
  
  displayMessage("Sistema Listo", "Esperando", "seleccion...");
  Serial.println("Sistema listo. Esperando partido de ThingSpeak...");
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  // Verificar conexión WiFi
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    displayMessage("Error WiFi", "Reconectando...", "");
    connectWiFi();
    return;
  } else {
    wifiConnected = true;
  }
  
  // Verificar ThingSpeak periódicamente para nuevos partidos
  if (millis() - lastThingSpeakCheck >= THINGSPEAK_INTERVAL) {
    lastThingSpeakCheck = millis();
    checkThingSpeakData();
  }
  
  // Actualizar estadísticas si hay un partido seleccionado
  if (currentMatch.dataValid && currentMatch.matchId > 0) {
    unsigned long updateInterval = (currentMatch.status == "IN_PLAY" || 
                                   currentMatch.status == "PAUSED") 
                                   ? STATS_UPDATE_LIVE : STATS_UPDATE_FINISHED;
    
    if (millis() - lastStatsUpdate >= updateInterval) {
      lastStatsUpdate = millis();
      getMatchStatistics(currentMatch.matchId);
    }
  }
  
  delay(100);
}

// ==================== CONEXIÓN WiFi ====================
void connectWiFi() {
  Serial.print("Conectando a WiFi: ");
  Serial.println(WIFI_SSID);
  
  displayMessage("Conectando", "WiFi...", "");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
    
    displayMessage("WiFi OK", WiFi.localIP().toString(), "");
    delay(2000);
    wifiConnected = true;
  } else {
    Serial.println("\nError: No se pudo conectar a WiFi");
    displayMessage("Error WiFi", "Verifique config", "");
    delay(5000);
    wifiConnected = false;
  }
}

// ==================== THINGSPEAK ====================
void checkThingSpeakData() {
  if (!wifiConnected) return;
  
  Serial.println("\n--- Consultando ThingSpeak ---");
  
  HTTPClient http;
  String url = "https://api.thingspeak.com/channels/" + String(THINGSPEAK_CHANNEL_ID) + 
               "/feeds/last.json?api_key=" + String(THINGSPEAK_READ_API_KEY);
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Respuesta ThingSpeak recibida");
    
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      // ThingSpeak fields:
      // field1: match_id (ID del partido)
      // field2: status (IN_PLAY, FINISHED, etc.)
      
      String field1 = doc["field1"].as<String>();
      String field2 = doc["field2"].as<String>();
      
      if (field1.length() > 0) {
        int newMatchId = field1.toInt();
        
        // Verificar si es un partido nuevo
        if (newMatchId != currentMatch.matchId || !currentMatch.dataValid) {
          Serial.print("Nuevo partido detectado: ");
          Serial.println(newMatchId);
          
          currentMatch.matchId = newMatchId;
          currentMatch.status = field2.length() > 0 ? field2 : "FINISHED";
          
          displayMessage("Cargando", "partido...", "");
          getMatchStatistics(newMatchId);
        }
      }
    } else {
      Serial.print("Error parseando JSON: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("Error HTTP ThingSpeak: ");
    Serial.println(httpCode);
  }
  
  http.end();
}

// ==================== FOOTBALL-DATA.ORG ====================
void getMatchStatistics(int matchId) {
  if (!wifiConnected || matchId == 0) return;
  
  Serial.println("\n--- Obteniendo Estadísticas del Partido ---");
  Serial.print("Match ID: ");
  Serial.println(matchId);
  
  HTTPClient http;
  
  String url = "https://api.football-data.org/v4/matches/" + String(matchId);
  
  http.begin(url);
  http.addHeader("X-Auth-Token", FOOTBALL_DATA_API_KEY);
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    
    // Parsear respuesta
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      // Extraer información básica
      currentMatch.homeTeam = cleanSpecialChars(doc["homeTeam"]["shortName"].as<String>());
      currentMatch.awayTeam = cleanSpecialChars(doc["awayTeam"]["shortName"].as<String>());
      
      // Si no hay shortName, usar el nombre completo
      if (currentMatch.homeTeam.length() == 0) {
        currentMatch.homeTeam = cleanSpecialChars(doc["homeTeam"]["name"].as<String>());
      }
      if (currentMatch.awayTeam.length() == 0) {
        currentMatch.awayTeam = cleanSpecialChars(doc["awayTeam"]["name"].as<String>());
      }
      
      // Marcador
      JsonObject score = doc["score"]["fullTime"];
      currentMatch.homeScore = score["home"] | 0;
      currentMatch.awayScore = score["away"] | 0;
      
      // Si el partido está en juego, usar el marcador regular
      if (doc["status"].as<String>() == "IN_PLAY" || doc["status"].as<String>() == "PAUSED") {
        JsonObject regularScore = doc["score"]["regularTime"];
        if (!regularScore.isNull()) {
          currentMatch.homeScore = regularScore["home"] | 0;
          currentMatch.awayScore = regularScore["away"] | 0;
        }
      }
      
      currentMatch.status = doc["status"].as<String>();
      
      // Extraer fecha del partido
      String utcDate = doc["utcDate"].as<String>();
      currentMatch.matchDate = formatDate(utcDate);
      
      Serial.println("Información básica obtenida:");
      Serial.print("  ");
      Serial.print(currentMatch.homeTeam);
      Serial.print(" ");
      Serial.print(currentMatch.homeScore);
      Serial.print(" - ");
      Serial.print(currentMatch.awayScore);
      Serial.print(" ");
      Serial.println(currentMatch.awayTeam);
      Serial.print("  Estado: ");
      Serial.println(currentMatch.status);
      
      // Extraer estadísticas si están disponibles
      // Nota: Football-Data.org no siempre tiene estadísticas detalladas en tiempo real
      // Dependiendo del plan, podrías no tener acceso a corners y tiros
      
      // Inicializar valores por defecto
      currentMatch.homeCorners = 0;
      currentMatch.awayCorners = 0;
      currentMatch.homeShotsOnGoal = 0;
      currentMatch.awayShotsOnGoal = 0;
      currentMatch.homeShotsTotal = 0;
      currentMatch.awayShotsTotal = 0;
      
      // Intentar obtener estadísticas (si están disponibles)
      // Football-Data.org no proporciona estadísticas en tiempo real en el plan gratuito
      // Solo muestra información básica del partido
      
      Serial.println("Nota: Estadísticas detalladas (corners/tiros) no disponibles en Football-Data.org API gratuita");
      
      currentMatch.dataValid = true;
      currentMatch.lastUpdate = millis();
      
      // Mostrar en OLED
      displayMatchBasic();
      
    } else {
      Serial.print("Error parseando JSON: ");
      Serial.println(error.c_str());
      displayMessage("Error", "Datos invalidos", "");
    }
  } else {
    Serial.print("Error HTTP Football-Data: ");
    Serial.println(httpCode);
    
    if (httpCode == 403) {
      displayMessage("Error 403", "Verifica", "API Key");
    } else if (httpCode == 404) {
      displayMessage("Error 404", "Partido no", "encontrado");
    } else if (httpCode == 429) {
      displayMessage("Error 429", "Limite API", "Espera 1 min");
    } else {
      displayMessage("Error API", "HTTP: " + String(httpCode), "");
    }
  }
  
  http.end();
}

// ==================== FUNCIONES DE DISPLAY ====================
void displayMessage(String line1, String line2, String line3) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 10);
  display.println(line1);
  
  display.setCursor(0, 30);
  display.println(line2);
  
  if (line3.length() > 0) {
    display.setCursor(0, 50);
    display.println(line3);
  }
  
  display.display();
}

void displayMatchBasic() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Fecha del partido (arriba)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(currentMatch.matchDate);
  
  // Estado del partido
  display.setCursor(0, 10);
  display.print("Estado: ");
  display.println(getStatusText(currentMatch.status));
  display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
  
  // Nombres de equipos (acortados si es necesario)
  display.setCursor(0, 25);
  String homeShort = shortenTeamName(currentMatch.homeTeam, 11);
  String awayShort = shortenTeamName(currentMatch.awayTeam, 11);
  display.println(homeShort);
  display.setCursor(0, 42);
  display.println(awayShort);
  
  // Marcador grande
  display.setTextSize(2);
  display.setCursor(85, 25);
  display.print(currentMatch.homeScore);
  display.setCursor(85, 42);
  display.print(currentMatch.awayScore);
  
  // Indicador de actualización
  display.setTextSize(1);
  display.setCursor(0, 57);
  int minAgo = (millis() - currentMatch.lastUpdate) / 60000; // Minutos en lugar de segundos
  display.print("Act: ");
  display.print(minAgo);
  display.print("m");
  
  display.display();
}

String shortenTeamName(String name, int maxLen) {
  if (name.length() <= maxLen) return name;
  
  // Eliminar palabras comunes
  name.replace(" FC", "");
  name.replace(" CF", "");
  name.replace(" United", "");
  name.replace(" City", "");
  name.replace(" Utd", "");
  
  if (name.length() <= maxLen) return name;
  
  // Truncar y agregar puntos
  return name.substring(0, maxLen - 1) + ".";
}

String getStatusText(String status) {
  if (status == "IN_PLAY") return "EN VIVO";
  if (status == "PAUSED") return "DESCANSO";
  if (status == "FINISHED") return "FINALIZADO";
  if (status == "SCHEDULED" || status == "TIMED") return "PROGRAMADO";
  if (status == "POSTPONED") return "POSPUESTO";
  if (status == "CANCELLED") return "CANCELADO";
  if (status == "SUSPENDED") return "SUSPENDIDO";
  if (status == "AWARDED") return "ADJUDICADO";
  return status;
}

String formatDate(String utcDate) {
  // utcDate viene en formato: "2024-01-15T20:00:00Z"
  // Extraer: año, mes, día
  
  if (utcDate.length() < 10) return "Sin fecha";
  
  String year = utcDate.substring(2, 4);   // "24"
  String month = utcDate.substring(5, 7);  // "01"
  String day = utcDate.substring(8, 10);   // "15"
  
  // Nombres de meses
  String monthNames[] = {"Ene", "Feb", "Mar", "Abr", "May", "Jun", 
                         "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"};
  
  int monthInt = month.toInt();
  String monthName = (monthInt >= 1 && monthInt <= 12) ? monthNames[monthInt - 1] : month;
  
  // Formato: "15 Ene 2024"
  return day + " " + monthName + " " + "20" + year;
}

String cleanSpecialChars(String text) {
  // Reemplazar caracteres especiales comunes
  
  // Caracteres acentuados
  text.replace("á", "a"); text.replace("Á", "A");
  text.replace("é", "e"); text.replace("É", "E");
  text.replace("í", "i"); text.replace("Í", "I");
  text.replace("ó", "o"); text.replace("Ó", "O");
  text.replace("ú", "u"); text.replace("Ú", "U");
  text.replace("ü", "u"); text.replace("Ü", "U");
  
  // Caracteres especiales específicos
  text.replace("ñ", "n"); text.replace("Ñ", "N");
  text.replace("ç", "c"); text.replace("Ç", "C");
  text.replace("ã", "a"); text.replace("Ã", "A");
  text.replace("õ", "o"); text.replace("Õ", "O");
  text.replace("â", "a"); text.replace("Â", "A");
  text.replace("ê", "e"); text.replace("Ê", "E");
  text.replace("î", "i"); text.replace("Î", "I");
  text.replace("ô", "o"); text.replace("Ô", "O");
  text.replace("û", "u"); text.replace("Û", "U");
  
  // Otros caracteres
  text.replace("'", "'"); text.replace("'", "'");
  text.replace("–", "-"); text.replace("—", "-");
  
  return text;
}

// ==================== FIN DEL CÓDIGO ====================