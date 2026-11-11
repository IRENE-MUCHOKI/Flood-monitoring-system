// Pin Definitions
const int trigPin = 2;
const int echoPin = 3;

const int greenLedPin = 8;
const int yellowLedPin = 9;
const int redLedPin = 10;
const int buzzerPin = 11;

// CONTAINER & THRESHOLD SETTINGS (25 cm Container)
const float CONTAINER_DEPTH = 25.0;     
const float WARNING_HEIGHT = 10.0;    
const float DANGER_HEIGHT = 18.0;     
const float FLASH_FLOOD_RATE = 1.5;   // Trigger threshold for rapid rise

float previousWaterHeight = 0.0;
unsigned long previousMillis = 0;
const long interval = 2000; // Increased to 2 seconds for a smoother rate window

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Initialize baseline height
  previousWaterHeight = CONTAINER_DEPTH - getAveragedDistance();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Get a heavily smoothed distance reading using multiple samples
  float currentDistance = getAveragedDistance();
  float waterHeight = CONTAINER_DEPTH - currentDistance;
  if (waterHeight < 0) waterHeight = 0;
  if (waterHeight > CONTAINER_DEPTH) waterHeight = CONTAINER_DEPTH;

  float rateOfRise = 0;

  // Calculate rate of rise every 2 seconds
  if (currentMillis - previousMillis >= interval) {
    float timeSeconds = interval / 1000.0;
    rateOfRise = (waterHeight - previousWaterHeight) / timeSeconds;
    
    // --- DEADBAND FILTER ---
    // Ignore tiny micro-jitters between -0.4 and +0.4 cm/s, force them to 0.00
    if (rateOfRise > -0.4 && rateOfRise < 0.4) {
      rateOfRise = 0.0;
    }

    previousWaterHeight = waterHeight;
    previousMillis = currentMillis;

    // Determine current alert state
    String status = "SAFE";
    if (rateOfRise >= FLASH_FLOOD_RATE) {
      status = "FLASH_FLOOD";
    } else if (waterHeight >= DANGER_HEIGHT) {
      status = "DANGER";
    } else if (waterHeight >= WARNING_HEIGHT) {
      status = "WARNING";
    }

    // Output formatted string for the Chrome Web Dashboard
    Serial.print("HEIGHT:");
    Serial.print(waterHeight, 1);
    Serial.print("|RATE:");
    Serial.print(rateOfRise, 2);
    Serial.print("|STATUS:");
    Serial.println(status);
  }

  // Local Physical Warning Execution 
  digitalWrite(greenLedPin, LOW);
  digitalWrite(yellowLedPin, LOW);
  digitalWrite(redLedPin, LOW);
  digitalWrite(buzzerPin, LOW);

  if (waterHeight >= DANGER_HEIGHT || rateOfRise >= FLASH_FLOOD_RATE) {
    digitalWrite(redLedPin, HIGH);
    digitalWrite(buzzerPin, HIGH);
  } else if (waterHeight >= WARNING_HEIGHT) {
    digitalWrite(yellowLedPin, HIGH);
  } else {
    digitalWrite(greenLedPin, HIGH);
  }

  delay(100);
}

// Function that takes 10 rapid samples and averages them to kill sensor noise
float getAveragedDistance() {
  float sum = 0;
  int validSamples = 0;
  
  for (int i = 0; i < 10; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    long duration = pulseIn(echoPin, HIGH, 30000);
    if (duration > 0) {
      sum += (duration * 0.034 / 2);
      validSamples++;
    }
    delay(20); // Small pause between internal samples
  }
  
  if (validSamples > 0) {
    return sum / validSamples;
  }
  return CONTAINER_DEPTH; // Fallback
}
