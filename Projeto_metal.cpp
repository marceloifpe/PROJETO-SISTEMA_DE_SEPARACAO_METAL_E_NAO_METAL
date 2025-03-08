#define F_CPU 16000000UL
#include <Servo.h>
#include <Ultrasonic.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define SERVO_PIN 9   // Pino do servo motor
#define TRIG_PIN 4    // Trigger do ultrassônico
#define ECHO_PIN 5    // Echo do ultrassônico

Servo meuServo;
Ultrasonic sensorUltrassonico(TRIG_PIN, ECHO_PIN);
//Variáveis Globais
volatile bool detectouMetal = false;  // Flag para metal detectado
unsigned long tempoInicial = 0;       // Tempo de detecção do objeto
bool objetoDetectado = false;         // Flag para objeto detectado
bool movimentoServo = false;          // Flag para controle do movimento do servo
unsigned long tempoMovimento = 0;     // Tempo para movimento do servo
const unsigned long tempoEsperaMetal = 2000;  // Tempo de espera para detecção de metal (2 segundos)
const unsigned long tempoMaximoEspera = 4000; // Tempo máximo de espera para detecção de metal (4 segundos)

void setup() {
    // Configura os pinos de entrada e saída
    DDRD &= ~(1 << PD2);  // Configura SENSOR_METAL como entrada
    DDRD |= (1 << PD6) | (1 << PD7);  // Configura LEDs como saída

    // Inicializa o servo e o sensor ultrassônico
    Serial.begin(9600);
    meuServo.attach(SERVO_PIN);
    meuServo.write(90);  // Posição inicial do servo

    // Configura a interrupção externa (INT0) para borda de descida
    EICRA |= (1 << ISC01);  // Configura INT0 para borda de descida
    EIMSK |= (1 << INT0);   // Habilita INT0

    // Configura Timer2 para interrupção por overflow (1ms aprox.)
    cli();  // Desabilita interrupções globais
    TCCR2B = (1 << CS22);  // Prescaler de 64 (aproximadamente 1ms por overflow)
    TIMSK2 = (1 << TOIE2);  // Habilita a interrupção por overflow do Timer2
    TCNT2 = 5;  // Inicializa o Timer2 com valor 5 (aproximadamente 1ms)
    sei();  // Habilita interrupções globais
}

void loop() {
    // Detecção de distância do ultrassônico
    if (!objetoDetectado) {
        long microsec = sensorUltrassonico.timing();
        float distancia = sensorUltrassonico.convert(microsec, Ultrasonic::CM);

        Serial.print("Distância: ");
        Serial.print(distancia);
        Serial.println(" cm");

        if (distancia > 0 && distancia <= 10) {
            objetoDetectado = true;
            tempoInicial = millis();  // Marca o tempo de detecção do objeto
            Serial.println("Objeto detectado, aguardando sensor de metal...");
        }
    }

    // Controle do movimento do servo
    if (objetoDetectado && !movimentoServo) {
        if (detectouMetal && millis() - tempoInicial >= tempoEsperaMetal) {
            // Metal detectado dentro de 2 segundos
            Serial.println("Metal detectado! Movendo para direita...");
            PORTD |= (1 << PD6);  // Acende LED verde
            PORTD &= ~(1 << PD7);  // Apaga LED vermelho
            meuServo.write(45);  // Move o servo para a esquerda
            tempoMovimento = millis();  // Marca o tempo do movimento
            movimentoServo = true;  // Impede movimento repetido
            detectouMetal = false;  // Reseta a flag de detecção de metal
        } else if (millis() - tempoInicial >= tempoMaximoEspera) {
            // Metal não detectado após 4 segundos
            Serial.println("Nenhum metal detectado após 4s. Movendo para esquerda...");
            PORTD |= (1 << PD7);  // Acende LED vermelho
            PORTD &= ~(1 << PD6);  // Apaga LED verde
            meuServo.write(135);  // Move o servo para a direita
            tempoMovimento = millis();  // Marca o tempo do movimento
            movimentoServo = true;  // Impede movimento repetido
        }
    }

    // Retorna o servo à posição inicial após 700ms
    if (movimentoServo && millis() - tempoMovimento >= 700) {
        meuServo.write(90);  // Retorna o servo para a posição inicial
        PORTD &= ~((1 << PD6) | (1 << PD7));  // Apaga LEDs
        objetoDetectado = false;  // Reseta a detecção de objeto
        movimentoServo = false;  // Reseta o controle do movimento
        Serial.println("Retornando ao centro descarte Realizado !...");
    }
}

// Interrupção externa para detecção de metal (INT0)
ISR(INT0_vect) {
    detectouMetal = true;  // Marca que o metal foi detectado
    Serial.println("Metal detectado!");
}

// Interrupção do Timer2 para overflow (aproximadamente 1ms)
ISR(TIMER2_OVF_vect) {
    TCNT2 = 5;  // Reinicia o timer
}
