#define LED_PIN_RED D6   // LED connected to D0
#define LED_PIN_YELLOW D3
#define LED_PIN_GREEN D4
#define SWITCH_PIN D0 // Switch connected to D6
#define LED_PEDESTRIAN 21

  int car = true;

void setup() {
    pinMode(LED_PIN_RED, OUTPUT);            // Set LED as output
    pinMode(LED_PIN_YELLOW, OUTPUT);
    pinMode(LED_PIN_GREEN, OUTPUT);
    pinMode(LED_PEDESTRIAN, OUTPUT);
    pinMode(SWITCH_PIN, INPUT_PULLUP);   // Set switch as input with internal pull-up

    digitalWrite(LED_PIN_GREEN, HIGH);
    digitalWrite(LED_PIN_RED, LOW);
    digitalWrite(LED_PIN_YELLOW, LOW);
    digitalWrite(LED_PEDESTRIAN, LOW);
}

void loop() {
 if(!car && (digitalRead(LED_PIN_GREEN) == HIGH || digitalRead(LED_PIN_RED) == HIGH) && digitalRead(SWITCH_PIN) == LOW){
    digitalWrite(LED_PIN_GREEN, HIGH);
    digitalWrite(LED_PIN_RED, LOW);
    digitalWrite(LED_PIN_YELLOW, LOW);
    digitalWrite(LED_PEDESTRIAN, HIGH);
    delay(3000);
 }else if((car || digitalRead(SWITCH_PIN) == HIGH) && digitalRead(LED_PIN_GREEN) == HIGH){
    digitalWrite(LED_PIN_YELLOW, HIGH);
    digitalWrite(LED_PIN_RED, LOW);
    digitalWrite(LED_PIN_GREEN, LOW);
    digitalWrite(LED_PEDESTRIAN, HIGH);
    delay(2000);
 }else if(digitalRead(SWITCH_PIN) == HIGH && (digitalRead(LED_PIN_YELLOW) == HIGH || digitalRead(LED_PIN_RED) == HIGH)){
    digitalWrite(LED_PIN_RED, HIGH);
    digitalWrite(LED_PIN_YELLOW, LOW);
    digitalWrite(LED_PIN_GREEN, LOW);
    digitalWrite(LED_PEDESTRIAN, LOW);
    delay(5000);
 }else if(digitalRead(SWITCH_PIN) == LOW && car && (digitalRead(LED_PIN_YELLOW) == HIGH || digitalRead(LED_PIN_RED) == HIGH)){
    digitalWrite(LED_PIN_GREEN, LOW);
    digitalWrite(LED_PIN_YELLOW, LOW);
    digitalWrite(LED_PIN_RED, HIGH);
    digitalWrite(LED_PEDESTRIAN, HIGH);
    delay(5000);
 }
}
