void setup() {
  // On ouvre le port série à 9600 bits par seconde
  Serial.begin(9600); 
}

void loop() {
  // On vérifie si des données sont arrivées
  if (Serial.available() > 0) {
    // On lit ce qui arrive (juste pour vider le tampon)
    String data = Serial.readString();
    
    // On répond
    Serial.println("Hello Python! I received your message.");
  }
}
