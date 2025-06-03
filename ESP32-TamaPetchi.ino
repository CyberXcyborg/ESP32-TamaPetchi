#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Replace with your network credentials
const char* ssid = "TEST";
const char* password = "TEST";

// Web server
WebServer server(80);

// Pet variables
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 60000;  // Update pet stats every minute

// Add this global variable to track wake-up message display
bool showWakeMessage = false;
unsigned long wakeMessageStartTime = 0;
String previousState = "";

// Pet stats (0-100)
struct {
  int hunger = 50;
  int happiness = 50;
  int health = 80;
  int energy = 100;
  int cleanliness = 80;
  int age = 0;  // Age in minutes
  bool isAlive = true;
  String state = "normal";  // normal, eating, playing, sleeping, sick, dead
  unsigned long stateChangeTime = 0;
  unsigned long lastInteractionTime = 0;
} pet;

// Forward declarations for functions
void loadPetData();
void savePetData();
void handleRoot();
void handleGetPet();
void handleFeed();
void handlePlay();
void handleClean();
void handleSleep();
void handleHeal();
void handleReset();
void handleUpdate();
void feedPet();
void sendJsonResponse(bool success, String message);

void setup() {
  Serial.begin(115200);

  // Initialize SPIFFS (for storing pet data)
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
    return;
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  // Try to load pet data from SPIFFS
  loadPetData();
  
  // Store initial state
  previousState = pet.state;

  // Define web server routes
  server.on("/", handleRoot);
  server.on("/pet", HTTP_GET, handleGetPet);
  server.on("/feed", HTTP_POST, handleFeed);
  server.on("/play", HTTP_POST, handlePlay);
  server.on("/clean", HTTP_POST, handleClean);
  server.on("/sleep", HTTP_POST, handleSleep);
  server.on("/heal", HTTP_POST, handleHeal);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/update", HTTP_GET, handleUpdate);  // For AJAX updates

  // Start server
  server.begin();
}

void loop() {
  server.handleClient();

  // Update pet stats every minute
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentMillis;
    updatePet();
    savePetData();
  }
}

void updatePet() {
  // Store previous state before any updates
  previousState = pet.state;
  
  if (!pet.isAlive) return;

  // Handle different stat decreases based on sleep state
  if (pet.state == "sleeping") {
    // When sleeping:
    // - Hunger decreases at half rate
    // - Happiness decreases at half rate
    // - Energy increases
    pet.hunger = max(0, pet.hunger - 2);      // Reduced from 5 to 2
    pet.happiness = max(0, pet.happiness - 1); // Reduced from 3 to 1
    pet.energy = min(100, pet.energy + 10);    // Increase energy when sleeping
    pet.cleanliness = max(0, pet.cleanliness - 2); // Reduced from 4 to 2
    
    // Auto wake-up condition: when energy is full
    if (pet.energy >= 100) {
      pet.state = "normal";
      pet.stateChangeTime = millis();
      // Set flag to show wake message
      showWakeMessage = true;
      wakeMessageStartTime = millis();
      Serial.println("Pet woke up automatically after full rest");
    }
  } else {
    // When not sleeping, normal stat decreases
    pet.hunger = max(0, pet.hunger - 5);
    pet.happiness = max(0, pet.happiness - 3);
    pet.energy = max(0, pet.energy - 2);
    pet.cleanliness = max(0, pet.cleanliness - 4);
  }

  // Health decreases if other stats are too low
  if (pet.hunger < 20 || pet.cleanliness < 20) {
    pet.health = max(0, pet.health - 5);
  }

  // Check if pet is sick
  if (pet.health < 30 && pet.state != "sick" && pet.state != "dead" && pet.state != "sleeping") {
    pet.state = "sick";
    pet.stateChangeTime = millis();
  }

  // Check if pet is hungry
  else if (pet.hunger < 20 && pet.state != "sick" && pet.state != "dead" && pet.state != "sleeping") {
    pet.state = "hungry";
    pet.stateChangeTime = millis();
  }

  // Return to normal state after some time
  else if ((pet.state == "eating" || pet.state == "playing") && millis() - pet.stateChangeTime > 5000) {
    pet.state = "normal";
  }

  // Check if pet died
  if (pet.health <= 0) {
    pet.isAlive = false;
    pet.state = "dead";
  }

  // Increase age
  pet.age++;
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 TamaPetchi - Enhanced Blob Pet</title>
    <link href="https://cdn.jsdelivr.net/npm/tailwindcss@2.2.19/dist/tailwind.min.css" rel="stylesheet">
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/@fortawesome/fontawesome-free@6.4.0/css/all.min.css">
    <style>
        body {
            font-family: 'Comic Sans MS', cursive, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
        }
        
        .pet-container {
            background: linear-gradient(145deg, #e0f0ff, #b0d7ff);
            border-radius: 20px;
            box-shadow: 
                inset 8px 8px 15px rgba(255,255,255,0.3),
                inset -8px -8px 15px rgba(0,0,0,0.1),
                0 10px 30px rgba(0,0,0,0.2);
            transition: all 0.5s ease;
            position: relative;
            overflow: hidden;
        }
        
        .pet-sprite {
            transition: all 0.5s ease;
            filter: drop-shadow(2px 2px 4px rgba(0,0,0,0.2));
        }
        
        .stat-bar {
            background: linear-gradient(90deg, #f0f0f0, #e0e0e0);
            border-radius: 15px;
            overflow: hidden;
            box-shadow: inset 2px 2px 5px rgba(0,0,0,0.1);
        }
        
        .stat-fill {
            height: 100%;
            border-radius: 15px;
            transition: width 0.8s ease;
            position: relative;
            overflow: hidden;
        }
        
        .stat-fill::after {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: linear-gradient(90deg, 
                rgba(255,255,255,0.2) 0%, 
                rgba(255,255,255,0.4) 50%, 
                rgba(255,255,255,0.2) 100%);
            animation: shimmer 2s infinite;
        }
        
        @keyframes shimmer {
            0% { transform: translateX(-100%); }
            100% { transform: translateX(100%); }
        }
        
        .hunger-fill { background: linear-gradient(90deg, #ff9800, #ff5722); }
        .happiness-fill { background: linear-gradient(90deg, #ffeb3b, #ffc107); }
        .health-fill { background: linear-gradient(90deg, #4caf50, #2e7d32); }
        .energy-fill { background: linear-gradient(90deg, #2196f3, #1565c0); }
        .cleanliness-fill { background: linear-gradient(90deg, #00bcd4, #0097a7); }
        
        .game-button {
            background: linear-gradient(145deg, #ffffff, #e0e0e0);
            border: none;
            border-radius: 15px;
            box-shadow: 
                8px 8px 15px rgba(0,0,0,0.1),
                -8px -8px 15px rgba(255,255,255,0.8);
            transition: all 0.2s ease;
            color: #333;
            font-weight: bold;
        }
        
        .game-button:hover {
            transform: translateY(-2px);
            box-shadow: 
                10px 10px 20px rgba(0,0,0,0.15),
                -10px -10px 20px rgba(255,255,255,0.9);
        }
        
        .game-button:active {
            transform: translateY(1px);
            box-shadow: 
                4px 4px 8px rgba(0,0,0,0.2),
                -4px -4px 8px rgba(255,255,255,0.6);
        }
        
        .game-button:disabled {
            opacity: 0.5;
            cursor: not-allowed;
            transform: none;
        }
        
        /* Pet Animations */
        .bounce {
            animation: bounce 2s infinite ease-in-out;
        }
        @keyframes bounce {
            0%, 100% { transform: translateY(0); }
            50% { transform: translateY(-8px); }
        }
        
        .wobble {
            animation: wobble 3s infinite ease-in-out;
        }
        @keyframes wobble {
            0%, 100% { transform: rotate(0deg) scale(1); }
            25% { transform: rotate(-2deg) scale(1.02); }
            75% { transform: rotate(2deg) scale(0.98); }
        }
        
        .pulse {
            animation: pulse 2s infinite ease-in-out;
        }
        @keyframes pulse {
            0%, 100% { transform: scale(1); }
            50% { transform: scale(1.05); }
        }
        
        .shake {
            animation: shake 0.8s infinite;
        }
        @keyframes shake {
            0%, 100% { transform: translateX(0) rotate(0deg); }
            25% { transform: translateX(-3px) rotate(-1deg); }
            75% { transform: translateX(3px) rotate(1deg); }
        }
        
        .eating-motion {
            animation: eating 1s infinite alternate;
        }
        @keyframes eating {
            0% { transform: scaleY(1); }
            100% { transform: scaleY(0.95) translateY(2px); }
        }
        
        .playing-motion {
            animation: playing 1.5s infinite ease-in-out;
        }
        @keyframes playing {
            0%, 100% { transform: translateY(0) rotate(0deg); }
            25% { transform: translateY(-12px) rotate(-3deg); }
            50% { transform: translateY(-15px) rotate(0deg); }
            75% { transform: translateY(-12px) rotate(3deg); }
        }
        
        .sleeping-motion {
            animation: sleeping 4s infinite ease-in-out;
        }
        @keyframes sleeping {
            0%, 100% { transform: scale(1) rotate(0deg); }
            50% { transform: scale(1.02) rotate(1deg); }
        }
        
        .dead-motion {
            animation: dead 3s infinite ease-in-out;
        }
        @keyframes dead {
            0%, 100% { transform: rotate(-2deg); }
            50% { transform: rotate(-4deg); }
        }
        
        /* Environment Effects */
        .environment-normal {
            background: linear-gradient(145deg, #e0f0ff, #b0d7ff);
        }
        
        .environment-eating {
            background: linear-gradient(145deg, #fff5e0, #ffd7b0);
        }
        
        .environment-playing {
            background: linear-gradient(145deg, #e0ffe0, #b0ffb0);
        }
        
        .environment-sleeping {
            background: linear-gradient(145deg, #e0e0ff, #b0b0ff);
        }
        
        .environment-sick {
            background: linear-gradient(145deg, #ffe0e0, #ffb0b0);
        }
        
        .environment-dead {
            background: linear-gradient(145deg, #e0e0e0, #b0b0b0);
        }
        
        /* Particles */
        .particle {
            position: absolute;
            border-radius: 50%;
            pointer-events: none;
            z-index: 10;
        }
        
        .food-particle {
            background: #ff9800;
            animation: food-fall 2s ease-in forwards;
        }
        @keyframes food-fall {
            0% { transform: translateY(-20px) rotate(0deg); opacity: 1; }
            100% { transform: translateY(80px) rotate(360deg); opacity: 0; }
        }
        
        .play-particle {
            animation: play-burst 1.5s ease-out forwards;
        }
        @keyframes play-burst {
            0% { transform: scale(0) rotate(0deg); opacity: 1; }
            100% { transform: scale(2) rotate(180deg); opacity: 0; }
        }
        
        .heal-particle {
            background: #4caf50;
            animation: heal-sparkle 2s ease-out forwards;
        }
        @keyframes heal-sparkle {
            0% { transform: scale(0) translateY(0); opacity: 1; }
            50% { opacity: 1; }
            100% { transform: scale(1.5) translateY(-30px); opacity: 0; }
        }
        
        .sleep-z {
            font-size: 24px;
            color: #666;
            opacity: 0;
            animation: sleep-float 3s infinite ease-in-out;
        }
        @keyframes sleep-float {
            0% { transform: translate(0, 0) scale(0.5); opacity: 0; }
            25% { opacity: 1; }
            50% { transform: translate(20px, -20px) scale(1); }
            75% { opacity: 1; }
            100% { transform: translate(40px, -40px) scale(1.5); opacity: 0; }
        }
        
        .message {
            transition: all 0.3s ease;
            border-radius: 10px;
            box-shadow: 0 4px 15px rgba(0,0,0,0.1);
        }
        
        .success {
            background: linear-gradient(135deg, #c8e6c9, #a5d6a7);
            color: #2e7d32;
            border: 2px solid #4caf50;
        }
        
        .error {
            background: linear-gradient(135deg, #ffcdd2, #f8bbd9);
            color: #c62828;
            border: 2px solid #f44336;
        }
        
        .glow-effect {
            filter: drop-shadow(0 0 10px rgba(255, 193, 7, 0.6));
        }
        
        .sick-effect {
            filter: hue-rotate(60deg) saturate(0.7);
        }
        
        @media (max-width: 640px) {
            .game-button {
                padding: 8px 12px;
                font-size: 14px;
            }
        }
    </style>
</head>
<body class="bg-gradient-to-br from-purple-400 via-pink-500 to-red-500 min-h-screen p-4">
    <div class="max-w-md mx-auto bg-white rounded-3xl shadow-2xl overflow-hidden">
        <!-- Header -->
        <div class="bg-gradient-to-r from-purple-600 to-blue-600 text-white p-6 text-center">
            <h1 class="text-2xl font-bold flex items-center justify-center gap-2">
                <i class="fas fa-heart text-pink-300"></i>
                ESP32 TamaPetchi
                <i class="fas fa-star text-yellow-300"></i>
            </h1>
        </div>
        
        <!-- Message Area -->
        <div id="message" class="message mx-4 mt-4 p-3 hidden">
            <span id="message-text"></span>
        </div>
        
        <!-- Pet Display -->
        <div class="p-6">
            <div id="pet-container" class="pet-container w-64 h-64 mx-auto flex items-center justify-center relative">
                <div id="pet" class="pet-sprite">
                    <!-- Pet SVG will be inserted here -->
                </div>
                <!-- Particles container -->
                <div id="particles-container" class="absolute inset-0 pointer-events-none"></div>
            </div>
        </div>
        
        <!-- Stats Display -->
        <div class="px-6 pb-4 space-y-3">
            <!-- Hunger -->
            <div class="flex items-center gap-3">
                <i class="fas fa-utensils text-orange-500 w-6"></i>
                <div class="flex-1">
                    <div class="flex justify-between text-sm text-gray-600 mb-1">
                        <span>Hunger</span>
                        <span id="hunger-value">100%</span>
                    </div>
                    <div class="stat-bar h-4">
                        <div id="hunger-bar" class="stat-fill hunger-fill" style="width: 100%"></div>
                    </div>
                </div>
            </div>
            
            <!-- Happiness -->
            <div class="flex items-center gap-3">
                <i class="fas fa-smile text-yellow-500 w-6"></i>
                <div class="flex-1">
                    <div class="flex justify-between text-sm text-gray-600 mb-1">
                        <span>Happiness</span>
                        <span id="happiness-value">100%</span>
                    </div>
                    <div class="stat-bar h-4">
                        <div id="happiness-bar" class="stat-fill happiness-fill" style="width: 100%"></div>
                    </div>
                </div>
            </div>
            
            <!-- Health -->
            <div class="flex items-center gap-3">
                <i class="fas fa-heartbeat text-green-500 w-6"></i>
                <div class="flex-1">
                    <div class="flex justify-between text-sm text-gray-600 mb-1">
                        <span>Health</span>
                        <span id="health-value">100%</span>
                    </div>
                    <div class="stat-bar h-4">
                        <div id="health-bar" class="stat-fill health-fill" style="width: 100%"></div>
                    </div>
                </div>
            </div>
            
            <!-- Energy -->
            <div class="flex items-center gap-3">
                <i class="fas fa-bolt text-blue-500 w-6"></i>
                <div class="flex-1">
                    <div class="flex justify-between text-sm text-gray-600 mb-1">
                        <span>Energy</span>
                        <span id="energy-value">100%</span>
                    </div>
                    <div class="stat-bar h-4">
                        <div id="energy-bar" class="stat-fill energy-fill" style="width: 100%"></div>
                    </div>
                </div>
            </div>
            
            <!-- Cleanliness -->
            <div class="flex items-center gap-3">
                <i class="fas fa-soap text-cyan-500 w-6"></i>
                <div class="flex-1">
                    <div class="flex justify-between text-sm text-gray-600 mb-1">
                        <span>Cleanliness</span>
                        <span id="cleanliness-value">100%</span>
                    </div>
                    <div class="stat-bar h-4">
                        <div id="cleanliness-bar" class="stat-fill cleanliness-fill" style="width: 100%"></div>
                    </div>
                </div>
            </div>
            
            <!-- Age -->
            <div class="text-center text-gray-600 mt-4">
                <i class="fas fa-clock text-purple-500"></i>
                Age: <span id="age-value" class="font-bold">0</span> minutes
            </div>
        </div>
        
        <!-- Action Buttons -->
        <div class="p-6 grid grid-cols-2 gap-3">
            <button id="feed-btn" class="game-button p-3 flex items-center justify-center gap-2 text-orange-600 hover:text-orange-700">
                <i class="fas fa-utensils"></i>
                <span>Feed</span>
            </button>
            
            <button id="play-btn" class="game-button p-3 flex items-center justify-center gap-2 text-green-600 hover:text-green-700">
                <i class="fas fa-gamepad"></i>
                <span>Play</span>
            </button>
            
            <button id="clean-btn" class="game-button p-3 flex items-center justify-center gap-2 text-cyan-600 hover:text-cyan-700">
                <i class="fas fa-soap"></i>
                <span>Clean</span>
            </button>
            
            <button id="sleep-btn" class="game-button p-3 flex items-center justify-center gap-2 text-purple-600 hover:text-purple-700">
                <i class="fas fa-bed"></i>
                <span id="sleep-text">Sleep</span>
            </button>
            
            <button id="heal-btn" class="game-button p-3 flex items-center justify-center gap-2 text-pink-600 hover:text-pink-700">
                <i class="fas fa-heart-pulse"></i>
                <span>Heal</span>
            </button>
            
            <button id="reset-btn" class="game-button p-3 flex items-center justify-center gap-2 text-red-600 hover:text-red-700">
                <i class="fas fa-refresh"></i>
                <span>Reset</span>
            </button>
        </div>
    </div>

    <script>
        // Pet state management
        let pet = {
            hunger: 100,
            happiness: 100,
            health: 100,
            energy: 100,
            cleanliness: 100,
            age: 0,
            isAlive: true,
            state: "normal"
        };
        
        let updateInterval;
        let previousState = "";
        let animationTimeouts = [];
        
        // Initialize the game
        window.onload = function() {
            setupEventListeners();
            updatePetData();
            updateInterval = setInterval(updatePetData, 5000);
        };
        
        function setupEventListeners() {
            document.getElementById('feed-btn').addEventListener('click', () => performAction('feed'));
            document.getElementById('play-btn').addEventListener('click', () => performAction('play'));
            document.getElementById('clean-btn').addEventListener('click', () => performAction('clean'));
            document.getElementById('sleep-btn').addEventListener('click', () => performAction('sleep'));
            document.getElementById('heal-btn').addEventListener('click', () => performAction('heal'));
            document.getElementById('reset-btn').addEventListener('click', resetPet);
        }
        
        // Fetch pet data from server
        function updatePetData() {
            fetch('/pet')
                .then(response => response.json())
                .then(data => {
                    if (previousState !== data.state) {
                        handleStateChange(previousState, data.state);
                    }
                    previousState = data.state;
                    pet = data;
                    updateDisplay();
                })
                .catch(error => {
                    console.error('Error fetching pet data:', error);
                    showMessage('Connection error. Check if device is online.', 'error');
                });
        }
        
        // Handle state changes and trigger animations
        function handleStateChange(oldState, newState) {
            const container = document.getElementById('pet-container');
            container.className = `pet-container w-64 h-64 mx-auto flex items-center justify-center relative environment-${newState}`;
            
            clearAnimationTimeouts();
            
            switch(newState) {
                case 'eating':
                    createFoodParticles();
                    break;
                case 'playing':
                    createPlayParticles();
                    break;
                case 'sleeping':
                    if (oldState !== 'sleeping') {
                        createSleepEffects();
                    }
                    break;
                case 'normal':
                    if (oldState === 'sleeping') {
                        showMessage('Your pet woke up feeling refreshed!', 'success');
                    }
                    break;
            }
        }
        
        // Update the visual display
        function updateDisplay() {
            updateStatBars();
            updatePetSprite();
            updateButtons();
            
            if (!pet.isAlive) {
                showMessage('Your pet has passed away. Click Reset to start over.', 'error');
            }
        }
        
        function updateStatBars() {
            const stats = ['hunger', 'happiness', 'health', 'energy', 'cleanliness'];
            stats.forEach(stat => {
                document.getElementById(`${stat}-bar`).style.width = `${pet[stat]}%`;
                document.getElementById(`${stat}-value`).textContent = `${pet[stat]}%`;
            });
            document.getElementById('age-value').textContent = pet.age;
        }
        
        function updatePetSprite() {
            const petElement = document.getElementById('pet');
            petElement.innerHTML = generatePetSVG();
            
            // Remove existing animation classes
            petElement.className = 'pet-sprite';
            
            // Add state-specific animations
            if (!pet.isAlive) {
                petElement.classList.add('dead-motion');
            } else {
                switch(pet.state) {
                    case 'eating':
                        petElement.classList.add('eating-motion');
                        break;
                    case 'playing':
                        petElement.classList.add('playing-motion');
                        break;
                    case 'sleeping':
                        petElement.classList.add('sleeping-motion');
                        break;
                    case 'sick':
                        petElement.classList.add('shake', 'sick-effect');
                        break;
                    case 'hungry':
                        petElement.classList.add('pulse');
                        break;
                    default: // normal
                        if (pet.happiness > 70) {
                            petElement.classList.add('bounce', 'glow-effect');
                        } else if (pet.happiness > 30) {
                            petElement.classList.add('wobble');
                        } else {
                            petElement.classList.add('pulse');
                        }
                }
            }
        }
        
        function generatePetSVG() {
            if (!pet.isAlive) {
                return generateDeadPet();
            }
            
            switch(pet.state) {
                case 'eating': return generateEatingPet();
                case 'playing': return generatePlayingPet();
                case 'sleeping': return generateSleepingPet();
                case 'sick': return generateSickPet();
                case 'hungry': return generateHungryPet();
                default: return generateNormalPet();
            }
        }
        
        function generateNormalPet() {
            const happinessLevel = pet.happiness;
            const mouthPath = happinessLevel > 70 ? 
                "M42,58 Q50,52 58,58" : // Happy smile
                happinessLevel > 30 ? 
                "M45,60 L55,60" : // Neutral
                "M42,62 Q50,68 58,62"; // Sad
            
            const eyeColor = happinessLevel > 50 ? "black" : "#444";
            const glowIntensity = Math.max(0, (happinessLevel - 50) / 50);
            
            return `
                <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite" width="120" height="120">
                    <!-- Head + Body (Blob Style) -->
                    <ellipse cx="50" cy="50" rx="28" ry="32" fill="#FFCC66" />
                    
                    <!-- Belly -->
                    <ellipse cx="50" cy="60" rx="12" ry="16" fill="#FFE5A0" />
                    
                    <!-- Eyes -->
                    <circle cx="40" cy="42" r="5" fill="${eyeColor}" />
                    <circle cx="60" cy="42" r="5" fill="${eyeColor}" />
                    <circle cx="39" cy="41" r="1.5" fill="white" />
                    <circle cx="59" cy="41" r="1.5" fill="white" />
                    
                    <!-- Mouth -->
                    <path d="${mouthPath}" stroke="black" stroke-width="2" fill="none" />
                    
                    <!-- Blush (more intense when happy) -->
                    <ellipse cx="32" cy="48" rx="3" ry="1.5" fill="pink" opacity="${0.3 + glowIntensity * 0.4}" />
                    <ellipse cx="68" cy="48" rx="3" ry="1.5" fill="pink" opacity="${0.3 + glowIntensity * 0.4}" />
                    
                    <!-- Feet -->
                    <ellipse cx="40" cy="80" rx="4" ry="2" fill="#AA8844" />
                    <ellipse cx="60" cy="80" rx="4" ry="2" fill="#AA8844" />
                    
                    <!-- Ears -->
                    <ellipse cx="30" cy="28" rx="5" ry="8" fill="#FFCC66" />
                    <ellipse cx="70" cy="28" rx="5" ry="8" fill="#FFCC66" />
                    
                    ${happinessLevel > 80 ? `
                        <!-- Happy sparkles -->
                        <circle cx="25" cy="25" r="2" fill="#FFD700" opacity="0.8" />
                        <circle cx="75" cy="30" r="2" fill="#FFD700" opacity="0.8" />
                        <circle cx="20" cy="65" r="1.5" fill="#FFD700" opacity="0.8" />
                    ` : ''}
                </svg>
            `;
        }
        
        function generateEatingPet() {
            return `
                <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite" width="120" height="120">
                    <!-- Head + Body -->
                    <ellipse cx="50" cy="52" rx="28" ry="30" fill="#FFCC66" />
                    
                    <!-- Belly -->
                    <ellipse cx="50" cy="62" rx="12" ry="14" fill="#FFE5A0" />
                    
                    <!-- Eyes (focused on food) -->
                    <circle cx="40" cy="42" r="5" fill="black" />
                    <circle cx="60" cy="42" r="5" fill="black" />
                    <circle cx="41" cy="40" r="1.5" fill="white" />
                    <circle cx="61" cy="40" r="1.5" fill="white" />
                    
                    <!-- Eating mouth (open) -->
                    <ellipse cx="50" cy="58" rx="6" ry="4" fill="black" />
                    <ellipse cx="50" cy="57" rx="4" ry="2" fill="#FF6B6B" />
                    
                    <!-- Blush -->
                    <ellipse cx="32" cy="48" rx="3" ry="1.5" fill="pink" opacity="0.6" />
                    <ellipse cx="68" cy="48" rx="3" ry="1.5" fill="pink" opacity="0.6" />
                    
                    <!-- Feet -->
                    <ellipse cx="40" cy="80" rx="4" ry="2" fill="#AA8844" />
                    <ellipse cx="60" cy="80" rx="4" ry="2" fill="#AA8844" />
                    
                    <!-- Ears -->
                    <ellipse cx="30" cy="28" rx="5" ry="8" fill="#FFCC66" />
                    <ellipse cx="70" cy="28" rx="5" ry="8" fill="#FFCC66" />
                    
                    <!-- Food near mouth -->
                    <circle cx="45" cy="45" r="3" fill="#FF8C00" />
                    <circle cx="47" cy="43" r="1" fill="#FFD700" />
                </svg>
            `;
        }
        
        function generatePlayingPet() {
            return `
                <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite" width="120" height="120">
                    <!-- Head + Body (slightly jumped) -->
                    <ellipse cx="50" cy="48" rx="28" ry="32" fill="#FFCC66" />
                    
                    <!-- Belly -->
                    <ellipse cx="50" cy="58" rx="12" ry="16" fill="#FFE5A0" />
                    
                    <!-- Eyes (excited star-shaped) -->
                    <g id="star-eye-left">
                        <polygon points="40,37 41.5,41 45,41 42.25,43.5 43.5,47 40,44.5 36.5,47 37.75,43.5 35,41 38.5,41" fill="black" />
                        <circle cx="40" cy="42" r="2" fill="white" opacity="0.8" />
                    </g>
                    <g id="star-eye-right">
                        <polygon points="60,37 61.5,41 65,41 62.25,43.5 63.5,47 60,44.5 56.5,47 57.75,43.5 55,41 58.5,41" fill="black" />
                        <circle cx="60" cy="42" r="2" fill="white" opacity="0.8" />
                    </g>
                    
                    <!-- Extremely joyful open mouth with tongue -->
                    <ellipse cx="50" cy="58" rx="8" ry="6" fill="black" />
                    <ellipse cx="50" cy="57" rx="6" ry="4" fill="#FF69B4" />
                    <ellipse cx="50" cy="60" rx="3" ry="2" fill="#FF1493" />
                    
                    <!-- Excited blush -->
                    <ellipse cx="30" cy="46" rx="5" ry="3" fill="pink" opacity="0.8" />
                    <ellipse cx="70" cy="46" rx="5" ry="3" fill="pink" opacity="0.8" />
                    
                    <!-- Feet (mid-jump) -->
                    <ellipse cx="38" cy="78" rx="4" ry="2" fill="#AA8844" />
                    <ellipse cx="62" cy="78" rx="4" ry="2" fill="#AA8844" />
                    
                    <!-- Ears (perked up) -->
                    <ellipse cx="28" cy="24" rx="6" ry="10" fill="#FFCC66" />
                    <ellipse cx="72" cy="24" rx="6" ry="10" fill="#FFCC66" />
                    
                    <!-- Sweat drops from excitement -->
                    <ellipse cx="25" cy="35" rx="2" ry="4" fill="#87CEEB" opacity="0.7" />
                    <ellipse cx="75" cy="38" rx="2" ry="4" fill="#87CEEB" opacity="0.7" />
                    
                    <!-- Play energy lines -->
                    <path d="M20,45 Q22,42 24,45" stroke="#FFD700" stroke-width="2" fill="none" opacity="0.8" />
                    <path d="M76,50 Q78,47 80,50" stroke="#FFD700" stroke-width="2" fill="none" opacity="0.8" />
                    <path d="M18,60 Q20,57 22,60" stroke="#FF69B4" stroke-width="2" fill="none" opacity="0.8" />
                    <path d="M78,65 Q80,62 82,65" stroke="#FF69B4" stroke-width="2" fill="none" opacity="0.8" />
                    
                    <!-- Extra joy sparkles -->
                    <circle cx="25" cy="25" r="2" fill="#FFD700" opacity="0.9" />
                    <circle cx="75" cy="30" r="2" fill="#FFD700" opacity="0.9" />
                    <circle cx="15" cy="55" r="1.5" fill="#FF69B4" opacity="0.9" />
                    <circle cx="85" cy="45" r="1.5" fill="#4ECDC4" opacity="0.9" />
                </svg>
            `;
        }
        
        function generateSleepingPet() {
            return `
                <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite" width="120" height="120">
                    <!-- Head + Body (relaxed) -->
                    <ellipse cx="50" cy="52" rx="28" ry="30" fill="#D4A574" />
                    
                    <!-- Belly -->
                    <ellipse cx="50" cy="62" rx="12" ry="14" fill="#E8C792" />
                    
                    <!-- Closed eyes -->
                    <path d="M35,42 Q40,40 45,42" stroke="black" stroke-width="2" fill="none" />
                    <path d="M55,42 Q60,40 65,42" stroke="black" stroke-width="2" fill="none" />
                    
                    <!-- Peaceful mouth -->
                    <path d="M47,58 Q50,60 53,58" stroke="black" stroke-width="1.5" fill="none" />
                    
                    <!-- Feet -->
                    <ellipse cx="40" cy="80" rx="4" ry="2" fill="#AA8844" />
                    <ellipse cx="60" cy="80" rx="4" ry="2" fill="#AA8844" />
                    
                    <!-- Ears (drooped) -->
                    <ellipse cx="32" cy="30" rx="4" ry="7" fill="#D4A574" />
                    <ellipse cx="68" cy="30" rx="4" ry="7" fill="#D4A574" />
                    
                    <!-- Night cap -->
                    <ellipse cx="50" cy="25" rx="15" ry="8" fill="#4A4A8C" />
                    <circle cx="60" cy="20" r="4" fill="#FFFF99" />
                </svg>
            `;
        }
        
        function generateSickPet() {
            return `
                <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite" width="120" height="120">
                    <!-- Head + Body (sickly green tint) -->
                    <ellipse cx="50" cy="50" rx="28" ry="32" fill="#B8CC66" />
                    
                    <!-- Belly -->
                    <ellipse cx="50" cy="60" rx="12" ry="16" fill="#D1E5A0" />
                    
                    <!-- Sick eyes (X eyes or spiral) -->
                    <circle cx="40" cy="42" r="5" fill="white" />
                    <circle cx="60" cy="42" r="5" fill="white" />
                    <path d="M37,39 L43,45 M37,45 L43,39" stroke="red" stroke-width="2" />
                    <path d="M57,39 L63,45 M57,45 L63,39" stroke="red" stroke-width="2" />
                    
                    <!-- Sick mouth -->
                    <path d="M42,62 Q50,68 58,62" stroke="black" stroke-width="2" fill="none" />
                    
                    <!-- Sickly pallor -->
                    <ellipse cx="32" cy="48" rx="3" ry="1.5" fill="#90EE90" opacity="0.4" />
                    <ellipse cx="68" cy="48" rx="3" ry="1.5" fill="#90EE90" opacity="0.4" />
                    
                    <!-- Feet -->
                    <ellipse cx="40" cy="80" rx="4" ry="2" fill="#AA8844" />
                    <ellipse cx="60" cy="80" rx="4" ry="2" fill="#AA8844" />
                    
                    <!-- Ears -->
                    <ellipse cx="30" cy="28" rx="5" ry="8" fill="#B8CC66" />
                    <ellipse cx="70" cy="28" rx="5" ry="8" fill="#B8CC66" />
                    
                    <!-- Thermometer -->
                    <rect x="70" y="35" width="2" height="12" fill="silver" />
                    <circle cx="71" cy="47" r="2" fill="red" />
                    
                    <!-- Germs -->
                    <circle cx="25" cy="30" r="2" fill="#90EE90" opacity="0.6" />
                    <circle cx="20" cy="40" r="1.5" fill="#90EE90" opacity="0.6" />
                    <circle cx="80" cy="45" r="2" fill="#90EE90" opacity="0.6" />
                </svg>
            `;
        }
        
        function generateHungryPet() {
            return `
                <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite" width="120" height="120">
                    <!-- Head + Body (slightly thinner) -->
                    <ellipse cx="50" cy="50" rx="25" ry="32" fill="#FFCC66" />
                    
                    <!-- Belly (smaller) -->
                    <ellipse cx="50" cy="60" rx="10" ry="14" fill="#FFE5A0" />
                    
                    <!-- Sad eyes -->
                    <circle cx="40" cy="42" r="5" fill="black" />
                    <circle cx="60" cy="42" r="5" fill="black" />
                    <circle cx="39" cy="43" r="1.5" fill="white" />
                    <circle cx="59" cy="43" r="1.5" fill="white" />
                    
                    <!-- Hungry mouth -->
                    <path d="M42,62 Q50,68 58,62" stroke="black" stroke-width="2" fill="none" />
                    
                    <!-- Tears -->
                    <ellipse cx="35" cy="50" rx="1" ry="4" fill="#87CEEB" opacity="0.7" />
                    <ellipse cx="65" cy="50" rx="1" ry="4" fill="#87CEEB" opacity="0.7" />
                    
                    <!-- Feet -->
                    <ellipse cx="40" cy="80" rx="4" ry="2" fill="#AA8844" />
                    <ellipse cx="60" cy="80" rx="4" ry="2" fill="#AA8844" />
                    
                    <!-- Ears (drooped) -->
                    <ellipse cx="32" cy="30" rx="4" ry="7" fill="#FFCC66" />
                    <ellipse cx="68" cy="30" rx="4" ry="7" fill="#FFCC66" />
                    
                    <!-- Hunger indicator -->
                    <text x="50" y="15" text-anchor="middle" font-size="8" fill="red">HUNGRY!</text>
                    
                    <!-- Empty food bowl -->
                    <ellipse cx="25" cy="75" rx="8" ry="3" fill="#8B4513" />
                    <ellipse cx="25" cy="74" rx="6" ry="2" fill="#D2691E" />
                </svg>
            `;
        }
        
        function generateDeadPet() {
            return `
                <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite" width="120" height="120">
                    <!-- Head + Body (gray) -->
                    <ellipse cx="50" cy="55" rx="28" ry="30" fill="#888888" />
                    
                    <!-- Belly -->
                    <ellipse cx="50" cy="65" rx="12" ry="14" fill="#AAAAAA" />
                    
                    <!-- X eyes -->
                    <path d="M35,38 L45,48 M35,48 L45,38" stroke="black" stroke-width="3" />
                    <path d="M55,38 L65,48 M55,48 L65,38" stroke="black" stroke-width="3" />
                    
                    <!-- Mouth -->
                    <path d="M42,65 Q50,70 58,65" stroke="black" stroke-width="2" fill="none" />
                    
                    <!-- Feet -->
                    <ellipse cx="40" cy="82" rx="4" ry="2" fill="#666666" />
                    <ellipse cx="60" cy="82" rx="4" ry="2" fill="#666666" />
                    
                    <!-- Ears -->
                    <ellipse cx="30" cy="32" rx="5" ry="6" fill="#888888" />
                    <ellipse cx="70" cy="32" rx="5" ry="6" fill="#888888" />
                    
                    <!-- Angel halo -->
                    <ellipse cx="50" cy="20" rx="12" ry="3" fill="none" stroke="#FFD700" stroke-width="2" />
                    
                    <!-- RIP marker -->
                    <rect x="75" y="65" width="12" height="15" fill="#666666" />
                    <text x="81" y="73" text-anchor="middle" font-size="4" fill="white">RIP</text>
                </svg>
            `;
        }
        
        function updateButtons() {
            const buttons = document.querySelectorAll('.game-button:not(#reset-btn)');
            buttons.forEach(button => {
                button.disabled = !pet.isAlive;
            });
            
            // Update sleep button text
            const sleepBtn = document.getElementById('sleep-text');
            sleepBtn.textContent = pet.state === 'sleeping' ? 'Wake' : 'Sleep';
        }
        
        // Particle effects
        function createFoodParticles() {
            const container = document.getElementById('particles-container');
            for (let i = 0; i < 6; i++) {
                const particle = document.createElement('div');
                particle.className = 'particle food-particle';
                particle.style.width = '6px';
                particle.style.height = '6px';
                particle.style.left = (40 + Math.random() * 20) + '%';
                particle.style.top = '10%';
                particle.style.animationDelay = (i * 0.3) + 's';
                container.appendChild(particle);
                
                setTimeout(() => {
                    if (particle.parentNode) {
                        particle.parentNode.removeChild(particle);
                    }
                }, 2500);
            }
        }
        
        function createPlayParticles() {
            const container = document.getElementById('particles-container');
            const colors = ['#FF6B9D', '#4ECDC4', '#FFE66D', '#95E1D3', '#FF8A80'];
            
            for (let i = 0; i < 10; i++) {
                const particle = document.createElement('div');
                particle.className = 'particle play-particle';
                particle.style.width = '8px';
                particle.style.height = '8px';
                particle.style.backgroundColor = colors[Math.floor(Math.random() * colors.length)];
                particle.style.left = (45 + Math.random() * 10) + '%';
                particle.style.top = (45 + Math.random() * 10) + '%';
                particle.style.animationDelay = (i * 0.1) + 's';
                container.appendChild(particle);
                
                setTimeout(() => {
                    if (particle.parentNode) {
                        particle.parentNode.removeChild(particle);
                    }
                }, 2000);
            }
        }
        
        function createSleepEffects() {
            const container = document.getElementById('particles-container');
            
            function addSleepZ() {
                if (pet.state === 'sleeping') {
                    const z = document.createElement('div');
                    z.className = 'sleep-z';
                    z.textContent = 'Z';
                    z.style.position = 'absolute';
                    z.style.left = '60%';
                    z.style.top = '30%';
                    container.appendChild(z);
                    
                    setTimeout(() => {
                        if (z.parentNode) {
                            z.parentNode.removeChild(z);
                        }
                    }, 3000);
                    
                    if (pet.state === 'sleeping') {
                        const timeout = setTimeout(addSleepZ, 2000);
                        animationTimeouts.push(timeout);
                    }
                }
            }
            
            addSleepZ();
        }
        
        function createHealParticles() {
            const container = document.getElementById('particles-container');
            
            for (let i = 0; i < 12; i++) {
                const particle = document.createElement('div');
                particle.className = 'particle heal-particle';
                particle.style.width = '6px';
                particle.style.height = '6px';
                particle.style.left = (30 + Math.random() * 40) + '%';
                particle.style.top = (30 + Math.random() * 40) + '%';
                particle.style.animationDelay = (i * 0.15) + 's';
                container.appendChild(particle);
                
                setTimeout(() => {
                    if (particle.parentNode) {
                        particle.parentNode.removeChild(particle);
                    }
                }, 2500);
            }
        }
        
        function clearAnimationTimeouts() {
            animationTimeouts.forEach(timeout => clearTimeout(timeout));
            animationTimeouts = [];
            
            // Clear particles
            const container = document.getElementById('particles-container');
            container.innerHTML = '';
        }
        
        // Action handlers
        function performAction(action) {
            fetch(`/${action}`, { method: 'POST' })
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        const messages = {
                            feed: 'Your pet enjoyed the meal! 🍎',
                            play: 'Your pet had fun playing! 🎉',
                            clean: 'Your pet is squeaky clean! 🧼',
                            sleep: pet.state === 'sleeping' ? 'Good morning! 🌅' : 'Sweet dreams! 😴',
                            heal: 'Your pet feels much better! 💚'
                        };
                        
                        showMessage(messages[action], 'success');
                        
                        // Trigger appropriate effects
                        switch(action) {
                            case 'feed':
                                createFoodParticles();
                                break;
                            case 'play':
                                createPlayParticles();
                                break;
                            case 'heal':
                                createHealParticles();
                                break;
                        }
                        
                        updatePetData();
                    } else {
                        showMessage(data.message || `Failed to ${action}`, 'error');
                    }
                })
                .catch(error => {
                    console.error('Error:', error);
                    showMessage(`Error performing ${action}`, 'error');
                });
        }
        
        function resetPet() {
            if (confirm('Are you sure you want to reset your pet? This cannot be undone!')) {
                fetch('/reset', { method: 'POST' })
                    .then(response => response.json())
                    .then(data => {
                        if (data.success) {
                            showMessage('Welcome to your new pet! 🐣', 'success');
                            clearAnimationTimeouts();
                            updatePetData();
                        } else {
                            showMessage(data.message || 'Failed to reset pet', 'error');
                        }
                    })
                    .catch(error => {
                        console.error('Error:', error);
                        showMessage('Error resetting pet', 'error');
                    });
            }
        }
        
        function showMessage(text, type) {
            const messageDiv = document.getElementById('message');
            const messageText = document.getElementById('message-text');
            
            messageText.textContent = text;
            messageDiv.className = `message mx-4 mt-4 p-3 ${type}`;
            messageDiv.classList.remove('hidden');
            
            if (window.messageTimeout) {
                clearTimeout(window.messageTimeout);
            }
            
            window.messageTimeout = setTimeout(() => {
                messageDiv.classList.add('hidden');
            }, 4000);
        }
    </script>
</body>
</html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleGetPet() {
  DynamicJsonDocument jsonDoc(1024);

  jsonDoc["hunger"] = pet.hunger;
  jsonDoc["happiness"] = pet.happiness;
  jsonDoc["health"] = pet.health;
  jsonDoc["energy"] = pet.energy;
  jsonDoc["cleanliness"] = pet.cleanliness;
  jsonDoc["age"] = pet.age;
  jsonDoc["isAlive"] = pet.isAlive;
  jsonDoc["state"] = pet.state;

  String response;
  serializeJson(jsonDoc, response);

  server.send(200, "application/json", response);
}

void feedPet() {
  if (!pet.isAlive) {
    sendJsonResponse(false, "Pet is not alive");
    return;
  }

  pet.hunger = min(100, pet.hunger + 20);
  pet.energy = max(0, pet.energy - 5);
  pet.state = "eating";
  pet.stateChangeTime = millis();
  pet.lastInteractionTime = millis();

  // If very hungry, improve health too
  if (pet.hunger < 20) {
    pet.health = min(100, pet.health + 5);
  }

  savePetData();
  sendJsonResponse(true, "");
}

void handleFeed() {
  feedPet();
}

void handlePlay() {
  if (!pet.isAlive) {
    sendJsonResponse(false, "Pet is not alive");
    return;
  }

  if (pet.energy < 10) {
    sendJsonResponse(false, "Pet is too tired to play");
    return;
  }

  pet.happiness = min(100, pet.happiness + 15);
  pet.energy = max(0, pet.energy - 15);
  pet.hunger = max(0, pet.hunger - 10);
  pet.state = "playing";
  pet.stateChangeTime = millis();
  pet.lastInteractionTime = millis();

  savePetData();
  sendJsonResponse(true, "");
}

void handleClean() {
  if (!pet.isAlive) {
    sendJsonResponse(false, "Pet is not alive");
    return;
  }

  pet.cleanliness = 100;
  pet.health = min(100, pet.health + 5);
  pet.lastInteractionTime = millis();

  savePetData();
  sendJsonResponse(true, "");
}

void handleSleep() {
  if (!pet.isAlive) {
    sendJsonResponse(false, "Pet is not alive");
    return;
  }

   if (pet.state == "sleeping") {
    pet.state = "normal";
  } else {
    pet.state = "sleeping";
    pet.stateChangeTime = millis();
  }

  pet.lastInteractionTime = millis();
  savePetData();
  sendJsonResponse(true, "");
}

void handleHeal() {
  if (!pet.isAlive) {
    sendJsonResponse(false, "Pet is not alive");
    return;
  }

  pet.health = 100;
  pet.state = "normal";
  pet.lastInteractionTime = millis();

  savePetData();
  sendJsonResponse(true, "");
}

void handleReset() {
  // Reset pet to default values
  pet.hunger = 50;
  pet.happiness = 50;
  pet.health = 80;
  pet.energy = 100;
  pet.cleanliness = 80;
  pet.age = 0;
  pet.isAlive = true;
  pet.state = "normal";
  pet.stateChangeTime = millis();
  pet.lastInteractionTime = millis();

  savePetData();
  sendJsonResponse(true, "");
}

void handleUpdate() {
  // This endpoint is for AJAX updates of pet data
  // Just return current pet data
  handleGetPet();
}

void sendJsonResponse(bool success, String message) {
  DynamicJsonDocument jsonDoc(256);

  jsonDoc["success"] = success;
  if (message.length() > 0) {
    jsonDoc["message"] = message;
  }

  String response;
  serializeJson(jsonDoc, response);

  server.send(200, "application/json", response);
}

void savePetData() {
  // Save pet data to SPIFFS
  File file = SPIFFS.open("/pet_data.json", "w");
  if (!file) {
    Serial.println("Failed to open pet data file for writing");
    return;
  }

  DynamicJsonDocument jsonDoc(1024);

  jsonDoc["hunger"] = pet.hunger;
  jsonDoc["happiness"] = pet.happiness;
  jsonDoc["health"] = pet.health;
  jsonDoc["energy"] = pet.energy;
  jsonDoc["cleanliness"] = pet.cleanliness;
  jsonDoc["age"] = pet.age;
  jsonDoc["isAlive"] = pet.isAlive;
  jsonDoc["state"] = pet.state;

  if (serializeJson(jsonDoc, file) == 0) {
    Serial.println("Failed to write pet data to file");
  }

  file.close();
}

void loadPetData() {
  // Try to load pet data from SPIFFS
  if (!SPIFFS.exists("/pet_data.json")) {
    Serial.println("No pet data file found, using defaults");
    return;
  }

  File file = SPIFFS.open("/pet_data.json", "r");
  if (!file) {
    Serial.println("Failed to open pet data file for reading");
    return;
  }

  DynamicJsonDocument jsonDoc(1024);
  DeserializationError error = deserializeJson(jsonDoc, file);

  if (error) {
    Serial.println("Failed to parse pet data file");
  } else {
    pet.hunger = jsonDoc["hunger"];
    pet.happiness = jsonDoc["happiness"];
    pet.health = jsonDoc["health"];
    pet.energy = jsonDoc["energy"];
    pet.cleanliness = jsonDoc["cleanliness"];
    pet.age = jsonDoc["age"];
    pet.isAlive = jsonDoc["isAlive"];
    pet.state = jsonDoc["state"].as<String>();
  }

  file.close();
}
