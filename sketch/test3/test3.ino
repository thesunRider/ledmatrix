#define R1_PIN_DEFAULT 39
#define G1_PIN_DEFAULT 13
#define B1_PIN_DEFAULT 48
#define R2_PIN_DEFAULT 47
#define G2_PIN_DEFAULT 14
#define B2_PIN_DEFAULT 21
#define A_PIN_DEFAULT  40
#define B_PIN_DEFAULT  11 //was 19
#define C_PIN_DEFAULT  41
#define D_PIN_DEFAULT  12 // 15
#define E_PIN_DEFAULT  -1 // required for 1/32 scan panels, like 64x64. Any available pin would do, i.e. IO32
#define LAT_PIN_DEFAULT 20
#define OE_PIN_DEFAULT  1
#define CLK_PIN_DEFAULT 42



void setup() {
  // put your setup code here, to run once:
  pinMode(R1_PIN_DEFAULT,OUTPUT);
  pinMode(G1_PIN_DEFAULT,OUTPUT);
  pinMode(B1_PIN_DEFAULT,OUTPUT);
  pinMode(R2_PIN_DEFAULT,OUTPUT);
  pinMode(G2_PIN_DEFAULT,OUTPUT);
  pinMode(B2_PIN_DEFAULT,OUTPUT);
  pinMode(A_PIN_DEFAULT,OUTPUT);
  pinMode(B_PIN_DEFAULT,OUTPUT);
  pinMode(C_PIN_DEFAULT,OUTPUT);
  pinMode(D_PIN_DEFAULT,OUTPUT);
  pinMode(LAT_PIN_DEFAULT,OUTPUT);
  pinMode(OE_PIN_DEFAULT,OUTPUT);
  pinMode(CLK_PIN_DEFAULT,OUTPUT);
  
}

void loop() {
  // put your main code here, to run repeatedly:
digitalWrite(R1_PIN_DEFAULT,HIGH);
digitalWrite(G1_PIN_DEFAULT,HIGH);
digitalWrite(B1_PIN_DEFAULT,HIGH);
digitalWrite(R2_PIN_DEFAULT,HIGH);
digitalWrite(G2_PIN_DEFAULT,HIGH);
digitalWrite(B2_PIN_DEFAULT,HIGH);
digitalWrite(A_PIN_DEFAULT,HIGH);
digitalWrite(B_PIN_DEFAULT,HIGH);
digitalWrite(C_PIN_DEFAULT,HIGH);
digitalWrite(D_PIN_DEFAULT,HIGH);
digitalWrite(LAT_PIN_DEFAULT,HIGH);
digitalWrite(OE_PIN_DEFAULT,HIGH);
digitalWrite(CLK_PIN_DEFAULT,HIGH);
delay(1);
digitalWrite(R1_PIN_DEFAULT,LOW);
digitalWrite(G1_PIN_DEFAULT,LOW);
digitalWrite(B1_PIN_DEFAULT,LOW);
digitalWrite(R2_PIN_DEFAULT,LOW);
digitalWrite(G2_PIN_DEFAULT,LOW);
digitalWrite(B2_PIN_DEFAULT,LOW);
digitalWrite(A_PIN_DEFAULT,LOW);
digitalWrite(B_PIN_DEFAULT,LOW);
digitalWrite(C_PIN_DEFAULT,LOW);
digitalWrite(D_PIN_DEFAULT,LOW);
digitalWrite(LAT_PIN_DEFAULT,LOW);
digitalWrite(OE_PIN_DEFAULT,LOW);
digitalWrite(CLK_PIN_DEFAULT,LOW);
delay(1);

}
