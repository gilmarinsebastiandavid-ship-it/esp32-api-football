/*
 * Sistema IoT de Estadísticas de Fútbol con Servidor Web Embebido
 * ESP32 + OLED SSD1306 + Football-Data.org + ThingSpeak
 * 
 * Funcionalidades:
 * - Servidor web en el ESP32 (acceso por IP)
 * - Selecciona liga y partido desde navegador
 * - Muestra estadísticas en OLED
 * - Actualización automática para partidos en vivo
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
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
const char* THINGSPEAK_CHANNEL_ID = "TU_NUEVO_CHANNEL_ID";
const char* THINGSPEAK_READ_API_KEY = "TU_NUEVO_READ_API_KEY";
const char* THINGSPEAK_WRITE_API_KEY = "TU_WRITE_API_KEY";

// Football-Data.org
const char* FOOTBALL_DATA_API_KEY = "6498ab2b943c4df9ab27ab91430ab8a2";

// OLED Display (128x64)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Servidor Web en puerto 80
WebServer server(80);

// ==================== DECLARACIÓN DE FUNCIONES ====================
void connectWiFi();
void setupWebServer();
void handleRoot();
void handleSetMatch();
void handleStatus();
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
  String status;
  String matchDate;
  int matchId;
  bool dataValid;
  unsigned long lastUpdate;
};

MatchStats currentMatch;
unsigned long lastThingSpeakCheck = 0;
unsigned long lastStatsUpdate = 0;
const unsigned long THINGSPEAK_INTERVAL = 30000;
const unsigned long STATS_UPDATE_LIVE = 900000;
const unsigned long STATS_UPDATE_FINISHED = 900000;
bool wifiConnected = false;

// ==================== HTML DE LA PÁGINA WEB ====================
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Control ESP32 - Partidos</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 600px;
            width: 100%;
            padding: 40px;
        }
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 28px;
            text-align: center;
        }
        .subtitle {
            color: #666;
            text-align: center;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .ip-info {
            background: #f0f0f0;
            padding: 10px;
            border-radius: 10px;
            text-align: center;
            margin-bottom: 20px;
            font-size: 14px;
            color: #555;
        }
        .section {
            margin-bottom: 25px;
        }
        label {
            display: block;
            color: #555;
            font-weight: 600;
            margin-bottom: 8px;
            font-size: 14px;
        }
        input, select {
            width: 100%;
            padding: 12px 15px;
            border: 2px solid #e0e0e0;
            border-radius: 10px;
            font-size: 15px;
            transition: all 0.3s;
        }
        input:focus, select:focus {
            outline: none;
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }
        .btn {
            width: 100%;
            padding: 15px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
            margin-top: 10px;
        }
        .btn:hover:not(:disabled) {
            transform: translateY(-2px);
            box-shadow: 0 10px 25px rgba(102, 126, 234, 0.3);
        }
        .btn:disabled {
            opacity: 0.6;
            cursor: not-allowed;
        }
        .status {
            padding: 15px;
            border-radius: 10px;
            margin-top: 20px;
            font-size: 14px;
            display: none;
        }
        .status.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
            display: block;
        }
        .status.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
            display: block;
        }
        .status.info {
            background: #d1ecf1;
            color: #0c5460;
            border: 1px solid #bee5eb;
            display: block;
        }
        .match-item {
            padding: 15px;
            border: 2px solid #e0e0e0;
            border-radius: 10px;
            margin-bottom: 10px;
            cursor: pointer;
            transition: all 0.3s;
            background: white;
        }
        .match-item:hover {
            border-color: #667eea;
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.2);
            transform: translateY(-2px);
        }
        .match-item.selected {
            border-color: #667eea;
            background: linear-gradient(135deg, rgba(102, 126, 234, 0.1) 0%, rgba(118, 75, 162, 0.1) 100%);
        }
        .match-teams {
            font-weight: 600;
            color: #333;
            margin-bottom: 5px;
            font-size: 16px;
        }
        .match-info {
            color: #666;
            font-size: 13px;
        }
        .match-score {
            float: right;
            font-size: 20px;
            font-weight: bold;
            color: #667eea;
        }
        .badge {
            display: inline-block;
            padding: 4px 10px;
            border-radius: 12px;
            font-size: 11px;
            font-weight: 600;
            margin-left: 8px;
        }
        .badge.live {
            background: #ff4757;
            color: white;
            animation: pulse 2s infinite;
        }
        .badge.finished {
            background: #28a745;
            color: white;
        }
        .badge.scheduled {
            background: #ffc107;
            color: #333;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.6; }
        }
        .matches-container {
            max-height: 400px;
            overflow-y: auto;
            margin-top: 15px;
            padding-right: 5px;
        }
        .matches-container::-webkit-scrollbar {
            width: 8px;
        }
        .matches-container::-webkit-scrollbar-track {
            background: #f1f1f1;
            border-radius: 10px;
        }
        .matches-container::-webkit-scrollbar-thumb {
            background: #667eea;
            border-radius: 10px;
        }
        .loader {
            border: 4px solid #f3f3f3;
            border-top: 4px solid #667eea;
            border-radius: 50%;
            width: 40px;
            height: 40px;
            animation: spin 1s linear infinite;
            margin: 20px auto;
            display: none;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        .manual-section {
            background: #fff3cd;
            padding: 15px;
            border-radius: 10px;
            border: 2px dashed #ffc107;
            margin-top: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>⚽ Control ESP32</h1>
        <p class="subtitle">Sistema de Estadísticas de Fútbol en Tiempo Real</p>
        <div class="ip-info" id="ipInfo">📡 Conectado al ESP32</div>
        
        <!-- Selección de Competición -->
        <div class="section">
            <label for="competition">🏆 Competición</label>
            <select id="competition">
                <option value="">-- Selecciona una competición --</option>
                <option value="2021">🏴󠁧󠁢󠁥󠁮󠁧󠁿 Premier League (Inglaterra)</option>
                <option value="2014">🇪🇸 La Liga (España)</option>
                <option value="2002">🇩🇪 Bundesliga (Alemania)</option>
                <option value="2019">🇮🇹 Serie A (Italia)</option>
                <option value="2015">🇫🇷 Ligue 1 (Francia)</option>
                <option value="2001">🇪🇺 UEFA Champions League</option>
                <option value="2018">🇪🇺 UEFA Europa League</option>
                <option value="2152">🇧🇷 Brasileirão (Brasil)</option>
            </select>
            <button class="btn" id="loadBtn" onclick="loadMatches()">📋 Cargar Partidos</button>
        </div>
        
        <!-- Loader -->
        <div class="loader" id="loader"></div>
        
        <!-- Filtro de Estado -->
        <div class="section" id="filterSection" style="display: none;">
            <label for="statusFilter">🔍 Filtrar por Estado</label>
            <select id="statusFilter" onchange="filterMatches()">
                <option value="ALL">Todos los partidos</option>
                <option value="IN_PLAY">🔴 EN VIVO</option>
                <option value="FINISHED">✅ Finalizados</option>
                <option value="SCHEDULED">📅 Programados</option>
            </select>
        </div>
        
        <!-- Lista de Partidos -->
        <div class="section" id="matchesSection" style="display: none;">
            <label>⚽ Selecciona un Partido</label>
            <div class="matches-container" id="matchesList"></div>
        </div>
        
        <!-- Ingreso Manual -->
        <div class="manual-section">
            <label for="matchId">🎯 O ingresa el Match ID manualmente</label>
            <input type="number" id="matchId" placeholder="Ej: 327120">
            <button class="btn" onclick="sendManualMatch()" style="background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);">🚀 Enviar al ESP32</button>
        </div>
        
        <div class="status" id="status"></div>
    </div>

    <script>
        const API_KEY = '6498ab2b943c4df9ab27ab91430ab8a2';
        let allMatches = [];
        let selectedMatch = null;
        
        async function loadMatches() {
            const competitionId = document.getElementById('competition').value;
            
            if (!competitionId) {
                showStatus('Por favor selecciona una competición', 'error');
                return;
            }
            
            showStatus('Cargando partidos...', 'info');
            document.getElementById('loader').style.display = 'block';
            document.getElementById('loadBtn').disabled = true;
            document.getElementById('matchesSection').style.display = 'none';
            document.getElementById('filterSection').style.display = 'none';
            
            try {
                const proxyUrl = 'https://corsproxy.io/?';
                const apiUrl = 'https://api.football-data.org/v4/competitions/' + competitionId + '/matches';
                
                const response = await fetch(proxyUrl + encodeURIComponent(apiUrl), {
                    headers: {
                        'X-Auth-Token': API_KEY
                    }
                });
                
                if (!response.ok) throw new Error('Error HTTP: ' + response.status);
                
                const data = await response.json();
                allMatches = data.matches || [];
                
                if (allMatches.length === 0) {
                    showStatus('No hay partidos disponibles para esta competición', 'info');
                    return;
                }
                
                allMatches.sort((a, b) => {
                    const statusOrder = { 'IN_PLAY': 0, 'PAUSED': 1, 'SCHEDULED': 2, 'TIMED': 2, 'FINISHED': 3 };
                    return (statusOrder[a.status] || 4) - (statusOrder[b.status] || 4);
                });
                
                document.getElementById('filterSection').style.display = 'block';
                document.getElementById('matchesSection').style.display = 'block';
                document.getElementById('statusFilter').value = 'ALL';
                
                displayMatches(allMatches);
                showStatus('✅ ' + allMatches.length + ' partidos cargados correctamente', 'success');
                
            } catch (error) {
                showStatus('❌ Error al cargar partidos: ' + error.message, 'error');
            } finally {
                document.getElementById('loader').style.display = 'none';
                document.getElementById('loadBtn').disabled = false;
            }
        }
        
        function filterMatches() {
            const filterValue = document.getElementById('statusFilter').value;
            
            if (filterValue === 'ALL') {
                displayMatches(allMatches);
            } else {
                const filtered = allMatches.filter(match => match.status === filterValue);
                displayMatches(filtered);
            }
        }
        
        function displayMatches(matches) {
            const container = document.getElementById('matchesList');
            container.innerHTML = '';
            
            if (matches.length === 0) {
                container.innerHTML = '<p style="text-align: center; color: #999; padding: 20px;">No hay partidos disponibles</p>';
                return;
            }
            
            matches.forEach(match => {
                const div = document.createElement('div');
                div.className = 'match-item';
                div.onclick = () => selectMatch(match, div);
                
                const homeTeam = match.homeTeam.shortName || match.homeTeam.name;
                const awayTeam = match.awayTeam.shortName || match.awayTeam.name;
                const homeScore = match.score.fullTime.home ?? (match.score.regularTime.home ?? '-');
                const awayScore = match.score.fullTime.away ?? (match.score.regularTime.away ?? '-');
                
                const statusBadge = getStatusBadge(match.status);
                const date = new Date(match.utcDate);
                const dateStr = date.toLocaleDateString('es-ES', { 
                    day: '2-digit', 
                    month: 'short', 
                    hour: '2-digit', 
                    minute: '2-digit' 
                });
                
                div.innerHTML = `
                    <div class="match-teams">
                        ${homeTeam} vs ${awayTeam}
                        ${statusBadge}
                    </div>
                    <div class="match-info">
                        📅 ${dateStr} | ID: ${match.id}
                        <span class="match-score">${homeScore} - ${awayScore}</span>
                    </div>
                `;
                
                container.appendChild(div);
            });
        }
        
        function selectMatch(match, element) {
            document.querySelectorAll('.match-item').forEach(item => {
                item.classList.remove('selected');
            });
            
            element.classList.add('selected');
            selectedMatch = match;
            
            showStatus('Partido seleccionado: ' + match.homeTeam.name + ' vs ' + match.awayTeam.name, 'info');
            
            // Enviar automáticamente
            sendMatchToESP32(match.id);
        }
        
        function sendManualMatch() {
            const matchId = document.getElementById('matchId').value;
            
            if (!matchId) {
                showStatus('Por favor ingresa un Match ID', 'error');
                return;
            }
            
            sendMatchToESP32(matchId);
        }
        
        function sendMatchToESP32(matchId) {
            showStatus('Enviando al ESP32...', 'info');
            
            fetch('/setmatch?id=' + matchId)
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        showStatus('✅ Partido enviado correctamente al ESP32!', 'success');
                    } else {
                        showStatus('❌ Error: ' + data.message, 'error');
                    }
                })
                .catch(error => {
                    showStatus('❌ Error de conexión con el ESP32', 'error');
                });
        }
        
        function getStatusBadge(status) {
            const badges = {
                'IN_PLAY': '<span class="badge live">🔴 EN VIVO</span>',
                'PAUSED': '<span class="badge live">⏸️ DESCANSO</span>',
                'FINISHED': '<span class="badge finished">✅ FINALIZADO</span>',
                'SCHEDULED': '<span class="badge scheduled">📅 PROGRAMADO</span>',
                'TIMED': '<span class="badge scheduled">📅 PROGRAMADO</span>'
            };
            return badges[status] || '<span class="badge">' + status + '</span>';
        }
        
        function showStatus(message, type) {
            const statusDiv = document.getElementById('status');
            statusDiv.textContent = message;
            statusDiv.className = 'status ' + type;
            
            if (type !== 'error') {
                setTimeout(() => {
                    statusDiv.style.display = 'none';
                }, 5000);
            }
        }
        
        // Obtener IP del ESP32
        fetch('/status')
            .then(response => response.json())
            .then(data => {
                // La IP se muestra en el OLED
            })
            .catch(error => console.log('Info de estado no disponible'));
    </script>
</body>
</html>
)rawliteral";

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== Sistema de Estadísticas de Fútbol ===");
  Serial.println("API: Football-Data.org");
  
  // Inicializar I2C para OLED
  Wire.begin(21, 22);
  
  // Inicializar OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Error: No se detectó el display OLED"));
    while(1);
  }
  
  display.clearDisplay();
  displayMessage("Iniciando...", "Servidor Web", "ESP32");
  delay(2000);
  
  // Conectar WiFi
  connectWiFi();
  
  // Configurar servidor web
  setupWebServer();
  
  // Inicializar estructura
  currentMatch.dataValid = false;
  currentMatch.matchId = 0;
  currentMatch.lastUpdate = 0;
  
  displayMessage("Sistema Listo", "IP:", WiFi.localIP().toString());
  Serial.println("========================================");
  Serial.print("Accede a la interfaz web en: http://");
  Serial.println(WiFi.localIP());
  Serial.println("========================================");
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  // Manejar solicitudes web
  server.handleClient();
  
  // Verificar conexión WiFi
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    displayMessage("Error WiFi", "Reconectando...", "");
    connectWiFi();
    return;
  } else {
    wifiConnected = true;
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
  
  delay(10);
}

// ==================== SERVIDOR WEB ====================
void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/setmatch", handleSetMatch);
  server.on("/status", handleStatus);
  
  server.begin();
  Serial.println("Servidor web iniciado");
}

void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleSetMatch() {
  if (server.hasArg("id")) {
    int matchId = server.arg("id").toInt();
    
    if (matchId > 0) {
      Serial.print("Nuevo partido recibido desde web: ");
      Serial.println(matchId);
      
      currentMatch.matchId = matchId;
      currentMatch.status = "SCHEDULED";
      
      displayMessage("Cargando", "partido...", String(matchId));
      getMatchStatistics(matchId);
      
      server.send(200, "application/json", "{\"success\":true,\"message\":\"Partido configurado\"}");
    } else {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"Match ID inválido\"}");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Falta parámetro id\"}");
  }
}

void handleStatus() {
  String json = "{";
  json += "\"matchId\":" + String(currentMatch.matchId) + ",";
  json += "\"homeTeam\":\"" + currentMatch.homeTeam + "\",";
  json += "\"awayTeam\":\"" + currentMatch.awayTeam + "\",";
  json += "\"homeScore\":" + String(currentMatch.homeScore) + ",";
  json += "\"awayScore\":" + String(currentMatch.awayScore) + ",";
  json += "\"status\":\"" + getStatusText(currentMatch.status) + "\",";
  json += "\"dataValid\":" + String(currentMatch.dataValid ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
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
    
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      currentMatch.homeTeam = cleanSpecialChars(doc["homeTeam"]["shortName"].as<String>());
      currentMatch.awayTeam = cleanSpecialChars(doc["awayTeam"]["shortName"].as<String>());
      
      if (currentMatch.homeTeam.length() == 0) {
        currentMatch.homeTeam = cleanSpecialChars(doc["homeTeam"]["name"].as<String>());
      }
      if (currentMatch.awayTeam.length() == 0) {
        currentMatch.awayTeam = cleanSpecialChars(doc["awayTeam"]["name"].as<String>());
      }
      
      JsonObject score = doc["score"]["fullTime"];
      currentMatch.homeScore = score["home"] | 0;
      currentMatch.awayScore = score["away"] | 0;
      
      if (doc["status"].as<String>() == "IN_PLAY" || doc["status"].as<String>() == "PAUSED") {
        JsonObject regularScore = doc["score"]["regularTime"];
        if (!regularScore.isNull()) {
          currentMatch.homeScore = regularScore["home"] | 0;
          currentMatch.awayScore = regularScore["away"] | 0;
        }
      }
      
      currentMatch.status = doc["status"].as<String>();
      
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
      
      currentMatch.dataValid = true;
      currentMatch.lastUpdate = millis();
      
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
  
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(currentMatch.matchDate);
  
  display.setCursor(0, 10);
  display.print("Estado: ");
  display.println(getStatusText(currentMatch.status));
  display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
  
  display.setCursor(0, 25);
  String homeShort = shortenTeamName(currentMatch.homeTeam, 11);
  String awayShort = shortenTeamName(currentMatch.awayTeam, 11);
  display.println(homeShort);
  display.setCursor(0, 42);
  display.println(awayShort);
  
  display.setTextSize(2);
  display.setCursor(85, 25);
  display.print(currentMatch.homeScore);
  display.setCursor(85, 42);
  display.print(currentMatch.awayScore);
  
  display.setTextSize(1);
  display.setCursor(0, 57);
  int minAgo = (millis() - currentMatch.lastUpdate) / 60000;
  display.print("Act: ");
  display.print(minAgo);
  display.print("m");
  
  display.display();
}

String shortenTeamName(String name, int maxLen) {
  if (name.length() <= maxLen) return name;
  
  name.replace(" FC", "");
  name.replace(" CF", "");
  name.replace(" United", "");
  name.replace(" City", "");
  name.replace(" Utd", "");
  
  if (name.length() <= maxLen) return name;
  
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
  if (utcDate.length() < 10) return "Sin fecha";
  
  String year = utcDate.substring(2, 4);
  String month = utcDate.substring(5, 7);
  String day = utcDate.substring(8, 10);
  
  String monthNames[] = {"Ene", "Feb", "Mar", "Abr", "May", "Jun", 
                         "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"};
  
  int monthInt = month.toInt();
  String monthName = (monthInt >= 1 && monthInt <= 12) ? monthNames[monthInt - 1] : month;
  
  return day + " " + monthName + " " + "20" + year;
}

String cleanSpecialChars(String text) {
  text.replace("á", "a"); text.replace("Á", "A");
  text.replace("é", "e"); text.replace("É", "E");
  text.replace("í", "i"); text.replace("Í", "I");
  text.replace("ó", "o"); text.replace("Ó", "O");
  text.replace("ú", "u"); text.replace("Ú", "U");
  text.replace("ü", "u"); text.replace("Ü", "U");
  text.replace("ñ", "n"); text.replace("Ñ", "N");
  text.replace("ç", "c"); text.replace("Ç", "C");
  text.replace("ã", "a"); text.replace("Ã", "A");
  text.replace("õ", "o"); text.replace("Õ", "O");
  text.replace("â", "a"); text.replace("Â", "A");
  text.replace("ê", "e"); text.replace("Ê", "E");
  text.replace("î", "i"); text.replace("Î", "I");
  text.replace("ô", "o"); text.replace("Ô", "O");
  text.replace("û", "u"); text.replace("Û", "U");
  text.replace("'", "'"); text.replace("'", "'");
  text.replace("–", "-"); text.replace("—", "-");
  
  return text;
}

// ==================== FIN DEL CÓDIGO ====================