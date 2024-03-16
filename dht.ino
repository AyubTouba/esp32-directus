#include "DHT.h"
#define TYPE DHT11
int sensePin=2;
DHT HT(sensePin,TYPE);

float humidity;
float tempC;
float tempF;

int setTime = 500;
void setup() {
  // put your setup code here, to run once:
 Serial.begin(9600);
 HT.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  humidity = HT.readHumidity();
  tempC = HT.readTemperature();
  tempF= HT.readTemperature(true);

  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print("Temperature C: ");
  Serial.println(tempC);

  delay(setTime);
}
