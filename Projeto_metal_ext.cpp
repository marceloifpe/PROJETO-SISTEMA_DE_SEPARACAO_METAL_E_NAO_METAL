#define F_CPU 16000000UL
#include <Servo.h>
#include <Ultrasonic.h>
#include <avr/io.h>
#include <avr/interrupt.h>

Servo meuServo;
Ultrasonic sensorUltrassonico(0b100, 0b101); // Pinos 4 (TRIG) e 5 (ECHO)

// Variáveis Globais
volatile bool detectouMetal = false;
unsigned long tempoInicial = 0;
bool objetoDetectado = false;
bool movimentoServo = false;
unsigned long tempoMovimento = 0;
const unsigned long tempoEsperaMetal = 2000;
const unsigned long tempoMaximoEspera = 4000;

void setup() {
    // Configuração de portas (PD4=TRIG como saída, PD2=entrada, PD6/PD7=LEDs)
    DDRD = 0b11010000;    // PD4(TRIG), PD6 e PD7 como saídas (0b11010000)
    PORTD = 0b00000100;   // Pull-up em PD2 (SENSOR_METAL)

    Serial.begin(9600);
    meuServo.attach(0b1001);  // Servo no pino 9 (0b00001001)
    meuServo.write(90);       // Posição inicial

    // Configuração de interrupções
    EICRA |= (1 << ISC01);    // INT0 borda de descida
    EIMSK |= (1 << INT0);     // Habilita INT0
    // Habilita interrupções globais
    sei();


}

void loop() {
    if (!objetoDetectado) {
        // Medição corrigida do sensor
        long microsec = sensorUltrassonico.timing();
        float distancia = sensorUltrassonico.convert(microsec, Ultrasonic::CM);

        // Exibição garantida no Serial Monitor
        Serial.print("Distância: ");
        Serial.print(distancia);
        Serial.println(" cm");

        if (distancia > 0 && distancia <= 10) {
            objetoDetectado = true;
            tempoInicial = millis();
            Serial.println("Objeto detectado, aguardando sensor de metal...");
        }
    }

    // Restante do código permanece igual...
    if (objetoDetectado && !movimentoServo) {
        if (detectouMetal && millis() - tempoInicial >= tempoEsperaMetal) {
            Serial.println("Metal detectado! Movendo para direita...");
            PORTD = 0b01000000;  // LED verde
            meuServo.write(45);
            tempoMovimento = millis();
            movimentoServo = true;
            detectouMetal = false;
        }
        else if (millis() - tempoInicial >= tempoMaximoEspera) {
            Serial.println("Nenhum metal detectado após 4s. Movendo para esquerda...");
            PORTD = 0b10000000;  // LED vermelho
            meuServo.write(135);
            tempoMovimento = millis();
            movimentoServo = true;
        }
    }

    if (movimentoServo && millis() - tempoMovimento >= 700) {
        meuServo.write(90);
        PORTD = 0b00000000;  // LEDs off
        objetoDetectado = false;
        movimentoServo = false;
        Serial.println("Retornando ao centro - Descarte realizado!");
    }
}

// Interrupções mantidas
ISR(INT0_vect) {
    detectouMetal = true;
    Serial.println("Metal detectado!");
}
