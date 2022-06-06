#include <avr/interrupt.h>
#include <avr/io.h>
#include "pitches.h"
#include "themes.h"

//#define PIN_A 9             // arduino speaker output +
//#define PIN_B 8             // arduino speaker output -

boolean isPlaying = false;
boolean silent = false;
int curNote = 0;
unsigned int curNotePitch = 0;
unsigned int curNoteLenght = 0;

int melodySize;
int *notes;
int *duration;


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 15200 bits per second:
  Serial.begin(115200);
  
  DDRB |= B00000011;   //D8_Out, //D9_Out
  PORTB &= B11111100;  //D8->Low, //D9->Low
    //pinMode(D8, OUTPUT);// выход динамика pin8 -
    //pinMode(D9, OUTPUT);// выход динамика pin9 +
    
    //Play_MarioUW();
    Play_Intro();
}

void play(int fff){
  // print out the value you read:
  //Serial.println(fff);
  
  if (isPlaying) return; 
  
  //Play_Pirates();
  //Play_Mario();
  Play_CrazyFrog();
  delay(100);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A2);
  int sensorValue2 = 0;
  
  if (sensorValue > 500) {    // ести напряжение на входе А2 больше 2.4 вольт
    delay(10);
    sensorValue2 = analogRead(A2);
    if(abs(sensorValue2 - sensorValue) < 10) {
      play(sensorValue);
    }
  }
  delay(100);        // delay in between reads for stability
}


//**************Timer****************************
#define DIV_0    TCCR1B = (1 << CS10) //Делитель 0
#define DIV_8    TCCR1B = (1 << CS11) //Делитель 8
#define DIV_64   TCCR1B = ((1 << CS11) | (1 << CS10)) //Делитель 64
#define DIV_256  TCCR1B = (1 << CS12) //Делитель 256
#define DIV_1024 TCCR1B = ((1 << CS12) | (1 << CS10)) //Делитель 1024

volatile uint16_t dub_tcnt1;



void stopSound(void)
{
  TIMSK1 &= ~(1<<TOIE1);    //запретить прерывания по таймеру1
  isPlaying = false;
}

void resumeSound(void)
{
  TIMSK1 |= (1<<TOIE1); //Продолжить отсчет, (разрешить прерывания по таймеру1)
}

ISR(TIMER1_OVF_vect)          // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
    //TCNT1 = dub_tcnt1;
    // Toggle PIN
      if ( !silent ) {
        PORTB  = ( (PORTB & B11111101) | ((PORTB & B00000001)<<1) ); // D9 = D8;
        PORTB ^= B00000001;                                          // D8 = D8 ^ 1;
      } else {
        PORTB &= B11111100;  // если пауза, то не дергать выводами (не звучать)
      //D8_Low; //digitalWrite( D8, LOW );
      //D9_Low; //digitalWrite( D9, LOW );
      }
      
    if (--curNoteLenght < 1) {
      PORTB &= B11111100;   // устоновить 0 на обеих выходах 8и9
      TIMSK1 &= ~(1<<TOIE1);    //запретить прерывания по таймеру1
      nextNote(-1);  // play next note
    };    
}


void nextNote(int n){
  if (n > -1) { curNote = n; }
  
  if (curNote < melodySize - 1) {
     isPlaying = true;
     playNote(notes[curNote], duration[curNote]);
     curNote++;
  } else {
    isPlaying = false; 
  }
    
}


#define RESOLUTION 65536    // Timer1 is 16 bit
unsigned int pwmPeriod;
unsigned char clockSelectBits;
char oldSREG;         // To hold Status Register while ints disabled

void playNote(long a, int b)
{
  if (a < 10) {
    curNotePitch=100;
    silent = true;
  } else  {
    curNotePitch = a;
    silent = false;
  }
  
  TIMSK1 = _BV(TOIE1);        // sets the timer overflow interrupt enable bit
  TCCR1A = 0;                 // clear control register A 
  TCCR1B = _BV(WGM13);        // set mode 8: phase and frequency correct pwm, stop the timer
  
  long cycles = (F_CPU / 2000000 ) * ( 500000 / a); //curNotePitch;    
  // the counter runs backwards after TOP, interrupt is at BOTTOM so divide microseconds by 2
  if(cycles < RESOLUTION)              clockSelectBits = _BV(CS10);              // no prescale, full xtal
  else if((cycles >>= 3) < RESOLUTION) clockSelectBits = _BV(CS11);              // prescale by /8
  else if((cycles >>= 3) < RESOLUTION) clockSelectBits = _BV(CS11) | _BV(CS10);  // prescale by /64
  else if((cycles >>= 2) < RESOLUTION) clockSelectBits = _BV(CS12);              // prescale by /256
  else if((cycles >>= 2) < RESOLUTION) clockSelectBits = _BV(CS12) | _BV(CS10);  // prescale by /1024
  else        cycles = RESOLUTION - 1, clockSelectBits = _BV(CS12) | _BV(CS10);  // request was out of bounds, set as maximum
  
  oldSREG = SREG;       
  cli();              // Disable interrupts for 16 bit register access
    curNoteLenght = ((4) * curNotePitch) / b;
    //Serial.println(a);
    //Serial.println(cycles);
    //Serial.println(curNoteLenght);
  ICR1 = pwmPeriod = cycles;                                          // ICR1 is TOP in p & f correct pwm mode
  SREG = oldSREG;
  
  TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
  TCCR1B |= clockSelectBits;  // reset clock select register, and starts the clock
  TIMSK1 |= (1<<TOIE1); //Продолжить отсчет, (разрешить прерывания по таймеру1)
}


void Play_Pirates()
{ 
    melodySize = sizeof(Pirates_note)/sizeof(int);
    duration = Pirates_duration;
    notes = Pirates_note;
    nextNote(0);
}

void Play_CrazyFrog()
{
    melodySize = sizeof(CrazyFrog_note)/sizeof(int);
    duration = CrazyFrog_duration;
    notes = CrazyFrog_note;
    nextNote(0);
}

void Play_MarioUW()
{
    melodySize = sizeof(MarioUW_note)/sizeof(int);
    duration = MarioUW_duration;
    notes = MarioUW_note;
    nextNote(0);
}

void Play_Titanic()
{
    melodySize = sizeof(Titanic_note)/sizeof(int);
    duration = Titanic_duration;
    notes = Titanic_note;
    nextNote(0);
}

void Play_Intro()
{
    melodySize = sizeof(Intro_note)/sizeof(int);
    duration = Intro_duration;
    notes = Intro_note;
    nextNote(0);
}

