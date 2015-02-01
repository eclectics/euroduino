//
// Program ed3_rls
// 
//  Description: Random Looping Sequencer, with quantisation option and variable loop length
//
//  I/O Usage:
//
//    pot1: chance of change for rand looping seq 
//    pot2: Scale choice, full ccw is unquantised, or choice of available scales 
//    sw1: length of loop UP=4 bytes Middle=2 bytes down = 1 byte
//    sw2: quantisation range UP=full range Middle=1/2 range Down=1 octave
//    dig1in: clock in
//    dig2in:
//    cv1in: seem noisy, not used
//    cv2in:
//    Dig1Out: trigger bit change 
//    Dig2Out: trigger on no bit change
//    CV1Out: quantised low byte of loop
//    CV2Out: unquantised low byte of loop
//
// created Jan 31, 2015
// Gregor McNish
// http://github.com/eclectics
//
// ============================================================
//
//  License:
//
//  This software is licensed under the Creative Commons
//  "Attribution-NonCommercial license. This license allows you
//  to tweak and build upon the code for non-commercial purposes,
//  without the requirement to license derivative works on the
//  same terms. If you wish to use this (or derived) work for
//  commercial work, please contact  the author.
//
//  For more information on the Creative Commons CC BY-NC license,
//  visit http://creativecommons.org/licenses/
//
//  ================= start of global section ==================


// I/O pins used on the Euro-Duino module.
const int pot1=A2;
const int pot2=A3;
const int switches[]={A4,A5,7,2}; //1 up/down, 2 up/down
const int dig1in=8;
const int dig2in=9;
const int cv1in=A0;
const int cv2in=A1;
const int dig1out=3;
const int dig2out=4;
const int cv1out=5;
const int cv2out=6;
const byte UP=255;
const byte DOWN=128;
byte controls[]={pot1,pot2,cv1in,cv2in};
byte digout[]={dig1out,dig2out};
byte sw[2]={0,0}; //switch state
int ctrlvals[4];

// Variables uses by the sketch

volatile int clkState;

ISR (PCINT0_vect){ //isr for pins 8to13
//check only pin 8 RISING
//doesn't matter if triggered by other things
    clkState=digitalRead(dig1in);
}
//sequence is variable length so using an array to store it
//not rotating the array physically, just keeping different pointers to where we're at in the sequence
const byte seqlength=32;
byte seq[seqlength];
byte ss=0; //seq start/end
byte lowbyte;
int i,j;
const int trigTime=20; //trigger length in milliseconds
const int ctrlTime=10; //time between reading a control in milliseconds (round robin)
unsigned int b; //bit to be flipped
unsigned long timeon[]={0,0};
unsigned long m,ctrlread=0;
byte scale=0,range=0,chance=0,length=0;

byte notes[]={0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,100,104,108,112,116,120,124,128,132,136,140,144,148,152,156,161,165,169,173,177,181,185,189,193,197,201,205,209,214,218,222,226,230,234,238,242};
  //using a 12 digit binary number to represent in and out of scale, each digit corresponds to 1 note
  //It gets translated to an octal number because using binary representation is limited to 8 digits unfortunately
  //In octal each digit is 0-7 and represents 3 binary digits, So
  //do the note selection you want ie 101011010101 (c major/ keyboard white keys). Make sure there's 12 steps!
  //group into sets of 3 101 011 010 101
  //each group of three turns into an octal number according to
  //000=0,001=1, 010=2, 011=3, 100=4,101=5,110=6,111=7
  //stick a 0 on the front of this 4 digit number-- that makes it get interpreted as octal
  //add it to the set of scales below-- delete any you don't want
  //you can comment lines out if you might need them again by preceding with // (like this line) 
  //Just make sure each one is follwed by a comma except the last one
  // Don't take the existing ones seriously, I don't know much about music...

  //The scale is chosen by A0, so the more there are here, the more fiddly it is
int scales[]={0, //used for unquantised
              07777, //chromatic
              05325, //major 101 011 010 101 5325
              05532,     //minor natural 101 101 011 010  5532
              04420,  //minor arpeggio
              04422,  //minor7th arpeggio
              04220,  //major arpeggio
              04221,  //major7th arpeggio 
              02452    //pentatonic 010 100 101 010 2452
            };

  byte note=0,qnote=0; 
  byte curscale[61]; //this holds the notes we select out of the full range for the current scale
  byte scalenotes; //how many notes in the scale
  byte numnotes=255; 
  byte lastscale=0;
  byte topscale=sizeof(scales)/2-1; //sizeof returns bytes, but scales are ints. This just represents the highest array index in scales
  byte ctrl=0; //which knob to read each pass
  byte tout;

byte trigtime=20;
unsigned long time,ctrltime,swtime;
unsigned int rates[2]={1000,1000};
unsigned long timeouts[2]={0,0};

void setup() {
    // Set up I/O pins
    pinMode(dig1in, INPUT); 
    pinMode(dig2in, INPUT); 
    pinMode(dig1out, OUTPUT); 
    pinMode(dig2out, OUTPUT); 
    for (i=0;i<4;i++){
        pinMode(switches[i], INPUT_PULLUP);
    }

// Set up the sketch variables
   //set up random sequence -- this makes sure the random sequence starts out different each time
   randomSeed(millis());
   //could be replaced with
   //seq[]={0,1,0,0,1,...}; with 32 entries
   for (i=0;i<seqlength;i++){
    seq[i]=random(2);
   }

  // set up clock interrupt
    PCMSK0 |= bit (PCINT0);  // want pin 0
    PCIFR  |= bit (PCIF0);   // clear any outstanding interrupts
    PCICR  |= bit (PCIE0);   // enable pin change interrupts for D8 to D13

}


void loop(){
    if (clkState==HIGH){ //stuff only happens on clock
      clkState=LOW;
      
        //looping sequencer
          b= seq[ss]; //low bit
          tout=1;
          if (random(254)<chance){ //chance of bit flipping
            b=b?0:1;
            tout=0;
          } 
          //triggers
          digitalWrite(digout[tout],HIGH);
          timeon[tout]=millis()+trigTime;

          seq[ss]=b; //"rotate" flipped bit 
          ss=(ss+1)%length;//increment seq
          lowbyte=rlsbyte(); //work out current value of byte for position in sequence
          note=map(lowbyte,0,255,0,range);
          qnote=scale?curscale[note]:note;

          analogWrite(cv1out,255-qnote); //euroduino inverted -- quantised
          analogWrite(cv2out,255-note); //non-quantised
        }  

        m=millis();
    //turn off triggers
        for (i=0;i<2;i++){
            if (timeon[i] && m>timeon[i]){
                digitalWrite(digout[i],LOW);
                timeon[i]=0;
            } 
        }
   //reading controls in between clock pulses; read 1 every ctrltime
   if (m>ctrlread){
    switch(ctrl){
        case 0: //changing scale
         scale=map(analogRead(pot2),0,1023,0,topscale); //scale 
         if (lastscale!=scale){ //pick the notes for the scale
              lastscale=scale;
              if (scale){ //ie not 0
                  for(i=0,j=0;i<61;i++){ //we're just reading the scale info note by note and picking notes from the full set
                    if (scales[scale] & (04000>>i%12)){ //should this interval be in the selected scale
                      curscale[j++]=notes[i];
                    }
                  }
                  scalenotes=0; //how many notes per octave in this scale?
                  i=scales[scale];
                  while (i){
                     scalenotes+= i & 1; 
                     i=i>>1;
                  }
               numnotes=scalenotes*5-1;
              } else {
                  numnotes=255; //unquantised
              } 
          }
          break;
        case 1:
         rdsw();
         switch (sw[1]){
            case UP:
                range=numnotes;
                break;
            case DOWN:
                range=numnotes/4;
                break;
            default:
                range=numnotes/2;
                break;
         }
         break;
        case 2:
         chance=analogRead(pot1)>>2; //TM flip chance
         break;
        case 3:
         rdsw();
         switch (sw[0]){
            case UP:
                length=32;
                break;
            case DOWN:
                length=8;
                break;
            default:
                length=16;
                break;
         }
         break;
    }
    ctrl=(ctrl+1)%4;
    ctrlread=m+ctrlTime;
   }

}


void rdsw(){
//set state of switches
//using internal Pullup which means open is HIGH, and closed is low
    byte i,v;
    byte pins[4];
    for (i=0;i<4;i++){
       pins[i]=digitalRead(switches[i]);
    }
    sw[0]=0;
    if (pins[0]==LOW){
        sw[0]=UP;
    } else if (pins[1]==LOW){
        sw[0]=DOWN;
    }
    sw[1]=0;
    if (pins[2]==LOW){
        sw[1]=UP;
    } else if (pins[3]==LOW){
        sw[1]=DOWN;
    }
}


byte rlsbyte(){
    // if sl was constant can use this
        //lowbyte=lowbyte>>1;
        //lowbyte=lowbyte | seq[(ss+7)%sl]<<7
    // but if sl reduces may no longer be in the right zone, so will need sudden seq jump 
    // could just use above anyway, it would sort it self out within 7 clock pulses? might be a good segue?
 byte b=0;
    for (i=0;i<8;i++){
         b=b|seq[(ss+i)%length]<<i;
    }
    return b;
 }

//  ===================== end of program =======================

