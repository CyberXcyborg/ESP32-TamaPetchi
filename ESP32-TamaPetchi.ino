#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Replace with your network credentials
const char* ssid = "";
const char* password = "";

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
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 TamaPetchi</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background-color: #f0f0f0;
      margin: 0;
      padding: 10px;
    }
    .container {
      max-width: 500px;
      margin: 0 auto;
      background-color: white;
      border-radius: 15px;
      padding: 15px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
    }
    h1 {
      color: #333;
      margin-top: 0;
    }
    .pet-container {
      width: 200px;
      height: 200px;
      margin: 20px auto;
      position: relative;
      background-color: #e0f0ff;
      border-radius: 15px;
      overflow: hidden;
      box-shadow: inset 0 0 20px rgba(0,0,0,0.2);
      transition: background-color 0.5s ease;
    }
    .pet-environment {
      position: absolute;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      z-index: 1;
    }
    .pet {
      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      width: 100px;
      height: 100px;
      transition: all 0.5s;
      z-index: 10;
    }
    .button-container {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      gap: 10px;
      margin: 20px 0;
    }
    button {
      background-color: #4CAF50;
      border: none;
      color: white;
      padding: 10px 15px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
      margin: 4px 2px;
      cursor: pointer;
      border-radius: 5px;
      transition: all 0.3s;
    }
    button:hover {
      background-color: #45a049;
      transform: scale(1.05);
    }
    button:active {
      transform: scale(0.95);
    }
    button:disabled {
      background-color: #cccccc;
      cursor: not-allowed;
    }
    .stat-container {
      margin-bottom: 10px;
      text-align: left;
    }
    .stat-bar {
      height: 20px;
      background-color: #ddd;
      border-radius: 10px;
      margin-top: 5px;
      overflow: hidden;
    }
    .stat-value {
      height: 100%;
      border-radius: 10px;
      transition: width 0.5s;
    }
    .hunger .stat-value {
      background-color: #FF9800;
    }
    .happiness .stat-value {
      background-color: #FFC107;
    }
    .health .stat-value {
      background-color: #4CAF50;
    }
    .energy .stat-value {
      background-color: #2196F3;
    }
    .cleanliness .stat-value {
      background-color: #00BCD4;
    }
    #feed { background-color: #FF9800; }
    #play { background-color: #FFC107; }
    #clean { background-color: #00BCD4; }
    #sleep { background-color: #673AB7; }
    #heal { background-color: #E91E63; }
    #reset { background-color: #F44336; }
    .dead-pet {
      filter: grayscale(100%);
    }
    .message {
      margin: 10px;
      padding: 10px;
      border-radius: 5px;
      display: none;
    }
    .success {
      background-color: #e8f5e9;
      color: #2e7d32;
    }
    .error {
      background-color: #ffebee;
      color: #c62828;
    }
    
    /* Enhanced Pet Styling */
    .pet-sprite {
      position: relative;
      transform-origin: center bottom;
    }
    
    /* Animation classes */
    .bounce {
      animation: bounce 1s infinite alternate;
    }
    @keyframes bounce {
      0% { transform: translateY(0); }
      100% { transform: translateY(-5px); }
    }
    
    .wobble {
      animation: wobble 1.5s infinite;
    }
    @keyframes wobble {
      0%, 100% { transform: rotate(0deg); }
      25% { transform: rotate(5deg); }
      75% { transform: rotate(-5deg); }
    }
    
    .pulse {
      animation: pulse 1.5s infinite;
    }
    @keyframes pulse {
      0%, 100% { transform: scale(1); }
      50% { transform: scale(1.1); }
    }
    
    .shake {
      animation: shake 0.5s infinite;
    }
    @keyframes shake {
      0%, 100% { transform: translateX(0); }
      25% { transform: translateX(-3px) rotate(-5deg); }
      75% { transform: translateX(3px) rotate(5deg); }
    }
    
    .sleep-z {
      animation: floating-z 3s infinite ease-in-out;
      opacity: 0;
    }
    @keyframes floating-z {
      0% { transform: translate(60px, 10px) scale(0.4); opacity: 0; }
      25% { opacity: 1; }
      50% { transform: translate(75px, -5px) scale(0.6); opacity: 1; }
      75% { opacity: 1; }
      100% { transform: translate(90px, -20px) scale(0.8); opacity: 0; }
    }
    
    .blink-eyes {
      animation: blink 4s infinite;
    }
    @keyframes blink {
      0%, 96%, 100% { opacity: 1; }
      97%, 99% { opacity: 0; }
    }
    
    .float {
      animation: float 3s infinite ease-in-out;
    }
    @keyframes float {
      0%, 100% { transform: translateY(0); }
      50% { transform: translateY(-8px); }
    }
    
    .rotating-food {
      animation: rotate-food 2s infinite linear;
      transform-origin: center;
    }
    @keyframes rotate-food {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
    
    .eating-motion {
      animation: eat 0.5s infinite alternate;
    }
    @keyframes eat {
      0% { transform: scaleY(1); }
      100% { transform: scaleY(0.9); }
    }
    
    .playing-motion {
      animation: play-jump 1s infinite alternate;
    }
    @keyframes play-jump {
      0% { transform: translateY(0) rotate(0deg); }
      25% { transform: translateY(-15px) rotate(-5deg); }
      50% { transform: translateY(-20px) rotate(0deg); }
      75% { transform: translateY(-15px) rotate(5deg); }
      100% { transform: translateY(0) rotate(0deg); }
    }
    
    .glow-effect {
      animation: glow 2s infinite alternate;
      filter: drop-shadow(0 0 5px rgba(0, 255, 255, 0.7));
    }
    @keyframes glow {
      0% { filter: drop-shadow(0 0 3px rgba(0, 255, 255, 0.7)); }
      100% { filter: drop-shadow(0 0 10px rgba(0, 255, 255, 1)); }
    }
    
    .sick-effect {
      animation: sick-pulse 2s infinite alternate;
    }
    @keyframes sick-pulse {
      0% { opacity: 1; filter: hue-rotate(0deg); }
      100% { opacity: 0.7; filter: hue-rotate(30deg); }
    }
    
    .bacteria-float {
      animation: bacteria-move 5s infinite linear;
      transform-origin: center;
    }
    @keyframes bacteria-move {
      0% { transform: translate(0, 0) rotate(0deg); }
      25% { transform: translate(5px, 5px) rotate(90deg); }
      50% { transform: translate(0, 10px) rotate(180deg); }
      75% { transform: translate(-5px, 5px) rotate(270deg); }
      100% { transform: translate(0, 0) rotate(360deg); }
    }
    
    /* Environment themes */
    .environment-normal {
      background: linear-gradient(to bottom, #e0f0ff 0%, #b0e0ff 100%);
    }
    
    .environment-eating {
      background: linear-gradient(to bottom, #fff0e0 0%, #ffe0b0 100%);
    }
    
    .environment-playing {
      background: linear-gradient(to bottom, #e0ffe0 0%, #b0ffb0 100%);
    }
    
    .environment-sleeping {
      background: linear-gradient(to bottom, #e0e0ff 0%, #b0b0ff 100%);
      box-shadow: inset 0 0 20px rgba(0,0,0,0.4);
    }
    
    .environment-sick {
      background: linear-gradient(to bottom, #ffe0e0 0%, #ffb0b0 100%);
    }
    
    .environment-dead {
      background: linear-gradient(to bottom, #e0e0e0 0%, #b0b0b0 100%);
    }
    
    /* Food particles */
    .food-particle {
      position: absolute;
      width: 8px;
      height: 8px;
      background-color: #FFA500;
      border-radius: 50%;
      opacity: 0;
      z-index: 5;
    }
    
    @keyframes food-fall {
      0% { transform: translateY(-50px) rotate(0deg); opacity: 1; }
      80% { opacity: 1; }
      100% { transform: translateY(50px) rotate(360deg); opacity: 0; }
    }
    
    /* Play particles */
    .play-particle {
      position: absolute;
      width: 10px;
      height: 10px;
      border-radius: 50%;
      opacity: 0;
      z-index: 5;
    }
    
    @keyframes play-particle {
      0% { transform: translate(0, 0) scale(0); opacity: 1; }
      100% { transform: translate(var(--x), var(--y)) scale(1.5); opacity: 0; }
    }
    
    /* Clean effect */
    .clean-effect {
      position: absolute;
      width: 100%;
      height: 100%;
      background: radial-gradient(circle, rgba(255,255,255,0.8) 0%, rgba(255,255,255,0) 70%);
      opacity: 0;
      z-index: 20;
    }
    
    @keyframes clean-animation {
      0% { opacity: 0; transform: scale(0.5); }
      50% { opacity: 1; }
      100% { opacity: 0; transform: scale(1.5); }
    }
    
    /* Heal effect */
    .heal-sparkle {
      position: absolute;
      width: 8px;
      height: 8px;
      background-color: #00FF00;
      border-radius: 50%;
      opacity: 0;
      z-index: 15;
    }
    
    @keyframes heal-sparkle {
      0% { opacity: 0; transform: scale(0.5) rotate(0deg); }
      50% { opacity: 1; }
      100% { opacity: 0; transform: scale(1.5) rotate(90deg); }
    }
    
    @media (max-width: 400px) {
      button {
        font-size: 14px;
        padding: 8px 12px;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 TamaPetchi</h1>
    
    <div id="message" class="message"></div>
    
    <div class="pet-container" id="pet-environment">
      <div id="environment-bg" class="pet-environment environment-normal"></div>
      <div id="pet" class="pet">
        <!-- Pet will be drawn here with JS -->
      </div>
      <!-- Effects will be dynamically added -->
    </div>
    
    <div id="stats">
      <div class="stat-container hunger">
        <label>Hunger:</label>
        <div class="stat-bar">
          <div class="stat-value" id="hunger-bar"></div>
        </div>
      </div>
      
      <div class="stat-container happiness">
        <label>Happiness:</label>
        <div class="stat-bar">
          <div class="stat-value" id="happiness-bar"></div>
        </div>
      </div>
      
      <div class="stat-container health">
        <label>Health:</label>
        <div class="stat-bar">
          <div class="stat-value" id="health-bar"></div>
        </div>
      </div>
      
      <div class="stat-container energy">
        <label>Energy:</label>
        <div class="stat-bar">
          <div class="stat-value" id="energy-bar"></div>
        </div>
      </div>
      
      <div class="stat-container cleanliness">
        <label>Cleanliness:</label>
        <div class="stat-bar">
          <div class="stat-value" id="cleanliness-bar"></div>
        </div>
      </div>
      
      <p>Age: <span id="age">0</span> minutes</p>
    </div>
    
    <div class="button-container">
      <button id="feed">Feed</button>
      <button id="play">Play</button>
      <button id="clean">Clean</button>
      <button id="sleep">Sleep/Wake</button>
      <button id="heal">Heal</button>
      <button id="reset">Reset</button>
    </div>
  </div>

  <script>
    // Pet state
    let pet = {};
    let updateInterval;
    let previousState = "";
    let animationInterval;
    
    // Initialize
    window.onload = function() {
      // Add event listeners to buttons
      document.getElementById('feed').addEventListener('click', feedPet);
      document.getElementById('play').addEventListener('click', playWithPet);
      document.getElementById('clean').addEventListener('click', cleanPet);
      document.getElementById('sleep').addEventListener('click', toggleSleep);
      document.getElementById('heal').addEventListener('click', healPet);
      document.getElementById('reset').addEventListener('click', resetPet);
      
      // Get initial pet data
      updatePetData();
      
      // Set up regular updates
      updateInterval = setInterval(updatePetData, 5000);
    };
    
    // Update pet data from the server
    function updatePetData() {
      fetch('/pet')
        .then(response => response.json())
        .then(data => {
          // Check if pet just woke up automatically
          if (previousState === "sleeping" && data.state === "normal" && data.energy >= 95) {
            showMessage('Your pet woke up fully rested!', 'success');
          }
          
          // Check if state changed
          if (previousState !== data.state) {
            triggerStateChangeAnimation(previousState, data.state);
          }
          
          // Save current state for next comparison
          previousState = data.state;
          
          // Update pet data
          pet = data;
          updateDisplay();
        })
        .catch(error => {
          console.error('Error fetching pet data:', error);
          showMessage('Error connecting to pet. Try refreshing.', 'error');
        });
    }
    
    // Trigger special animations on state change
    function triggerStateChangeAnimation(oldState, newState) {
      const petContainer = document.getElementById('pet-environment');
      
      // Clear any previous special effects
      clearSpecialEffects();
      
      // Apply new environment theme
      const environmentBg = document.getElementById('environment-bg');
      environmentBg.className = 'pet-environment environment-' + newState;
      
      // Special effect based on new state
      if (newState === "eating") {
        createFoodParticles();
      } else if (newState === "playing") {
        createPlayParticles();
      } else if (newState === "sleeping" && oldState !== "sleeping") {
        // Create sleep-z elements
        createSleepEffect();
      } else if (newState === "normal" && oldState === "sleeping") {
        // Wake up effect
        createWakeUpEffect();
      }
    }
    
    // Create food particles animation
    function createFoodParticles() {
      const petContainer = document.getElementById('pet-environment');
      for (let i = 0; i < 8; i++) {
        const food = document.createElement('div');
        food.className = 'food-particle';
        food.style.left = 20 + (Math.random() * 160) + 'px';
        food.style.animation = `food-fall ${1 + Math.random()}s forwards`;
        food.style.animationDelay = (i * 0.2) + 's';
        petContainer.appendChild(food);
        
        // Remove after animation completes
        setTimeout(() => {
          if (food && food.parentNode) {
            food.parentNode.removeChild(food);
          }
        }, 3000);
      }
    }
    
    // Create play particles animation
    function createPlayParticles() {
      const petContainer = document.getElementById('pet-environment');
      const colors = ['#FFA500', '#FFD700', '#9370DB', '#20B2AA', '#FF6347'];
      
      for (let i = 0; i < 12; i++) {
        const particle = document.createElement('div');
        particle.className = 'play-particle';
        particle.style.backgroundColor = colors[Math.floor(Math.random() * colors.length)];
        particle.style.left = '100px'; // Center of container
        particle.style.top = '100px';  // Center of container
        
        // Random direction
        const angle = Math.random() * Math.PI * 2;
        const distance = 30 + Math.random() * 80;
        const x = Math.cos(angle) * distance;
        const y = Math.sin(angle) * distance;
        particle.style.setProperty('--x', x + 'px');
        particle.style.setProperty('--y', y + 'px');
        
        particle.style.animation = `play-particle ${0.8 + Math.random()}s forwards`;
        particle.style.animationDelay = (i * 0.1) + 's';
        petContainer.appendChild(particle);
        
        // Remove after animation completes
        setTimeout(() => {
          if (particle && particle.parentNode) {
            particle.parentNode.removeChild(particle);
          }
        }, 2000);
      }
    }
    
    // Create sleep Z effects
    function createSleepEffect() {
      const petContainer = document.getElementById('pet-environment');
      
      // Create 3 Z's that appear and float up
      for (let i = 0; i < 3; i++) {
        const sleepZ = document.createElement('div');
        sleepZ.className = 'sleep-z';
        sleepZ.innerHTML = '<svg viewBox="0 0 30 30" xmlns="http://www.w3.org/2000/svg">' +
                           '<text x="0" y="20" font-size="24" fill="#7030A0">Z</text>' +
                           '</svg>';
        sleepZ.style.position = 'absolute';
        sleepZ.style.width = '30px';
        sleepZ.style.height = '30px';
        sleepZ.style.left = '80px';
        sleepZ.style.top = '70px';
        sleepZ.style.animationDelay = (i * 2) + 's';
        
        petContainer.appendChild(sleepZ);
        
        // Don't remove Z's while sleeping - they'll be cleared on state change
      }
      
      // Start an interval to continually add more Z's
      animationInterval = setInterval(() => {
        if (pet.state === "sleeping") {
          const sleepZ = document.createElement('div');
          sleepZ.className = 'sleep-z';
          sleepZ.innerHTML = '<svg viewBox="0 0 30 30" xmlns="http://www.w3.org/2000/svg">' +
                             '<text x="0" y="20" font-size="24" fill="#7030A0">Z</text>' +
                             '</svg>';
          sleepZ.style.position = 'absolute';
          sleepZ.style.width = '30px';
          sleepZ.style.height = '30px';
          sleepZ.style.left = '80px';
          sleepZ.style.top = '70px';
          
          petContainer.appendChild(sleepZ);
          
          // Remove after animation completes
          setTimeout(() => {
            if (sleepZ && sleepZ.parentNode) {
              sleepZ.parentNode.removeChild(sleepZ);
            }
          }, 3000);
        }
      }, 3000);
    }
    
    // Create wake up effect
    function createWakeUpEffect() {
      // Clear any sleep animation interval
      clearInterval(animationInterval);
      
      const petContainer = document.getElementById('pet-environment');
      const wakeEffect = document.createElement('div');
      wakeEffect.className = 'clean-effect';
      wakeEffect.style.animation = 'clean-animation 1s forwards';
      petContainer.appendChild(wakeEffect);
      
      // Remove after animation completes
      setTimeout(() => {
        if (wakeEffect && wakeEffect.parentNode) {
          wakeEffect.parentNode.removeChild(wakeEffect);
        }
      }, 1000);
    }
    
    // Clean effect animation
    function createCleanEffect() {
      const petContainer = document.getElementById('pet-environment');
      const cleanEffect = document.createElement('div');
      cleanEffect.className = 'clean-effect';
      cleanEffect.style.animation = 'clean-animation 1.5s forwards';
      petContainer.appendChild(cleanEffect);
      
      // Remove after animation completes
      setTimeout(() => {
        if (cleanEffect && cleanEffect.parentNode) {
          cleanEffect.parentNode.removeChild(cleanEffect);
        }
      }, 1500);
    }
    
    // Heal effect animation
    function createHealEffect() {
      const petContainer = document.getElementById('pet-environment');
      
      // Create multiple sparkles
      for (let i = 0; i < 15; i++) {
        const sparkle = document.createElement('div');
        sparkle.className = 'heal-sparkle';
        sparkle.style.left = 30 + (Math.random() * 140) + 'px';
        sparkle.style.top = 30 + (Math.random() * 140) + 'px';
        sparkle.style.animation = `heal-sparkle ${1 + Math.random()}s forwards`;
        sparkle.style.animationDelay = (i * 0.1) + 's';
        petContainer.appendChild(sparkle);
        
        // Remove after animation completes
        setTimeout(() => {
          if (sparkle && sparkle.parentNode) {
            sparkle.parentNode.removeChild(sparkle);
          }
        }, 2000);
      }
    }
    
    // Clear all special effects
    function clearSpecialEffects() {
      // Clear animation interval if it exists
      if (animationInterval) {
        clearInterval(animationInterval);
      }
      
      // Remove all special effect elements
      const effectElements = document.querySelectorAll('.food-particle, .play-particle, .sleep-z, .clean-effect, .heal-sparkle');
      effectElements.forEach(element => {
        if (element && element.parentNode) {
          element.parentNode.removeChild(element);
        }
      });
    }
    
    // Update visual display based on pet data
    function updateDisplay() {
      // Update stat bars
      document.getElementById('hunger-bar').style.width = pet.hunger + '%';
      document.getElementById('happiness-bar').style.width = pet.happiness + '%';
      document.getElementById('health-bar').style.width = pet.health + '%';
      document.getElementById('energy-bar').style.width = pet.energy + '%';
      document.getElementById('cleanliness-bar').style.width = pet.cleanliness + '%';
      document.getElementById('age').textContent = pet.age;
      
       // Update environment
      const environmentBg = document.getElementById('environment-bg');
      environmentBg.className = 'pet-environment environment-' + pet.state;
      
      // Update pet appearance
      const petDiv = document.getElementById('pet');
      petDiv.innerHTML = drawPet();
      
      // Add appropriate animation classes
      updatePetAnimation(petDiv);
      
      // Disable buttons if pet is dead
      const buttons = document.querySelectorAll('button:not(#reset)');
      buttons.forEach(button => {
        button.disabled = !pet.isAlive;
      });
      
      // Add dead class to pet if dead
      if (!pet.isAlive) {
        petDiv.classList.add('dead-pet');
        showMessage('Your pet has died! Press Reset to get a new one.', 'error');
      } else {
        petDiv.classList.remove('dead-pet');
      }
    }
    
    // Add appropriate animation classes based on state
    function updatePetAnimation(petDiv) {
      // Clear all animation classes first
      petDiv.classList.remove(
        'bounce', 'wobble', 'pulse', 'shake', 'float', 
        'eating-motion', 'playing-motion', 'glow-effect', 'sick-effect'
      );
      
      // Add appropriate animations based on state
      if (pet.state === "normal") {
        // Idle animations that cycle based on happiness
        if (pet.happiness > 70) {
          petDiv.classList.add('bounce');
          petDiv.classList.add('glow-effect');
        } else if (pet.happiness > 30) {
          petDiv.classList.add('wobble');
        } else {
          petDiv.classList.add('pulse');
        }
      } else if (pet.state === "eating") {
        petDiv.classList.add('eating-motion');
      } else if (pet.state === "playing") {
        petDiv.classList.add('playing-motion');
      } else if (pet.state === "sleeping") {
        // No animation during sleep
      } else if (pet.state === "sick") {
        petDiv.classList.add('shake');
        petDiv.classList.add('sick-effect');
      } else if (pet.state === "hungry") {
        petDiv.classList.add('pulse');
      } 
      // Dead has no animation
    }
    
    // Draw the pet based on its state
    function drawPet() {
      let svg = '';
      
      if (!pet.isAlive) {
        // Dead pixel spirit with enhanced details
        svg = `
          <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite">
            <rect x="30" y="20" width="40" height="50" fill="#888888" />
            <rect x="30" y="70" width="8" height="10" fill="#000000" />
            <rect x="38" y="70" width="8" height="10" fill="#888888" />
            <rect x="46" y="70" width="8" height="10" fill="#000000" />
            <rect x="54" y="70" width="8" height="10" fill="#888888" />
            <rect x="62" y="70" width="8" height="10" fill="#000000" />
            <line x1="35" y1="35" x2="45" y2="45" stroke="black" stroke-width="3" />
            <line x1="35" y1="45" x2="45" y2="35" stroke="black" stroke-width="3" />
            <line x1="55" y1="35" x2="65" y2="45" stroke="black" stroke-width="3" />
            <line x1="55" y1="45" x2="65" y2="35" stroke="black" stroke-width="3" />
            <rect x="40" y="55" width="20" height="2" fill="black" />
            <text x="41" y="18" font-size="10" fill="white">RIP</text>
            <rect x="40" y="10" width="20" height="4" fill="#999999" />
            <path d="M34,70 C34,65 36,60 50,60 C64,60 66,65 66,70" fill="none" stroke="#444444" stroke-width="1" />
          </svg>
        `;
      } else if (pet.state === "eating") {
        // Eating pixel spirit with food
        svg = `
          <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite">
            <rect x="30" y="20" width="40" height="50" fill="#00FFFF" />
            <rect x="30" y="70" width="8" height="10" fill="#000000" />
            <rect x="38" y="70" width="8" height="10" fill="#00FFFF" />
            <rect x="46" y="70" width="8" height="10" fill="#000000" />
            <rect x="54" y="70" width="8" height="10" fill="#00FFFF" />
            <rect x="62" y="70" width="8" height="10" fill="#000000" />
            <rect x="35" y="35" width="10" height="10" fill="black" class="blink-eyes" />
            <rect x="55" y="35" width="10" height="10" fill="black" class="blink-eyes" />
            <rect x="40" y="55" width="20" height="10" fill="black" />
            
            <!-- Food -->
            <g class="rotating-food" transform="translate(50, 80)">
              <circle cx="0" cy="0" r="7" fill="#FFCC00" />
              <circle cx="0" cy="-3" r="2" fill="#FF6600" />
            </g>
          </svg>
        `;
      } else if (pet.state === "playing") {
        // Playing pixel spirit with play elements
        svg = `
          <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite">
            <rect x="30" y="15" width="40" height="50" fill="#00FFFF" />
            <rect x="30" y="65" width="8" height="10" fill="#000000" />
            <rect x="38" y="65" width="8" height="10" fill="#00FFFF" />
            <rect x="46" y="65" width="8" height="10" fill="#000000" />
            <rect x="54" y="65" width="8" height="10" fill="#00FFFF" />
            <rect x="62" y="65" width="8" height="10" fill="#000000" />
            <rect x="35" y="30" width="8" height="8" fill="black" class="blink-eyes" />
            <rect x="57" y="30" width="8" height="8" fill="black" class="blink-eyes" />
            <rect x="35" y="50" width="30" height="5" fill="black" />
            <rect x="35" y="45" width="30" height="5" fill="#00FFFF" />
            
            <!-- Play elements -->
            <circle cx="20" cy="30" r="8" fill="#FFFF00" class="float" />
            <circle cx="80" cy="40" r="8" fill="#FFA500" class="float" style="animation-delay: 0.5s" />
            <circle cx="30" cy="15" r="6" fill="#FF5555" class="float" style="animation-delay: 1s" />
          </svg>
        `;
      } else if (pet.state === "sleeping") {
        // Sleeping pixel spirit
        svg = `
          <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite">
            <rect x="30" y="25" width="40" height="45" fill="#5D1C9E" />
            <rect x="30" y="70" width="8" height="10" fill="#000000" />
            <rect x="38" y="70" width="8" height="10" fill="#5D1C9E" />
            <rect x="46" y="70" width="8" height="10" fill="#000000" />
            <rect x="54" y="70" width="8" height="10" fill="#5D1C9E" />
            <rect x="62" y="70" width="8" height="10" fill="#000000" />
            <rect x="35" y="40" width="10" height="2" fill="black" />
            <rect x="55" y="40" width="10" height="2" fill="black" />
            <rect x="40" y="55" width="20" height="3" fill="black" />
            
            <!-- Small stars for sleeping effect -->
            <circle cx="25" cy="20" r="1" fill="#FFFFFF" opacity="0.8" />
            <circle cx="15" cy="30" r="1" fill="#FFFFFF" opacity="0.8" />
            <circle cx="85" cy="40" r="1" fill="#FFFFFF" opacity="0.8" />
            <circle cx="75" cy="15" r="1" fill="#FFFFFF" opacity="0.8" />
            <circle cx="35" cy="15" r="1" fill="#FFFFFF" opacity="0.8" />
          </svg>
        `;
      } else if (pet.state === "sick") {
        // Sick pixel spirit with bacteria
        svg = `
          <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite">
            <rect x="30" y="20" width="40" height="50" fill="#9ACD32" />
            <rect x="30" y="70" width="8" height="10" fill="#000000" />
            <rect x="38" y="70" width="8" height="10" fill="#9ACD32" />
            <rect x="46" y="70" width="8" height="10" fill="#000000" />
            <rect x="54" y="70" width="8" height="10" fill="#9ACD32" />
            <rect x="62" y="70" width="8" height="10" fill="#000000" />
            <line x1="35" y1="35" x2="45" y2="45" stroke="black" stroke-width="2" />
            <line x1="35" y1="45" x2="45" y2="35" stroke="black" stroke-width="2" />
            <line x1="55" y1="35" x2="65" y2="45" stroke="black" stroke-width="2" />
            <line x1="55" y1="45" x2="65" y2="35" stroke="black" stroke-width="2" />
            <rect x="40" y="60" width="20" height="3" fill="black" />
            
            <!-- Bacteria elements -->
            <g class="bacteria-float" style="animation-delay: 0s">
              <circle cx="25" cy="30" r="3" fill="#00FF00" opacity="0.7" />
            </g>
            <g class="bacteria-float" style="animation-delay: 0.5s">
              <circle cx="20" cy="35" r="3" fill="#00FF00" opacity="0.7" />
            </g>
            <g class="bacteria-float" style="animation-delay: 1s">
              <circle cx="15" cy="40" r="3" fill="#00FF00" opacity="0.7" />
            </g>
            <g class="bacteria-float" style="animation-delay: 1.5s">
              <circle cx="80" cy="45" r="3" fill="#00FF00" opacity="0.7" />
            </g>
          </svg>
        `;
      } else if (pet.state === "hungry") {
        // Hungry pixel spirit
        svg = `
          <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite">
            <rect x="30" y="20" width="40" height="50" fill="#00FFFF" />
            <rect x="30" y="70" width="8" height="10" fill="#000000" />
            <rect x="38" y="70" width="8" height="10" fill="#00FFFF" />
            <rect x="46" y="70" width="8" height="10" fill="#000000" />
            <rect x="54" y="70" width="8" height="10" fill="#00FFFF" />
            <rect x="62" y="70" width="8" height="10" fill="#000000" />
            <rect x="35" y="35" width="8" height="8" fill="black" class="blink-eyes" />
            <rect x="57" y="35" width="8" height="8" fill="black" class="blink-eyes" />
            <rect x="35" y="55" width="30" height="3" fill="black" />
            
            <!-- Hunger sign -->
            <rect x="35" y="10" width="30" height="5" fill="#FF3333" />
            <text x="37" y="15" font-size="8" fill="white">HUNGRY</text>
            <g class="float">
              <path d="M55,25 C60,22 63,28 58,32" stroke="#FFA500" fill="none" stroke-width="2" />
            </g>
          </svg>
        `;
      } else {
        // Normal pixel spirit with happiness-based variations
        let mouthShape = "";
        let eyeClass = "blink-eyes";
        let idleAnimClass = pet.happiness > 70 ? "bounce" : (pet.happiness > 30 ? "wobble" : "pulse");
        
        if (pet.happiness > 70) {
          mouthShape = `<rect x="35" y="50" width="30" height="5" fill="black" />
                        <rect x="35" y="45" width="30" height="5" fill="#00FFFF" />`;
        } else if (pet.happiness > 30) {
          mouthShape = `<rect x="35" y="50" width="30" height="3" fill="black" />`;
        } else {
          mouthShape = `<rect x="35" y="55" width="30" height="3" fill="black" />`;
        }
        
        svg = `
          <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" class="pet-sprite ${idleAnimClass}">
            <rect x="30" y="20" width="40" height="50" fill="#00FFFF" />
            <rect x="30" y="70" width="8" height="10" fill="#000000" />
            <rect x="38" y="70" width="8" height="10" fill="#00FFFF" />
            <rect x="46" y="70" width="8" height="10" fill="#000000" />
            <rect x="54" y="70" width="8" height="10" fill="#00FFFF" />
            <rect x="62" y="70" width="8" height="10" fill="#000000" />
            <rect x="35" y="35" width="8" height="8" fill="black" class="${eyeClass}" />
            <rect x="57" y="35" width="8" height="8" fill="black" class="${eyeClass}" />
            ${mouthShape}
            ${pet.hunger < 30 ? '<rect x="35" y="10" width="30" height="5" fill="#FF3333" />' : ''}
            <rect x="27" y="17" width="3" height="56" fill="#FFE0F0" opacity="0.3" />
            <rect x="70" y="17" width="3" height="56" fill="#FFE0F0" opacity="0.3" />
            <rect x="27" y="17" width="46" height="3" fill="#FFE0F0" opacity="0.3" />
            
            ${pet.happiness > 70 ? `
              <!-- Happy sparkles -->
              <circle cx="25" cy="25" r="2" fill="#FFFF00" opacity="0.8" class="float" style="animation-delay: 0s" />
              <circle cx="75" cy="30" r="2" fill="#FFFF00" opacity="0.8" class="float" style="animation-delay: 0.3s" />
              <circle cx="20" cy="60" r="2" fill="#FFFF00" opacity="0.8" class="float" style="animation-delay: 0.6s" />
            ` : ''}
          </svg>
        `;
      }
      
      return svg;
    }
    
    // Pet action functions
    function feedPet() {
      fetch('/feed', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showMessage('You fed your pet!', 'success');
            createFoodParticles();
            updatePetData();
          } else {
            showMessage(data.message || 'Failed to feed pet', 'error');
          }
        })
        .catch(error => {
          console.error('Error:', error);
          showMessage('Error feeding pet', 'error');
        });
    }
    
    function playWithPet() {
      fetch('/play', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showMessage('You played with your pet!', 'success');
            createPlayParticles();
            updatePetData();
          } else {
            showMessage(data.message || 'Failed to play with pet', 'error');
          }
        })
        .catch(error => {
          console.error('Error:', error);
          showMessage('Error playing with pet', 'error');
        });
    }
    
    function cleanPet() {
      fetch('/clean', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showMessage('You cleaned your pet!', 'success');
            createCleanEffect();
            updatePetData();
          } else {
            showMessage(data.message || 'Failed to clean pet', 'error');
          }
        })
        .catch(error => {
          console.error('Error:', error);
          showMessage('Error cleaning pet', 'error');
        });
    }
    
    function toggleSleep() {
      fetch('/sleep', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            if (pet.state === 'sleeping') {
              showMessage('You woke up your pet!', 'success');
              // Sleep effect will be removed by updatePetData
            } else {
              showMessage('Your pet is sleeping now', 'success');
              createSleepEffect();
            }
            updatePetData();
          } else {
            showMessage(data.message || 'Failed to toggle sleep', 'error');
          }
        })
        .catch(error => {
          console.error('Error:', error);
          showMessage('Error changing sleep state', 'error');
        });
    }
    
    function healPet() {
      fetch('/heal', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showMessage('You healed your pet!', 'success');
            createHealEffect();
            updatePetData();
          } else {
            showMessage(data.message || 'Failed to heal pet', 'error');
          }
        })
        .catch(error => {
          console.error('Error:', error);
          showMessage('Error healing pet', 'error');
        });
    }
    
    function resetPet() {
      if (confirm('Are you sure you want to reset your pet? This cannot be undone!')) {
        fetch('/reset', { method: 'POST' })
          .then(response => response.json())
          .then(data => {
            if (data.success) {
              showMessage('Your pet has been reset!', 'success');
              clearSpecialEffects();
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
    
    // Display a message that fades out
    function showMessage(text, type) {
      const messageDiv = document.getElementById('message');
      messageDiv.textContent = text;
      messageDiv.className = 'message ' + type;
      messageDiv.style.display = 'block';
      
      // Clear previous timeout if exists
      if (window.messageTimeout) {
        clearTimeout(window.messageTimeout);
      }
      
      // Hide message after 3 seconds
      window.messageTimeout = setTimeout(() => {
        messageDiv.style.display = 'none';
      }, 3000);
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