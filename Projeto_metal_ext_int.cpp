#define F_CPU 16000000UL
#include <Servo.h>
#include <Ultrasonic.h>
#include <avr/io.h>
#include <avr/interrupt.h>

Servo meuServo;
Ultrasonic sensorUltrassonico(0b100, 0b101); // Pinos 4 (TRIG) e 5 (ECHO)

// Variáveis Globais
volatile bool detectouMetal = false;
volatile int contador2s = 0;    // Contador para 2 segundos
volatile int contador4s = 0;    // Contador para 4 segundos
volatile int contadorMovServo = 0; // Contador para movimento do servo
bool objetoDetectado = false;
bool movimentoServo = false;

void setup() {
    // Configuração de portas (PD4=TRIG como saída, PD2=entrada, PD6/PD7=LEDs)
    DDRD = 0b11010000;    // PD4(TRIG), PD6 e PD7 como saídas
    PORTD = 0b00000100;   // Pull-up em PD2

    Serial.begin(9600);
    meuServo.attach(0b1001);  // Servo no pino 9
    meuServo.write(90);

    // Configuração de interrupções
    EICRA |= (1 << ISC01);    // INT0 borda de descida
    EIMSK |= (1 << INT0);     // Habilita INT0

    // Configura Timer2 para 1ms (prescaler 64)
    cli();
    TCCR2B |= (1 << CS22);    // Prescaler 64
    TIMSK2 |= (1 << TOIE2);   // Habilita overflow
    TCNT2 = 5;                // Inicializa contador
    sei();
}

void loop() {
    if (!objetoDetectado) {
        long microsec = sensorUltrassonico.timing();
        float distancia = sensorUltrassonico.convert(microsec, Ultrasonic::CM);

        Serial.print("Distância: ");
        Serial.print(distancia);
        Serial.println(" cm");

        if (distancia > 0 && distancia <= 8) {
            objetoDetectado = true;
            contador2s = 0;
            contador4s = 0;
            Serial.println("Objeto detectado, aguardando sensor de metal...");
        }
    }

    if (objetoDetectado && !movimentoServo) {
        int cont2s, cont4s;
        cli();
        cont2s = contador2s;
        cont4s = contador4s;
        sei();

        if (detectouMetal && cont2s >= 2000) { // 2000 ms = 2 segundos
            Serial.println("Metal detectado! Movendo para direita...");
            PORTD = 0b01000000;  // LED verde
            meuServo.write(45);
            contadorMovServo = 0; // Inicia contagem para retorno
            movimentoServo = true;
            detectouMetal = false;
        }
        else if (cont4s >= 4000) { // 4000 ms = 4 segundos
            Serial.println("Nenhum metal detectado após 4s. Movendo para esquerda...");
            PORTD = 0b10000000;  // LED vermelho
            meuServo.write(135);
            contadorMovServo = 0; // Inicia contagem para retorno
            movimentoServo = true;
        }
    }

    if (movimentoServo) {
        int contMovServo;
        cli();
        contMovServo = contadorMovServo;
        sei();

        if (contMovServo >= 700) { // 700 ms para retornar ao centro
            meuServo.write(90);
            PORTD = 0b00000000;  // LEDs off
            objetoDetectado = false;
            movimentoServo = false;
            contadorMovServo = 0; // Reset
            Serial.println("Retornando ao centro - Descarte realizado!");
        }
    }
}

// Interrupção do sensor de metal
ISR(INT0_vect) {
    detectouMetal = true;
    Serial.println("Metal detectado!");
}

// Interrupção do Timer2 (1ms)
ISR(TIMER2_OVF_vect) {
    TCNT2 = 5; // Reinicia contador

    if (objetoDetectado) {
        contador2s++;
        contador4s++;
    }

    if (movimentoServo) {
        contadorMovServo++;
    }
}
