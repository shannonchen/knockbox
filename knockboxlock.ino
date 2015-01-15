#include <Servo.h> 
/* Detects patterns of knocks and triggers a servo to unlock a wooden box
 Adapted from The Secret Gumball Machine by Steve Hoefer 
 
 Shannon Chen
 05-833 Gadgets, Sensors and Activity Recognition
 Final Project: Knock Lock Box
 
*/

// Pin definitions
const int knockSensor = 0;         // Piezo sensor on analog pin 0.
const int programSwitch = 8;       // When this is high we program a new code
const int servoPin = 13;            // Servo that we're controlling. 
const int redLED = 4;              // Status LED for failed knock
const int greenLED = 3;            // Status LED for successful knock.
const int activityLED1 = 2;        // Lights up when the servo is moving.

const int threshold = 150;        // Minimum signal from the piezo to register as a knock. Maximum is 1023. Lower it is, the higher the sensitivity

const int rejectValue = 30;        // If an individual knock is off by this percentage of a knock we ignore. 10 is strict.
const int averageRejectValue = 20; // If the average timing of the knocks is off by this percent we ignore. 10 is strict.
const int debounceThreshold = 80;  
const int knockFadeTime = 150;     // milliseconds we allow a knock to fade before we listen for another one.

const int maximumKnocks = 20;       // Maximum number of knocks to listen for.
const int knockComplete = 1500;     // Longest time to wait for a knock before we assume that it's finished.

// Variables.
Servo lockServo;                  // The servo that dispenses a gumball
int secretCode[maximumKnocks] = {50, 25, 25, 50, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Initial setup: "Shave and a Hair Cut, two bits." 100=full note, 50=half note, 25=quarter note, etc.
int knockReadings[maximumKnocks];    // When someone knocks this array fills with delays between knocks. (A correct knock looks a lot like the line above).
int knockSensorValue = 0;            // Most recent reading of the knock sensor.
int programButtonPressed;    // ButtonState

void setup() {

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(activityLED1, OUTPUT);
  pinMode(programSwitch, INPUT);

  Serial.begin(9600);

  Serial.println("Start.");  
  
  digitalWrite(redLED, HIGH);      // Turn on all the lights so we can be sure they work.
  digitalWrite(greenLED, HIGH);
  digitalWrite(activityLED1, HIGH);

  delay(2000);
  lockServo.attach(servoPin);   // Initialize the servo.
  delay(50);         
  for (int i=0;i<150;i++){         // Reset the rotation of the servo.
    lockServo.write(135);         // Write the location a few times just in case the wheel is a long way off or binding somewhere.
    delay(20);
  }  
  
  delay(50);
  lockServo.detach();           // Detach the servo when not using it.

  digitalWrite(redLED, LOW);       // Turn off the lights, testing is over.
  digitalWrite(greenLED, LOW);
  digitalWrite(activityLED1, LOW);


}

void loop() {
  // Listen for any knock at all.
  knockSensorValue = analogRead(knockSensor);

  if ((digitalRead(programSwitch)==HIGH) && (knockSensorValue >=threshold) ){   // Is the program button pressed?
    programButtonPressed = true;    // Yes, so lets save that state.
    Serial.println("Button pressed");
   
    digitalWrite(redLED, HIGH);            // ...and turn on the the lights so we know we're programming.
    digitalWrite(greenLED, HIGH);
    listenToSecretKnock();
  } 
  
  else {
    programButtonPressed = false;
    // Serial.println("No button pressed");
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, LOW);
  }
  
  if (knockSensorValue >=threshold){
    listenToSecretKnock();
  }
} 

void listenToSecretKnock(){
  Serial.println("(knock started)");   

  int i = 0;
  for (i=0;i<maximumKnocks;i++){
    knockReadings[i]=0;
  }

  int currentKnockNumber=0;         	         // Increment for the array.
  int startTime=millis();           	         // Reference for when this knock started.
  int now=millis();

  digitalWrite(greenLED, HIGH);      	        // Light the green LED for knock feedback.

  delay(debounceThreshold);
  digitalWrite(greenLED, LOW); 

  do {
    // Listen for the next knock or wait for it to time out. 
    knockSensorValue = analogRead(knockSensor);

    if (knockSensorValue > threshold){                   // Got another knock...
      Serial.println(" knock!");
      now=millis();                                      // Record the delay time.
      knockReadings[currentKnockNumber] = now-startTime;
      currentKnockNumber ++;                             // Increment the counter.
      startTime=now;          
     
      digitalWrite(greenLED, HIGH);      	         // Light the knock LED.

      delay(debounceThreshold);                        // Debounce the knock sensor. (And turn the LED off halfway through.)
      digitalWrite(greenLED, LOW);  
      delay(debounceThreshold);   
    }

    now=millis();

  } 
  
  while ((now-startTime < knockComplete) && (currentKnockNumber < maximumKnocks));

  // We have a completed knock, lets see if it's valid
  if (programButtonPressed==false){  
    if (validateKnock() == true){      
      boxUnlock(); 
    } 
    else {
      failedKnock();  
    }
  } 
 
  else { // if programButtonPressed==true
    validateKnock();
    Serial.println("New lock stored.");
    
    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, HIGH);
    
    for (i=0;i<3;i++){
      delay(100);
      digitalWrite(redLED, HIGH);
      digitalWrite(greenLED, LOW);
      delay(100);
      digitalWrite(redLED, LOW);
      digitalWrite(greenLED, HIGH);  
    }
  }
}

// We got a good knock, so do something!
void boxUnlock(){
  Serial.println("Box unlocked!");
  int i=0;

  digitalWrite(greenLED, HIGH);       // Turn on this light so the user knows the knock was successful.
  digitalWrite(activityLED1, HIGH);   // Turn on the servo light

  lockServo.attach(servoPin);      // Now we start with the servo.
  delay(50);
  
  int lightStatus=HIGH;               // Set this so we can blink the LED as the servo cycles.
  
  for (int i=0;i<=135;i++){        
    if (i>80){
      lockServo.write(80);   // Limit the rotation of the servo to 80 degrees.  We write this a few times in case it had trouble getting there.
    } 
    else {
      lockServo.write(i);       // Tell servo to rotate to position in variable 'i'.
    }
    if (i%18==0){                  
      lightStatus =! lightStatus;
    }
    digitalWrite(activityLED1, lightStatus); 
    delay(10);
  }
  
  delay(100);   

  
  digitalWrite(activityLED1, LOW);   
  
  delay(3000);
  
  for (int i=135;i>=80;i--){          // Assume the user got what it needed. Lock the box
    lockServo.write(135);   
    delay(10);
  }   
  
  delay(50);
 
  lockServo.detach();             // Done with the servo, so detach it - fixes the jittery problem
  delay(100);
}

// We didn't like the knock.  Indicate displeasure.
void failedKnock(){
  Serial.println("Secret knock failed.");
  digitalWrite(greenLED, LOW);  
  digitalWrite(redLED, LOW);		
  for (int i=0;i<3;i++){					
    digitalWrite(redLED, HIGH);
    delay(100);
    digitalWrite(redLED, LOW);
    delay(100);
  }
}

// Checks if our knock matches the secret.
// TRUE = good knock
// FALSE = bad knock
boolean validateKnock(){
  int i=0;

  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;
  
  for (i=0;i<maximumKnocks;i++){
    if (knockReadings[i] > 0){
      currentKnockCount++;
    }
    if (secretCode[i] > 0){  					
      secretKnockCount++;
    }
    
    if (knockReadings[i] > maxKnockInterval){
      maxKnockInterval = knockReadings[i];
    }
  }
  
  // Save the new lock in the array
  if (programButtonPressed==true){
    Serial.println("Recording new knock");
      for (i=0;i<maximumKnocks;i++){ // normalize the times
        secretCode[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100); 
      }
    // And flash the lights in the recorded pattern to let us know it's been programmed.
    digitalWrite(greenLED, LOW);
    digitalWrite(redLED, LOW);
    delay(500);

    //Start playing back the knocks
    digitalWrite(greenLED, HIGH);
    digitalWrite(redLED, HIGH);  // First knock
    delay(40);
    for (i = 0; i < maximumKnocks ; i++){
      digitalWrite(greenLED, LOW);
      digitalWrite(redLED, LOW);  

      if (programButtonPressed==true){
        if (secretCode[i] > 0){                                   
          delay(map(secretCode[i],0, 100, 0, maxKnockInterval)); 
          digitalWrite(greenLED, HIGH);
          digitalWrite(redLED, HIGH);  
        }
      }
      delay(40);
      digitalWrite(greenLED, LOW);
      digitalWrite(redLED, LOW);  
    }
    return false; 	// We don't unlock the door when we are recording a new knock.
  }
  
  if (currentKnockCount != secretKnockCount){
    return false; 
  }
  

  int totaltimeDifferences=0;
  int timeDiff=0;
  
  for (i=0;i<maximumKnocks;i++){
    knockReadings[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100);      
    timeDiff = abs(knockReadings[i]-secretCode[i]);
    if (timeDiff > rejectValue){
      return false;
    }
    totaltimeDifferences += timeDiff;
  }

  if (totaltimeDifferences/secretKnockCount>averageRejectValue){
    return false; 
  }
  
  return true;
  
}



