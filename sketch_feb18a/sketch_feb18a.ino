// Define the pins for each finger
const int T = 32; // Thumb
const int I = 33; // Index finger
const int M = 27; // Middle finger
const int R = 26; // Ring finger
const int P = 25; // Pinky finger

// Define the PWM frequency, finger index constants, and resolution
const int freq = 30000;
const int thumb = 0, indec = 1, middl = 2, ringf = 3, pinky = 4;
int myHand[5] = {thumb, indec, middl, ringf, pinky};
const int resolution = 8;

// Array to store power levels for H, J, K respectively
int powerLevels[3] = {0, 0, 0}; 

// Variable to keep track of the currently active pin
int activePin = -1;

// Structure to represent a step in a sequence
struct SequenceStep {
  int pinIndex; // Index of the pin (thumb, index, middle, ring, pinky)
  int duration; // Duration to keep the pin activated
};

// Structure to represent a sequence of steps
struct Sequence {
  SequenceStep steps[10]; // Maximum 10 steps per sequence
  int numSteps; // Number of steps in the sequence
};

// Array to store sequences
Sequence sequences[20]; // Maximum of 20 sequences
int numSequences = 0; // Current number of sequences

// Variables for frequency reading using interrupts
volatile unsigned long pulseCount = 0; // Count of pulses
volatile unsigned long startTime = 0; // Start time of pulse counting

// Setup function, runs once when the Arduino starts
void setup() {
  Serial.begin(9600); // Initialize serial communication
  
  // Setup PWM for each finger
  for(int i = 0; i < 5; i++){
    ledcSetup(myHand[i], freq, resolution);
  }

  // Attach each finger to its corresponding pin
  ledcAttachPin(T, thumb);
  ledcAttachPin(I, indec);
  ledcAttachPin(M, middl);
  ledcAttachPin(R, ringf);
  ledcAttachPin(P, pinky);

  // Turn off all fingers initially
  for(int i = 0; i < 5; i++){ 
      ledcWrite(myHand[i], 0);
  }

  // Attach interrupt to monitor pin T (Thumb)
  attachInterrupt(digitalPinToInterrupt(T), countPulse, RISING);
}

// Loop function, runs continuously after setup
void loop() {
  // Check if serial data is available
  if (Serial.available() > 0) {
    char keyPressed = Serial.read(); // Read the incoming byte
    
    // Check for finger activation (1-5)
    if (keyPressed >= '1' && keyPressed <= '5') {
      int pinIndex = keyPressed - '1'; // Convert char to pin index
      activatePin(pinIndex); // Activate the pin
    }
    // Check for power level adjustment (H, J, K)
    else if (keyPressed == 'H' || keyPressed == 'J' || keyPressed == 'K') {
      int powerIndex = keyPressed - 'H'; // Convert char to power index
      setPowerLevel(powerIndex); // Set the power level
    }
    // Add a new sequence (A)
    else if (keyPressed == 'A') {
      addSequence(); // Add a new sequence
    }
    // Execute a sequence (E)
    else if (keyPressed == 'E') {
      executeSequence(); // Execute a sequence
    }
    // Read frequency (F)
    else if (keyPressed == 'F') {
      unsigned long frequency = readFrequency(); // Read the frequency
      Serial.print("Frequency: ");
      Serial.print(frequency);
      Serial.println(" Hz");
    }
  }
}

// Function to activate a specific finger
void activatePin(int pinIndex) {
  if (pinIndex >= 0 && pinIndex < 5) {
    if (activePin != -1) {
      ledcWrite(myHand[activePin], 0); // Turn off previously active pin
    }
    activePin = pinIndex;
    ledcWrite(myHand[activePin], powerLevels[0]); // Write current power level
  }
}

// Function to set the power level for a finger
void setPowerLevel(int powerIndex) {
  if (powerIndex >= 0 && powerIndex < 3) {
    powerLevels[powerIndex] = map(Serial.read(), 0, 255, 0, 255); // Read power level from serial
    if (activePin != -1) {
      ledcWrite(myHand[activePin], powerLevels[0]); // Write current power level to active pin
    }
  }
}

// Function to add a new sequence
void addSequence() {
  if (numSequences < 20) {
    Sequence newSequence;
    newSequence.numSteps = 0;
    
    while (newSequence.numSteps < 10) { // Maximum of 10 steps per sequence
      char keyPressed = Serial.read();
      if (keyPressed >= '1' && keyPressed <= '5') {
        newSequence.steps[newSequence.numSteps].pinIndex = keyPressed - '1';
        newSequence.steps[newSequence.numSteps].duration = Serial.parseInt();
        newSequence.numSteps++;
      }
      else if (keyPressed == 'X') { // End sequence input
        sequences[numSequences] = newSequence;
        numSequences++;
        break;
      }
    }
  }
}

// Function to execute a sequence
void executeSequence() {
  int seqIndex = Serial.parseInt();
  if (seqIndex >= 0 && seqIndex < numSequences) {
    Sequence seq = sequences[seqIndex];
    for (int i = 0; i < seq.numSteps; i++) {
      activatePin(seq.steps[i].pinIndex); // Activate pin
      delay(seq.steps[i].duration); // Delay
      ledcWrite(myHand[seq.steps[i].pinIndex], 0); // Turn off the pin after the duration
    }
  }
}

// Interrupt service routine to count pulses
void countPulse() {
  if (pulseCount == 0) {
    startTime = micros(); // Record the start time of the pulse
  }
  pulseCount++;
}

// Function to read frequency
unsigned long readFrequency() {
  noInterrupts(); // Disable interrupts while reading pulseCount
  unsigned long pulseCountCopy = pulseCount; // Make a copy of pulseCount
  unsigned long elapsedTime = micros() - startTime; // Calculate elapsed time
  pulseCount = 0; // Reset pulse count
  interrupts(); // Enable interrupts

  unsigned long frequency = 0;
  if (elapsedTime != 0) {
    frequency = pulseCountCopy * 1000000 / elapsedTime; // Calculate frequency in Hz
  }
  return frequency;
}