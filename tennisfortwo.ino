#include <avr/io.h> 
#include <math.h> 
#include <stdlib.h>   //gives rand() function
 
#define g 0.8   
//gravitational acceleration (should be positive.)

#define ts 0.025 
// TimeStep

#define historyLength 7 

//control pins of TLC7528
#define outputSelector 10
#define CS 11
#define WR 12

void writex(uint8_t coord) {
  PORTB |= B00010000; //digitalWrite(WR,HIGH);//hold outputs- so new DAC data does not get sent out until we are ready
  PORTB &= B11111011; //digitalWrite(outputSelector,LOW);//select DACA
  PORTD = coord;
  PORTB &= B11101111;//digitalWrite(WR,LOW);//enable output again
}

void writey(uint8_t coord) {
  PORTB |= B00010000; //digitalWrite(WR,HIGH);//hold outputs- so new DAC data does not get sent out until we are ready
  PORTB |= B00000100; //digitalWrite(outputSelector,LOW);//select DACA
  PORTD = coord;
  PORTB &= B11101111;//digitalWrite(WR,LOW);//enable output again
}

void setup() { 
  Serial.begin(14400); //this is necessary because we're using D0 and D1 which are RX and TX on Arduino Nano
  float sintable[64];
  float costable[64];
  
  uint8_t xOldList[historyLength];
  uint8_t yOldList[historyLength];
  
  float xOld; // a few x & y position values
  float yOld = 0; // a few x & y position values
  float VxOld; //  x & y velocity values 
  float VyOld = 0; //  x & y velocity values 
  
  float Xnew, Ynew, VxNew, VyNew;
    
  uint8_t deadball = 0;
  uint8_t Langle, Rangle;
  uint8_t xp = 0;     
  uint8_t yp = 0;     
  
  unsigned int ADoutTemp;
  
  uint8_t NewBall = 101; 
  
  unsigned int NewBallDelay = 0;
  
  //Dummy variables: 
  uint8_t k = 0;
  uint8_t m = 0;
  
  uint8_t Serving = 0;
  //uint8_t CheckNet = 0;
  
  uint8_t ballside;
  uint8_t Lused = 0;
  uint8_t Rused = 0;
  
  // Create trig look-up table to keep things snappy.
  // 64 steps, total range: near pi.  Maybe a little more. 
  
   m = 0;
   while (m < 64)
    {
    sintable[m] = sin((float) 0.0647 * (float) m - (float) 2.07);   
    costable[m] = cos((float) 0.0647 * (float) m - (float) 2.07);
      m++;
  }
  
  //Outputs:
  DDRB = 255;
  DDRD = 255;
  writex(0);
  //PORTB = 0;
  writey(0);
  //PORTD = 0;
  
  //Inputs:
  DDRC = 0;
  PORTC = 0; // Pull-ups off.  We have our own external pull-ups anyway.
  
  //ADC Setup
  //PRR &=  ~(_BV(ICF1));  //Allow ADC to be powered up
  
  //ADC 3-5
  //analogReference(DEFAULT);
  //ADMUX = Serving * 4;  // Channel 0 or 4 (Serving = 0 if Left has ball. Serving = 1 if Right has ball.)
  //ADCSRA = 197  ;  // Enable ADC, start, prescale at 32 (197 = 11000101)
  ballside = 0;
  //analogReference(DEFAULT);

  for (byte i=0;i<8;i++){
    pinMode(i, OUTPUT);//set digital pins 0-7 as outputs
  }
  
  pinMode(outputSelector,OUTPUT);
  pinMode(CS,OUTPUT);
  pinMode(WR,OUTPUT);

  //set CS pin low
  digitalWrite(CS,LOW);

  
  // ---------
  // MAIN LOOP  
  // ---------
  
  for (;;) {
   
    if (ballside != (xOld >= 127)) {
      ballside = (xOld >= 127);
      if (ballside)
        Rused = 0;
      else
        Lused = 0; 
     //CheckNet = 1;
    }
  
    // IF ball has run out of energy, make a new ball!
    if ( NewBall > 10 ) {  
      NewBall = 0;
      deadball = 0; 
      NewBallDelay  = 1;
      Serving = (ballside == 0);
     
      if (Serving) {
        xOld = (float) 230;     
        VxOld = 0;// (float) -2*g; 
        ballside = 1;
        Rused = 0;
        Lused = 1; 
      } else {
        xOld = (float) 25; 
        VxOld = 0;// (float) 2*g; 
        ballside = 0;
        Rused = 1;
        Lused = 0; 
      }
      
      yOld = (float) 110;
      m = 0;
    
      while (m < historyLength) {
        xOldList[m] = xOld;
        yOldList[m] = yOld;
        m++;
      }
    }
    
       
    // Physics time!
    // x' = x + v*t + at*t/2
    // v' = v + a*t
    //
    // Horizontal (X) axis: No acceleration; a = 0.
    // Vertical (Y) axis: a = -g
    
     
    // If ADC conversion has finished
    //if ((ADCSRA & _BV(ADSC)) == 0) { 
      //ADoutTemp = ADCW;     // Read out ADC value 
      //Serial.println(ADCW);
      //Serial.println(analogRead(0));
    
      //We are using *ONE* ADC, but sequentially multiplexing it to sample
      //the two different input lines.
     
      //if (ADMUX == 0)
      if (ballside == 0)
        //Langle =  ADoutTemp >> 4; //ADoutTemp >> 2;
        Langle =  analogRead(0) >> 4; //ADoutTemp >> 2;
      else
        //Rangle =  ADoutTemp >> 4; // ADoutTemp >> 2;
        Rangle =  analogRead(2) >> 4; // ADoutTemp >> 2;
        
     // 64 angles allowed
     
      //ADMUX = 4*(ballside);  // Either ch 0 or ch 4.
      //ADCSRA |= _BV(ADSC);  // Start new ADC conversion 
    //}
  
    
    if (NewBallDelay) {
      if (((PINC & 2U) == 0) || ((PINC & 32U) == 0)) // 2U = 00000010 (2nd C port pin), 32U = 00100000 (5th C port pin)
        NewBallDelay = 10000;
    
      NewBallDelay++;
    
      if (NewBallDelay > 5000) // was 5000
        NewBallDelay = 0;
    
      m = 0;
      while (m < 255) {
        writey(yp);
        writex(xp);
        //PORTD = yp;
        //PORTB = xp;
        m++; 
      }
   
      VxNew = VxOld;
      VyNew = VyOld;
      Xnew =  xOld;
      Ynew = yOld;
    
    } else { 
    
      Xnew = xOld + VxOld;
      Ynew = yOld + VyOld - 0.5*g*ts*ts;
      
      VyNew = VyOld - g*ts;
      VxNew = VxOld;
    
      // Bounce at walls
      if (Xnew < 0) { 
        VxNew  *= -0.05; 
        VyNew *= 0.1; 
        Xnew = 0.1;
        deadball = 1;
        NewBall = 100;
      }
    
      if (Xnew > 255) { 
        VxNew  *= -0.05;  
        Xnew = 255;
        deadball = 1;
        NewBall = 100;
      }
     
      if (Ynew <= 0) {
        Ynew = 0;
      
        if (VyNew*VyNew < 10)  
          NewBall++;   
        
        if (VyNew < 0)
          VyNew *= -0.75;  
      }
    
      if (Ynew >= 255) {
        Ynew = 255; 
        VyNew = 0;  
      }  
  
      if (ballside) {
        if (Xnew < 127) {
          if (Ynew <= 63) {
            // Bounce off of net
            VxNew *= -0.5; 
            VyNew *= 0.5;       
            Xnew = 128.00;
            deadball = 1;
          } 
        }
      }
  
      if (ballside == 0) {
        if (Xnew > 127) {
          if (Ynew <= 63) {
            // Bounce off of net
            VxNew *= -0.5; 
            VyNew *= 0.5;       
            Xnew = 126.00;
            deadball = 1; 
          } 
        }
      }
  
      // Simple routine to detect button presses: works, if the presses are slow enough.
      if (xOld < 120) {
        if ((PINC & 2U) == 0) { 
          if ((Lused == 0) && (deadball == 0)) {  
            VxNew = 1.5*g*costable[Langle];   
            VyNew = g + 1.5*g*sintable[Langle];
            Lused = 1; 
            NewBall = 0;
          }
        }
      } else if (xOld > 134) { // Ball on right side of screen
        if ((PINC & 8U) == 0) { //was 32U
          if ((Rused == 0) && (deadball == 0)) {  
            VxNew = -1.5*g*costable[Rangle];   
            VyNew = g + -1.5*g*sintable[Rangle];
            Rused = 1;
            NewBall = 0;
          }
        }
      }
    }
   
    //Figure out which point we're going to draw. 
    xp =  (int) floor(Xnew);
    yp =  (int) floor(Ynew);
    //yp = 511 - (int) floor(Ynew);
    
 
    //Draw Ground and Net
    k = 0; // draws the ground and net multiple times per cycle. Increase to slow down, decrease to speed up.
    //while (k < 20) { 
    while (k < 7) {
      k++;
      m = 0;
  
      while (m < 127) {
        writey(0);
        writex(m);
        //PORTD = 0;    // Y-position
        //PORTB = m;    // X-position
        m++; 
      }

      writex(127);
      //PORTB = 127;   // X-position of NET
      m = 0;
      while (m < 61) {
        writey(m);
        //PORTD = m;    // Y-position
        m += 2; 
      }
  
      while (m > 1) {
        writey(m);
        //PORTD = m;    // Y-position
        m -= 2; 
      }

      writey(0);
      writex(127);
      //PORTD = 0;    // Y-position
      //PORTB = 127;   //Redundant, but allows time for scope trace to catch up.
      m = 127;
  
      while (m < 255) {
        writey(0);
        writex(m);
        //PORTD = 0;    // Y-position
        //PORTB = m;    // X-position
        m++; 
      }
    }
  
  
    m = 0;
    while (m < historyLength) {
      k = 0;
      while (k < (4*m*m)) {
        writex(xOldList[m]);
        writey(yOldList[m]);
        //PORTB = xOldList[m];
        //PORTD = yOldList[m];
        k++;
      }
      m++;
    }
  
  
    // Write the point to the buffer
    writey(yp);
    writex(xp);
    //PORTD = yp;
    //PORTB = xp; 
    m = 0;
    while (m < (historyLength - 1)) {
      xOldList[m] = xOldList[m+1];
      yOldList[m] = yOldList[m+1];
      m++; 
    }
  
    xOldList[(historyLength - 1)] = xp;
    yOldList[(historyLength - 1)] = yp;
  
    m = 0;
    while (m < 100) { 
      writey(yp);
      writex(xp);
      //PORTD = yp;
      //PORTB = xp;
      m++; 
    }
  
    //Age variables for the next iteration
    VxOld = VxNew;
    VyOld = VyNew; 
    xOld = Xnew;
    yOld = Ynew; 
    
  }

  return 0;
}

void loop() {}
