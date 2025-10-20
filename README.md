# esp32-api-football# ⚽ Sistema IoT de Estadísticas de Fútbol en Tiempo Real

Sistema completo de Internet de las Cosas que integra un ESP32 con display OLED para mostrar información de partidos de fútbol en tiempo real, controlado remotamente desde una interfaz web.



---

## 📋 Tabla de Contenidos

- [Descripción](#-descripción)
- [Características](#-características)
- [Hardware Requerido](#-hardware-requerido)
- [Software y APIs](#-software-y-apis)
- [Instalación](#-instalación)
- [Configuración](#-configuración)
- [Uso](#-uso)
- [Arquitectura del Sistema](#-arquitectura-del-sistema)
- [Problemas Encontrados y Soluciones](#-problemas-encontrados-y-soluciones)
- [Limitaciones](#-limitaciones)
- [Mejoras Futuras](#-mejoras-futuras)
- [Autor](#-autor)

---

## 🎯 Descripción

Este proyecto académico demuestra la implementación de un sistema IoT completo que:

1. **Consulta datos** de una API REST externa (Football-Data.org)
2. **Se comunica** bidireccionalmente con servicios en la nube (ThingSpeak)
3. **Muestra información** en tiempo real en un display OLED
4. **Es controlado remotamente** mediante una interfaz web

El sistema permite seleccionar partidos de fútbol desde una página web y visualizar en el ESP32:
- Fecha del partido
- Equipos enfrentados
- Marcador actual
- Estado del partido (EN VIVO, FINALIZADO, etc.)

---

## ✨ Características

### Funcionalidades Principales

- ✅ **Selección remota de partidos** - Desde interfaz web
- ✅ **Actualización automática** - Cada 15 minutos (optimizado para límites de API)
- ✅ **Display optimizado** - Información clara y legible
- ✅ **Manejo de caracteres especiales** - Conversión automática (ç→c, ñ→n, etc.)
- ✅ **Reconexión WiFi automática** - Sistema robusto ante fallos
- ✅ **Múltiples competiciones** - Premier League, La Liga, Champions League, etc.
- ✅ **Match ID manual** - Opción alternativa de ingreso directo

### Competencias Técnicas Demostradas

- 🌐 **Internet de las Cosas (IoT)**
- 🔌 **Integración de APIs REST**
- ☁️ **Computación en la Nube**
- 📡 **Comunicación bidireccional**
- 💻 **Programación embebida (C/C++)**
- 🎨 **Interfaz de usuario web (HTML/CSS/JavaScript)**
- 🔧 **Manejo de protocolos HTTP/HTTPS**

---

## 🛠️ Hardware Requerido

| Componente | Especificaciones | Cantidad |
|------------|------------------|----------|
| **ESP32 DevKit** | Módulo con WiFi integrado | 1 |
| **Display OLED** | SSD1306, 128x64, I2C | 1 |
| **Cables Jumper** | Macho-Hembra | 4 |
| **Cable USB** | Micro-USB para programación | 1 |

### Conexiones Hardware

```
ESP32          →    OLED SSD1306
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
3.3V           →    VCC
GND            →    GND
GPIO 21 (SDA)  →    SDA
GPIO 22 (SCL)  →    SCL
```

---

## 💻 Software y APIs

### Herramientas de Desarrollo

- **Visual Studio Code** con extensión PlatformIO
- **Python 3.x** (para servidor web local)
- **Navegador web moderno** (Chrome, Firefox, Edge)

### APIs y Servicios en la Nube

1. **Football-Data.org**
   - Función: Proveedor de datos de partidos
   - Plan: Gratuito (10 requests/minuto)
   - Registro: https://www.football-data.org/client/register

2. **ThingSpeak**
   - Función: Puente de comunicación web-ESP32
   - Plan: Gratuito
   - Registro: https://thingspeak.com/

### Librerías Arduino/PlatformIO

```ini
lib_deps = 
    adafruit/Adafruit GFX Library@^1.11.3
    adafruit/Adafruit SSD1306@^2.5.7
    bblanchon/ArduinoJson@^6.21.3
```

---

## 📥 Instalación

### 1. Clonar o Descargar el Proyecto

```bash
git clone [URL_DEL_REPOSITORIO]
cd football-stats-esp32
```

### 2. Instalar PlatformIO en VS Code

1. Abre Visual Studio Code
2. Ve a Extensions (Ctrl+Shift+X)
3. Busca "PlatformIO IDE"
4. Instala la extensión
5. Reinicia VS Code

### 3. Abrir el Proyecto

1. File → Open Folder
2. Selecciona la carpeta del proyecto
3. PlatformIO detectará automáticamente la configuración

### 4. Instalar Dependencias

Las librerías se instalarán automáticamente al compilar por primera vez.

---

## ⚙️ Configuración

### Paso 1: Configurar Football-Data.org

1. Regístrate en: https://www.football-data.org/client/register
2. Verifica tu email
3. Copia tu API Token (aparece al iniciar sesión)

### Paso 2: Configurar ThingSpeak

1. Crea una cuenta en: https://thingspeak.com/
2. Click en **"Channels"** → **"New Channel"**
3. Configura el canal:
   - **Name**: `ESP32 Football Stats`
   - **Field 1**: `match_id`
   - **Field 2**: `status`
4. Guarda el canal
5. Ve a **"API Keys"** y copia:
   - **Channel ID**
   - **Write API Key**
   - **Read API Key**

### Paso 3: Actualizar Credenciales en el Código

Abre `src/main.cpp` y actualiza:

```cpp
// WiFi
const char* WIFI_SSID = "TU_RED_WIFI";
const char* WIFI_PASSWORD = "TU_CONTRASEÑA";

// ThingSpeak
const char* THINGSPEAK_CHANNEL_ID = "TU_CHANNEL_ID";
const char* THINGSPEAK_READ_API_KEY = "TU_READ_API_KEY";

// Football-Data.org
const char* FOOTBALL_DATA_API_KEY = "TU_API_KEY";
```

### Paso 4: Compilar y Subir al ESP32

1. Conecta el ESP32 por USB
2. En VS Code, click en el icono ✓ (Build)
3. Espera a que compile
4. Click en el icono → (Upload)
5. Espera a que termine la carga
6. Abre el Monitor Serial (icono 🔌) a 115200 baud

---

## 🚀 Uso

### Iniciar el Sistema

1. **Conecta el ESP32** a la alimentación
2. El display mostrará:
   ```
   Iniciando...
   Sistema de
   Estadisticas
   ```
3. Luego conectará al WiFi:
   ```
   WiFi OK
   192.168.1.XXX
   ```
4. Finalmente:
   ```
   Sistema Listo
   Esperando
   seleccion...
   ```

### Seleccionar un Partido desde la Web

#### Opción A: Usando Servidor Local (Recomendado)

1. **Inicia el servidor Python:**
   ```bash
   # Navega a la carpeta donde está el HTML
   cd ruta/a/la/carpeta
   
   # Inicia el servidor
   python -m http.server 8000
   ```

2. **Abre el navegador:**
   - Ve a: `http://localhost:8000`

3. **Configura ThingSpeak en la página:**
   - **Channel ID**: [Tu Channel ID]
   - **Write API Key**: [Tu Write API Key]
   - *(Se guardan automáticamente)*

4. **Selecciona un partido:**
   - Elige una competición del dropdown
   - Click en "📋 Cargar Partidos"
   - Selecciona un partido de la lista
   - Click en "📡 Enviar al ESP32"

#### Opción B: Match ID Manual

1. Busca un Match ID en: https://www.football-data.org/
2. Ingresa el ID en el campo amarillo de la página
3. Click en "🚀 Enviar Directo al ESP32"

### Visualización en el OLED

Una vez enviado el partido, el display mostrará:

```
┌──────────────────────────┐
│ 15 Ene 2024              │  ← Fecha del partido
│ Estado: EN VIVO          │  ← Estado actual
├──────────────────────────┤
│ Man United          3    │  ← Equipo Local + Goles
│ Liverpool           2    │  ← Equipo Visitante + Goles
│ Act: 5m                  │  ← Minutos desde última actualización
└──────────────────────────┘
```

### Estados Posibles

- 🔴 **EN VIVO** - Partido en juego
- ⏸️ **DESCANSO** - Medio tiempo
- ✅ **FINALIZADO** - Partido terminado
- 📅 **PROGRAMADO** - Partido por jugar
- ⏰ **POSPUESTO** - Partido aplazado
- ❌ **CANCELADO** - Partido cancelado

---

## 🏗️ Arquitectura del Sistema

```
┌─────────────────┐
│  Interfaz Web   │
│  (HTML/JS)      │
└────────┬────────┘
         │ HTTP POST
         ▼
┌─────────────────┐
│  ThingSpeak     │ ◄──── Puente de Comunicación
│  (Cloud)        │
└────────┬────────┘
         │ HTTP GET (cada 30s)
         ▼
┌─────────────────┐
│     ESP32       │
│  (Firmware C++) │
└────────┬────────┘
         │ HTTP GET (cada 15 min)
         ▼
┌─────────────────┐
│ Football-Data   │
│     API         │
└────────┬────────┘
         │ JSON Response
         ▼
┌─────────────────┐
│  Display OLED   │
│  (Visualización)│
└─────────────────┘
```

### Flujo de Datos

1. **Usuario** selecciona partido en la interfaz web
2. **Interfaz web** envía Match ID a ThingSpeak (Field 1)
3. **ESP32** consulta ThingSpeak cada 30 segundos
4. **ESP32** detecta nuevo Match ID
5. **ESP32** solicita datos a Football-Data.org API
6. **API** responde con datos del partido (JSON)
7. **ESP32** parsea JSON y extrae información relevante
8. **Display OLED** muestra la información formateada
9. **ESP32** actualiza datos cada 15 minutos

---

## ⚠️ Problemas Encontrados y Soluciones

### 1. Error de Compilación: Parámetro por Defecto Duplicado

**Problema:**
```
error: default argument given for parameter 2 of 'String shortenTeamName(String, int)'
```

**Causa:** El parámetro por defecto `maxLen = 12` estaba definido tanto en la declaración como en la definición de la función.

**Solución:** Mantener el valor por defecto solo en la declaración:
```cpp
// Declaración (con valor por defecto)
String shortenTeamName(String name, int maxLen = 12);

// Definición (sin valor por defecto)
String shortenTeamName(String name, int maxLen) {
    // código...
}
```

### 2. Display OLED No Detectado

**Problema:** El ESP32 no detectaba el display I2C al inicio.

**Causa:** Falta de inicialización explícita del bus I2C.

**Solución:** Agregar inicialización I2C en el `setup()`:
```cpp
Wire.begin(21, 22); // SDA=GPIO21, SCL=GPIO22
```

### 3. Error CORS en la Página Web

**Problema:**
```
Failed to fetch: CORS policy blocked
```

**Causa:** Los navegadores bloquean peticiones HTTP desde archivos locales (`file://`) por seguridad.

**Solución Implementada:** Usar servidor web local con Python:
```bash
python -m http.server 8000
```

**Alternativa en el código:** Implementar proxy CORS:
```javascript
const proxyUrl = `https://corsproxy.io/?${encodeURIComponent(apiUrl)}`;
```

### 4. Caracteres Especiales Mal Mostrados

**Problema:** Nombres como "Barça" aparecían como "Bar�a" en el display.

**Causa:** El display OLED no soporta caracteres UTF-8 nativamente.

**Solución:** Función de limpieza de caracteres especiales:
```cpp
String cleanSpecialChars(String text) {
    text.replace("ç", "c");
    text.replace("ñ", "n");
    text.replace("á", "a");
    // etc...
    return text;
}
```

### 5. Límite de Requests API Alcanzado

**Problema:** Football-Data.org limitaba a 10 requests por minuto.

**Causa:** Actualización muy frecuente (cada 30 segundos).

**Solución:** Ajustar intervalos de actualización:
```cpp
const unsigned long THINGSPEAK_INTERVAL = 30000;    // 30 segundos
const unsigned long STATS_UPDATE_LIVE = 900000;     // 15 minutos
```

**Resultado:** Solo 4 requests por hora (96 por día), muy por debajo del límite.

### 6. APIs de Estadísticas Detalladas

**Problema:** Se requería mostrar corners y tiros a puerta, pero ninguna API gratuita ofrecía esta información.

**APIs Evaluadas:**
- ❌ **Football-Data.org** - Solo marcadores (plan gratuito)
- ❌ **API-Football (RapidAPI)** - Estadísticas solo en plan de pago ($15-50/mes)
- ❌ **Sportmonks** - Trial 14 días, luego $30+/mes
- ❌ **Goalserve** - Solo planes enterprise ($150+/mes)

**Decisión Final:** Usar Football-Data.org sin estadísticas detalladas, enfocándose en:
- ✅ Marcador en tiempo real
- ✅ Estado del partido
- ✅ Fecha del partido
- ✅ Información de equipos

**Justificación Académica:** El proyecto demuestra todas las competencias técnicas requeridas (IoT, APIs, Cloud) independientemente de las estadísticas mostradas.

### 7. Nombres de Equipos Muy Largos

**Problema:** Nombres como "Manchester United" no cabían en el display de 128px.

**Solución:** Función de acortamiento inteligente:
```cpp
String shortenTeamName(String name, int maxLen) {
    // Eliminar palabras comunes
    name.replace(" FC", "");
    name.replace(" United", "");
    
    // Truncar si aún es largo
    if (name.length() > maxLen) {
        return name.substring(0, maxLen - 1) + ".";
    }
    return name;
}
```

**Resultado:** "Manchester United" → "Man Utd" o "Man."

### 8. Zona Horaria de Fechas

**Problema Inicial:** Las fechas mostraban hora UTC, no hora local de Colombia.

**Solución Implementada:** Mostrar solo la fecha del partido (sin hora):
```cpp
String formatDate(String utcDate) {
    // Extraer solo día, mes, año
    // Formato: "15 Ene 2024"
    return day + " " + monthName + " " + year;
}
```

**Beneficio:** Evita confusión con zonas horarias y ahorra espacio en pantalla.

---

## 🚧 Limitaciones

### Técnicas

1. **Display Monocromático** - Solo blanco y negro (no escudos a color)
2. **Resolución Limitada** - 128x64 píxeles (información limitada)
3. **API Gratuita** - Sin estadísticas detalladas (corners, tiros)
4. **Actualización No Instantánea** - Cada 15 minutos (límite de API)
5. **Dependencia de Internet** - Requiere WiFi estable

### De las APIs

| API | Limitación |
|-----|------------|
| **Football-Data.org** | 10 req/min, sin stats detalladas |
| **ThingSpeak** | 15 segundos entre updates |
| **Proxy CORS** | Puede ser lento o caerse ocasionalmente |

### Del Hardware

- **Memoria RAM** - Solo 520 KB (limita tamaño de respuestas JSON)
- **Flash** - Solo espacio para firmware y código básico
- **Display** - No puede mostrar imágenes complejas

---

## 🔮 Mejoras Futuras

### Hardware

- [ ] **Display TFT a Color** - Para mostrar escudos y estadísticas gráficas
- [ ] **Display más grande** - 240x320px para más información
- [ ] **Botones físicos** - Para navegación sin página web
- [ ] **Buzzer** - Alertas sonoras de goles en vivo
- [ ] **Batería** - Sistema portátil

### Software

- [ ] **Múltiples partidos** - Rotar entre varios partidos guardados
- [ ] **Gráficas de rendimiento** - Si se consigue API con stats
- [ ] **Notificaciones push** - Avisar cuando empiece un partido
- [ ] **Modo offline** - Mostrar últimos datos guardados sin internet
- [ ] **Configuración por Bluetooth** - WiFi credentials vía BLE

### Servicios

- [ ] **Backend propio** - Servidor para cache y procesamiento
- [ ] **Base de datos** - Histórico de partidos y estadísticas
- [ ] **App móvil** - Control desde smartphone
- [ ] **Websockets** - Actualizaciones en tiempo real

---

## 📚 Referencias

### Documentación Oficial

- [ESP32 Arduino Core](https://docs.espressif.com/projects/arduino-esp32/)
- [Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306)
- [ArduinoJson Library](https://arduinojson.org/)
- [Football-Data.org API Docs](https://www.football-data.org/documentation/api)
- [ThingSpeak Documentation](https://www.mathworks.com/help/thingspeak/)

### Tutoriales Útiles

- [PlatformIO Getting Started](https://docs.platformio.org/en/latest/tutorials/index.html)
- [ESP32 HTTP Client](https://randomnerdtutorials.com/esp32-http-get-post-arduino/)
- [OLED Display Guide](https://randomnerdtutorials.com/esp32-ssd1306-oled-display-arduino-ide/)

---

## 📄 Licencia

Este proyecto es de código abierto y está disponible bajo la licencia MIT.

---

## 👤 Autor

**Proyecto Académico - IoT con ESP32**

- Curso: Internet de las Cosas
- Institución: [Tu Universidad]
- Fecha: Octubre 2025

---

## 🤝 Contribuciones

Si deseas mejorar este proyecto:

1. Fork el repositorio
2. Crea una rama para tu feature (`git checkout -b feature/mejora`)
3. Commit tus cambios (`git commit -m 'Agrega nueva funcionalidad'`)
4. Push a la rama (`git push origin feature/mejora`)
5. Abre un Pull Request

---

## ⚡ Quick Start (Resumen)

```bash
# 1. Configurar APIs
# - Registrarse en Football-Data.org
# - Crear canal en ThingSpeak

# 2. Actualizar credenciales en main.cpp
# - WiFi SSID y Password
# - ThingSpeak Channel ID y Read API Key
# - Football Data API Key

# 3. Compilar y subir
pio run --target upload

# 4. Abrir monitor serial
pio device monitor

# 5. Iniciar servidor web
python -m http.server 8000

# 6. Abrir navegador
# http://localhost:8000

# 7. ¡Seleccionar partido y disfrutar!
```

---

## 📞 Soporte

Si encuentras problemas o tienes preguntas:

1. Revisa la sección [Problemas Encontrados](#-problemas-encontrados-y-soluciones)
2. Verifica las [Limitaciones](#-limitaciones) conocidas
3. Consulta las [Referencias](#-referencias)
4. Abre un Issue en el repositorio

---

**⚽ ¡Disfruta de tu sistema de estadísticas de fútbol IoT!**