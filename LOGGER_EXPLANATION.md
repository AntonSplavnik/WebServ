# Logger - Explication Complète et Guide d'Intégration

## 📋 À Quoi Sert le Logger ?

Le Logger est un système de **journalisation (logging)** qui enregistre deux types d'événements dans des fichiers séparés :

### 1. **Access Log** (`access.log`)
- **Quoi** : Enregistre TOUTES les requêtes HTTP reçues par le serveur
- **Quand** : À chaque fois qu'une requête est **complètement traitée** (peu importe le résultat)
- **Pourquoi** : 
  - Analyser le trafic du serveur
  - Débugger les problèmes clients
  - Statistiques d'utilisation
  - Audit de sécurité

### 2. **Error Log** (`error.log`)
- **Quoi** : Enregistre les erreurs internes du serveur
- **Quand** : Problèmes de configuration, erreurs système, exceptions
- **Pourquoi** :
  - Diagnostiquer les problèmes serveur
  - Alertes sur les dysfonctionnements
  - Traçabilité des erreurs critiques

---

## 🏗️ Architecture du Logger

### Structure de Classe

```cpp
class Logger {
public:
    // Constructeur : ouvre les fichiers de log
    Logger(const std::string& accessPath, const std::string& errorPath);
    
    // Log une requête HTTP (access log)
    void logAccess(const std::string& clientIp, 
                   const std::string& method, 
                   const std::string& url, 
                   int status, 
                   size_t size);
    
    // Log une erreur serveur (error log)
    void logError(const std::string& level, 
                  const std::string& message);

private:
    std::ofstream _accessLog;  // Fichier access.log
    std::ofstream _errorLog;   // Fichier error.log
    std::string getTimestamp() const;  // Génère timestamp actuel
};
```

### Membres Privés

- **`_accessLog`** : Stream ouvert vers le fichier `access.log` (mode `std::ios::app` = append)
- **`_errorLog`** : Stream ouvert vers le fichier `error.log` (mode append)
- Les fichiers restent **ouverts pendant toute la vie du Logger** (pas de open/close à chaque log)

---

## 📝 Format des Logs

### Access Log Format

```
[timestamp] clientIp "METHOD URL" status responseSize
```

**Exemple réel** :
```
2025-11-18 14:32:15 127.0.0.1 "GET /index.html" 200 1024
2025-11-18 14:32:18 192.168.1.50 "POST /upload" 201 0
2025-11-18 14:32:22 127.0.0.1 "GET /missing.html" 404 512
2025-11-18 14:32:30 127.0.0.1 "DELETE /uploads/file.txt" 204 0
2025-11-18 14:32:45 10.0.0.5 "GET /cgi-bin/script.py" 500 256
```

**Composants** :
- `timestamp` : Format `YYYY-MM-DD HH:MM:SS`
- `clientIp` : Adresse IP du client (obtenue via `getpeername()`)
- `method` : `GET`, `POST`, `DELETE`
- `url` : URI demandée (ex: `/index.html`, `/upload`)
- `status` : Code HTTP (200, 404, 500, etc.)
- `responseSize` : Taille de la réponse en bytes

### Error Log Format

```
[timestamp] [LEVEL] message
```

**Exemple réel** :
```
2025-11-18 14:30:00 [INFO] Server started on port 8080
2025-11-18 14:30:05 [WARNING] CGI timeout for script.py (pid=12345)
2025-11-18 14:30:10 [ERROR] Failed to open file: /var/www/data.txt (Permission denied)
2025-11-18 14:30:15 [ERROR] Invalid configuration: missing root directive
2025-11-18 14:30:20 [WARNING] Client max body size exceeded (10MB limit)
```

**Niveaux (levels)** :
- **`INFO`** : Événements informationnels (démarrage serveur, arrêt propre)
- **`WARNING`** : Problèmes non-critiques (timeout CGI, fichier manquant)
- **`ERROR`** : Erreurs graves (échec système, config invalide, crash)

---

## 🔧 Comment Fonctionne le Logger (Implémentation Actuelle)

### 1. Construction du Logger

```cpp
Logger::Logger(const std::string& accessPath, const std::string& errorPath)
    : _accessLog(accessPath.c_str(), std::ios::app),
      _errorLog(errorPath.c_str(), std::ios::app) {}
```

**Fonctionnement** :
- Ouvre `accessPath` en mode **append** (ne supprime pas l'ancien contenu)
- Ouvre `errorPath` en mode **append**
- Si les fichiers n'existent pas, ils sont **créés automatiquement**
- Les streams restent **ouverts** jusqu'à la destruction du Logger

**Validation Préalable** (fonction helper) :
```cpp
bool assignLogFile(std::string& logField, const std::string& path) {
    // Si le fichier n'existe pas, on le crée
    if (!isValidFile(path, W_OK) && !path.empty()) {
        std::ofstream ofs(path.c_str(), std::ios::app);
        if (!ofs)
            throw ConfigParseException("Cannot create or open log file: " + path);
        std::cout << "Info: Created log file '" << path << "'." << std::endl;
    }
    logField = path;
    return true;
}
```

---

### 2. Génération du Timestamp

```cpp
std::string Logger::getTimestamp() const {
    time_t now = time(0);  // Obtient le temps actuel (secondes depuis 1970)
    char buf[32];
    // Format : YYYY-MM-DD HH:MM:SS
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return buf;
}
```

**Exemple** : `"2025-11-18 14:32:15"`

---

### 3. Log Access (Requête HTTP)

```cpp
void Logger::logAccess(const std::string& clientIp, 
                       const std::string& method,
                       const std::string& url, 
                       int status, 
                       size_t size) {
    _accessLog << getTimestamp() << " " 
               << clientIp << " \"" 
               << method << " " << url << "\" " 
               << status << " " 
               << size << std::endl;
}
```

**Sortie** :
```
2025-11-18 14:32:15 127.0.0.1 "GET /index.html" 200 1024
```

**Note** : `std::endl` **flush** le buffer (force l'écriture immédiate sur disque)

---

### 4. Log Error (Erreur Serveur)

```cpp
void Logger::logError(const std::string& level, 
                      const std::string& message) {
    _errorLog << getTimestamp() << " [" 
              << level << "] " 
              << message << std::endl;
}
```

**Sortie** :
```
2025-11-18 14:30:10 [ERROR] Failed to open file: /var/www/data.txt
```

---

## 📂 Configuration du Logger

### Dans le Fichier de Config (`working.conf`)

```nginx
server {
    listen 0.0.0.0:8080
    
    # Chemins des fichiers de log
    access_log runtime/logs/access.log
    error_log runtime/logs/error.log
    
    # ... autres directives
}
```

### Structure `ConfigData` (dans `config.hpp`)

```cpp
struct ConfigData {
    // ... autres membres
    
    // Logging
    std::string access_log;  // "runtime/logs/access.log"
    std::string error_log;   // "runtime/logs/error.log"
};
```

### Parsing de Config (dans `config.cpp`)

Le parser lit les directives `access_log` et `error_log` et les stocke dans `ConfigData`.

---

## 🚀 Comment Intégrer le Logger (Actuellement Manquant)

### ⚠️ Problème Actuel

**Le Logger existe mais n'est PAS utilisé dans le code de production** :
- ✅ Code implémenté : `logger.cpp`, `logger.hpp`
- ✅ Testé dans `confParsertest.cpp` (fonctionne)
- ❌ **Jamais instancié** dans `EventLoop`, `Server`, ou `Connection`
- ❌ Tous les logs utilisent `std::cout` au lieu du Logger

**Preuve** :
```cpp
// src/cgi/cgi_helpers.cpp:10
//Logger::getInstance().logError("ERROR", err); TODO: use my logger
```

---

## 📍 Où et Comment Intégrer le Logger

### Architecture Recommandée

```
EventLoop
    ├── _configs (vector<ConfigData>)
    ├── _loggers (map<port, Logger*>)  ← À AJOUTER
    └── _connectionPoolManager
            └── Connections
                    └── handleRequest() → logAccess()
```

### Option 1 : Logger Global par Serveur (Recommandé)

**Créer un Logger par block `server {}`** :

#### 1. Ajouter Logger dans `EventLoop`

```cpp
// event_loop.hpp
class EventLoop {
private:
    std::map<unsigned short, Logger*> _loggers;  // Port → Logger
    
public:
    Logger* getLogger(unsigned short port);
};
```

#### 2. Initialiser les Loggers au Démarrage

```cpp
// event_loop.cpp
void EventLoop::initListeningSockets() {
    for (size_t i = 0; i < _configs.size(); i++) {
        ConfigData& config = _configs[i];
        
        // Créer le Logger pour ce serveur
        for (size_t j = 0; j < config.listeners.size(); j++) {
            unsigned short port = config.listeners[j].second;
            
            if (_loggers.find(port) == _loggers.end()) {
                _loggers[port] = new Logger(
                    config.access_log, 
                    config.error_log
                );
                std::cout << "✓ Logger initialized for port " << port 
                          << " (access: " << config.access_log 
                          << ", error: " << config.error_log << ")" << std::endl;
            }
        }
    }
}
```

#### 3. Passer le Logger aux Connections

**Méthode A : Via ConnectionPoolManager**

```cpp
// connection_pool_manager.hpp
class ConnectionPoolManager {
private:
    EventLoop& _eventLoop;  // Référence pour obtenir le Logger
    
public:
    void handleConnectionEvent(int fd, short revents);
};

// connection_pool_manager.cpp
void ConnectionPoolManager::handleConnectionEvent(int fd, short revents) {
    Connection& conn = _connectionPool[fd];
    
    if (revents & POLLOUT) {
        // Après envoi complet de la réponse
        if (conn.sendResponse()) {
            // Obtenir le Logger pour ce port
            Logger* logger = _eventLoop.getLogger(conn.getPort());
            
            if (logger) {
                logger->logAccess(
                    conn.getClientIp(),
                    conn.getMethod(),
                    conn.getUrl(),
                    conn.getStatusCode(),
                    conn.getResponseSize()
                );
            }
        }
    }
}
```

**Méthode B : Injection dans Connection**

```cpp
// connection.hpp
class Connection {
private:
    Logger* _logger;  // Référence au Logger
    
public:
    void setLogger(Logger* logger);
    void completeResponse();  // Logs automatiquement
};

// connection.cpp
void Connection::completeResponse() {
    // ... envoi de la réponse ...
    
    if (_logger) {
        _logger->logAccess(
            _clientIp,
            _request.getMethod(),
            _request.getUrl(),
            _response.getStatusCode(),
            _response.getSize()
        );
    }
}
```

---

### Option 2 : Logger Singleton (Alternative)

**Un seul Logger global pour tout le serveur** :

```cpp
// logger.hpp
class Logger {
public:
    static Logger& getInstance(const std::string& accessPath = "", 
                               const std::string& errorPath = "");
    
    // ... reste identique
    
private:
    Logger(const std::string& accessPath, const std::string& errorPath);
    static Logger* _instance;
};

// logger.cpp
Logger* Logger::_instance = NULL;

Logger& Logger::getInstance(const std::string& accessPath, 
                            const std::string& errorPath) {
    if (!_instance) {
        _instance = new Logger(accessPath, errorPath);
    }
    return *_instance;
}
```

**Utilisation** :
```cpp
// Initialisation (au démarrage du serveur)
Logger::getInstance("runtime/logs/access.log", "runtime/logs/error.log");

// Utilisation partout
Logger::getInstance().logAccess("127.0.0.1", "GET", "/index.html", 200, 1024);
Logger::getInstance().logError("ERROR", "Failed to open file");
```

**⚠️ Inconvénient** : Un seul fichier de log pour tous les serveurs virtuels (pas de séparation par `server_name` ou port).

---

## 📌 Points d'Intégration dans le Code

### 1. **Logs d'Access (logAccess)**

**Quand** : Après l'envoi complet d'une réponse HTTP

**Où** :
- `connection.cpp:sendResponse()` → ligne 104 (quand `_bytesSent == _responseData.length()`)
- `server.cpp:handleClientWrite()` → ligne 436 (même condition)

**Code à ajouter** :
```cpp
// connection.cpp (après ligne 104)
if (_bytesSent == _responseData.length()) {
    // ✅ LOG ACCESS ICI
    if (_logger) {
        _logger->logAccess(
            _clientIp,                    // IP du client
            _request.getMethod(),         // "GET", "POST", "DELETE"
            _request.getUrl(),            // "/index.html"
            _response.getStatusCode(),    // 200, 404, 500, etc.
            _responseData.length()        // Taille de la réponse
        );
    }
    
    if (_shouldClose) {
        // ...
    }
}
```

### 2. **Logs d'Error (logError)**

**Quand** : Événements anormaux ou erreurs internes

#### a) **Démarrage/Arrêt Serveur**

```cpp
// event_loop.cpp:run() (ligne 100)
void EventLoop::run() {
    initListeningSockets();
    
    // ✅ LOG INFO
    if (_logger) {
        _logger->logError("INFO", "Server started successfully");
    }
    
    while (_running && !g_shutdown) {
        // ...
    }
    
    // ✅ LOG INFO
    if (_logger) {
        _logger->logError("INFO", "Server shutting down gracefully");
    }
}
```

#### b) **Timeouts CGI**

```cpp
// event_loop.cpp:checkCgiTimeouts() (ligne 168)
if (isCgiTimedOut(cgiMap, cgiIt->first)) {
    // ✅ LOG WARNING
    if (_logger) {
        std::ostringstream msg;
        msg << "CGI timeout for pid=" << cgiIt->second->getPid() 
            << " (exceeded " << CGI_TIMEOUT << "s)";
        _logger->logError("WARNING", msg.str());
    }
    
    _cgiExecutor.handleCGItimeout(cgiIt->second);
}
```

#### c) **Erreurs de Parsing de Requête**

```cpp
// connection.cpp:readRequest() (ligne 50)
HttpRequest httpRequest;
if (!httpRequest.parseRequest(_requestData)) {
    // ✅ LOG ERROR
    if (_logger) {
        std::ostringstream msg;
        msg << "Invalid HTTP request from " << _clientIp;
        _logger->logError("ERROR", msg.str());
    }
    return false;
}
```

#### d) **Erreurs Fichiers**

```cpp
// http_response.cpp:generateResponse()
std::ifstream file(filePath.c_str());
if (!file.is_open()) {
    // ✅ LOG WARNING
    if (_logger) {
        std::ostringstream msg;
        msg << "File not found: " << filePath << " (404)";
        _logger->logError("WARNING", msg.str());
    }
    generateErrorResponse(404);
}
```

#### e) **Erreurs Configuration**

```cpp
// config.cpp:parseConfigFile()
if (!isValidDirective(key)) {
    // ✅ LOG ERROR
    Logger::getInstance().logError("ERROR", 
        "Invalid directive in config: " + key);
    throw ConfigParseException("Invalid directive");
}
```

#### f) **Client Max Body Size Dépassé**

```cpp
// connection.cpp (vérification Content-Length)
if (contentLength > _maxBodySize) {
    // ✅ LOG WARNING
    if (_logger) {
        std::ostringstream msg;
        msg << "Client " << _clientIp << " exceeded max body size ("
            << contentLength << " > " << _maxBodySize << ")";
        _logger->logError("WARNING", msg.str());
    }
    generateErrorResponse(413);  // Payload Too Large
}
```

---

## 🔍 Récupération de l'IP Client

**Problème** : Actuellement, l'IP client n'est pas stockée.

### Solution : Utiliser `getpeername()`

```cpp
// connection.cpp (au moment de accept() ou dans constructeur)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

std::string Connection::getClientIpFromSocket(int fd) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    
    if (getpeername(fd, (struct sockaddr*)&addr, &len) == 0) {
        return inet_ntoa(addr.sin_addr);  // Convertit en "xxx.xxx.xxx.xxx"
    }
    return "unknown";
}
```

**Appel** :
```cpp
// Lors de la création de Connection
Connection::Connection(int fd, /* ... */) 
    : _fd(fd),
      _clientIp(getClientIpFromSocket(fd)),  // ✅ Récupère l'IP
      // ...
{}
```

---

## 📊 Exemple d'Utilisation Complète

### Scénario : Requête GET Réussie

```cpp
// 1. Client se connecte
Connection conn(clientFd, port);  
// → _clientIp = "192.168.1.100" (via getpeername)

// 2. Requête reçue et parsée
conn.readRequest();
// → Method: "GET", URL: "/index.html"

// 3. Réponse générée
HttpResponse response(request);
response.generateResponse(200);
// → Status: 200, Size: 2048 bytes

// 4. Réponse envoyée
conn.sendResponse();
// → Complete

// 5. Log Access
logger->logAccess(
    "192.168.1.100",  // clientIp
    "GET",            // method
    "/index.html",    // url
    200,              // status
    2048              // size
);
```

**Fichier `access.log`** :
```
2025-11-18 15:30:45 192.168.1.100 "GET /index.html" 200 2048
```

---

### Scénario : Timeout CGI

```cpp
// 1. CGI détecté comme timed out
if (isCgiTimedOut(cgiMap, fd)) {
    
    // 2. Log Warning
    std::ostringstream msg;
    msg << "CGI timeout for script=" << cgi->getScriptPath()
        << " pid=" << cgi->getPid() 
        << " (exceeded 20s)";
    logger->logError("WARNING", msg.str());
    
    // 3. Kill process
    cgi->terminate();
}
```

**Fichier `error.log`** :
```
2025-11-18 15:31:00 [WARNING] CGI timeout for script=script.py pid=12345 (exceeded 20s)
```

---

## 🎯 Résumé : Checklist d'Intégration

### Phase 1 : Infrastructure

- [ ] **Ajouter membre `Logger*` à `EventLoop`**
  - `std::map<unsigned short, Logger*> _loggers;`
  
- [ ] **Initialiser Loggers dans `initListeningSockets()`**
  - Créer un Logger par serveur virtuel
  - Utiliser `ConfigData.access_log` et `ConfigData.error_log`

- [ ] **Ajouter `_clientIp` à `Connection`**
  - Récupérer via `getpeername()` lors de `accept()`
  - Stocker comme `std::string _clientIp;`

- [ ] **Passer Logger à `Connection`**
  - Via `setLogger(Logger*)` ou constructeur

### Phase 2 : Logs Access

- [ ] **Log après envoi complet de réponse**
  - `connection.cpp:sendResponse()` ligne ~104
  - `server.cpp:handleClientWrite()` ligne ~436
  
- [ ] **Paramètres requis** :
  - `clientIp` : de `Connection._clientIp`
  - `method` : de `HttpRequest.getMethod()`
  - `url` : de `HttpRequest.getUrl()`
  - `status` : de `HttpResponse.getStatusCode()`
  - `size` : de `_responseData.length()`

### Phase 3 : Logs Error

- [ ] **Démarrage serveur** : `[INFO] Server started`
- [ ] **Arrêt serveur** : `[INFO] Server shutting down`
- [ ] **Timeout CGI** : `[WARNING] CGI timeout for pid=XXX`
- [ ] **Fichier manquant** : `[WARNING] File not found: path`
- [ ] **Requête invalide** : `[ERROR] Invalid HTTP request`
- [ ] **Body size dépassé** : `[WARNING] Client exceeded max body size`
- [ ] **Erreur config** : `[ERROR] Invalid directive: key`
- [ ] **Erreur socket** : `[ERROR] Socket error: strerror(errno)`

### Phase 4 : Nettoyage

- [ ] **Remplacer `std::cout` debug par Logger**
  - 60+ occurences identifiées
  - Garder seulement les logs critiques

- [ ] **Destructeur** : Fermer les streams
  ```cpp
  EventLoop::~EventLoop() {
      for (map<unsigned short, Logger*>::iterator it = _loggers.begin();
           it != _loggers.end(); ++it) {
          delete it->second;
      }
  }
  ```

---

## 📁 Structure Finale des Logs

```
runtime/
├── logs/
│   ├── access.log           # Tous les accès HTTP du serveur principal
│   ├── error.log            # Erreurs du serveur principal
│   ├── admin_access.log     # Accès au serveur admin (port 9090)
│   └── admin_error.log      # Erreurs du serveur admin
└── www/
    └── ...
```

---

## 🔄 Correspondance avec Nginx/Apache

### Nginx Access Log
```
127.0.0.1 - - [18/Nov/2025:15:30:45 +0000] "GET /index.html HTTP/1.1" 200 2048 "-" "Mozilla/5.0"
```

### Votre Access Log (Simplifié)
```
2025-11-18 15:30:45 127.0.0.1 "GET /index.html" 200 2048
```

**Différences** :
- ❌ Pas de user authentication (`- -`)
- ❌ Pas de HTTP version (`HTTP/1.1`)
- ❌ Pas de Referer ni User-Agent
- ✅ Format plus simple, suffisant pour WebServ

---

## ⚠️ Pièges à Éviter

### 1. **Ne PAS log AVANT la réponse complète**
```cpp
// ❌ MAUVAIS
void handleClientWrite() {
    logger->logAccess(...);  // Trop tôt !
    send(...);
}

// ✅ BON
void handleClientWrite() {
    send(...);
    if (envoyé_complet) {
        logger->logAccess(...);  // Après confirmation
    }
}
```

### 2. **Ne PAS oublier de flush**
Le Logger utilise déjà `std::endl` qui flush automatiquement.

### 3. **Ne PAS logger les erreurs normales dans error.log**
```cpp
// ❌ MAUVAIS : 404 est une réponse normale
logger->logError("ERROR", "404 Not Found");

// ✅ BON : Log seulement dans access.log
logger->logAccess(ip, "GET", "/missing.html", 404, 512);
```

### 4. **Gérer les paths de fichiers vides**
```cpp
// config.cpp - Valeurs par défaut
ConfigData::ConfigData() {
    access_log = "runtime/logs/access.log";  // Défaut
    error_log = "runtime/logs/error.log";
}
```

### 5. **Thread-safety (pas nécessaire en C++98 single-thread)**
Votre serveur utilise `poll()` single-thread, donc pas de problème de concurrence.

---

## 🚀 Ordre d'Implémentation Recommandé

1. **Ajouter `_clientIp` à Connection** (30min)
2. **Créer Loggers dans EventLoop** (1h)
3. **Log Access dans sendResponse()** (30min)
4. **Tester avec curl et vérifier access.log** (15min)
5. **Ajouter logs Error critiques** (1h)
6. **Remplacer std::cout par Logger** (2h)
7. **Tests complets** (30min)

**Total : ~6h de travail**

---

## ✅ Validation Finale

### Test 1 : Access Log
```bash
# Lancer le serveur
./webserv conf/working.conf

# Envoyer requêtes
curl http://localhost:8080/index.html
curl http://localhost:8080/missing.html
curl -X POST http://localhost:8080/upload -d "test=data"

# Vérifier les logs
cat runtime/logs/access.log
```

**Attendu** :
```
2025-11-18 15:30:00 127.0.0.1 "GET /index.html" 200 1024
2025-11-18 15:30:05 127.0.0.1 "GET /missing.html" 404 512
2025-11-18 15:30:10 127.0.0.1 "POST /upload" 201 0
```

### Test 2 : Error Log
```bash
# Vérifier les logs d'erreur
cat runtime/logs/error.log
```

**Attendu** :
```
2025-11-18 15:29:55 [INFO] Server started successfully
2025-11-18 15:30:05 [WARNING] File not found: runtime/www/missing.html (404)
```

---

**Fin du Document - Logger Expliqué et Prêt à Intégrer ! 📝**
