#include <16F1937.h>
#fuses INTRC_IO,NOWDT,PUT,MCLR,NOPROTECT,NOCPD,NOBROWNOUT,NOCLKOUT
#fuses NOIESO,NOFCMEN,NOWRT,NOVCAP,NOSTVREN,NODEBUG,NOLVP
#use delay(internal,clock=4000000)
#USE  RS232(BAUD=9600, XMIT=PIN_C6, RCV=PIN_C7) 

#BYTE TRISA = 0X08c
#BYTE TRISB = 0X08d
#BYTE TRISC = 0X08e
#BYTE TRISD = 0X08f
#BYTE TRISE = 0X90
#BYTE PORTA = 0x0c 
#BYTE PORTB = 0x0d
#BYTE PORTC = 0x0e
#BYTE PORTD = 0x0f
#BYTE PORTE = 0x10
#BYTE ANSELE = 0x190
#BYTE ANSELB = 0x18D
#BYTE ANSELA = 0x18C
#BYTE LATE   = 0x110
#BYTE ADCON0 = 0x9D
#BYTE ADCON1 = 0x9E

#define N1   porta,0
#define N2   porta,1
#define N3   porta,2
#define N4   porta,3
#define FC1   porta,4
#define btn1   portb,0
#define btn2   portb,1
#define btn3   portb,2
#define btn4   portb,3
#define enc1   portb,4
#define enc2   portb,5
#define enc11   portb,6
#define enc22   portb,7
#define M1   portc,0
#define M2   portc,1
#define M3   portc,2
#define M4   portc,3
#define TX   portc,6
#define RX   portc,7
#define led1   portd,4
#define led2   portd,5
#define led3   portd,6
#define led4   portd,7
#define disp1   porte,0
#define disp2   porte,1
#define disp3   porte,2
//#INCLUDE <stdlib.h>
#include <stdarg.h>
#include <math.h>




//////////////////////////////
/// definición de variables
//////////////////////////////

//Byte CONST display[10]= {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x67}; 
Byte CONST paso[4] = {0x08,0x04, 0x02 ,0x01};
int conteo1,conteo2, encoderact1, encoderact2,encoderact11, encoderact22;
int step1, step2, neg1, neg2, tiempo;

//////////////////////////////
/// definición de funciones
//////////////////////////////

void inicio();          //sincroniza el brazo en el step 0
void numero(int);       // muestra el numero de la velocidad en el display
void motor2Pos(int);    // activa el motor 2 antihorario 
void motor2Neg(int);    // activa el motor 2 horario
void motor1Pos(int);    // activa el motor 1 antihorario
void motor1Neg(int);    // activa el motor 2 horario
void neutro1();         // bobinas de motor 1 apagadas
void neutro2();         // bobinas de motor 2 apagadas
int serialMotor1();     // recibe los pasos para el motor 1
int serialMotor2();     // recibe los pasos para el motor 2
int velocidad();        // recibe el tiemmpo para la velocidad
int encoder1(int);      // pregunta si el encoder 1 cambia y suma pasos
int encoder2(int);      // pregunta si el encoder 1 cambia y suma pasos
int accion1();          //activa el motor 1
int accion2();          //activa el motor 2
int direccion1();       //busca la direccion de movimiento del motor 1
int direccion2();       //busca la direccion de movimiento del motor 2
void leds1(int, int);    //muestra la direccion de movimiento motor1
void leds2(int, int);    //muestra la direccion de movimiento motor2

///////////////////////////////////
/////// PROGRAMA PRINCIPAL ////////
///////////////////////////////////
void main ()
{
   ////////////////////////////////
   // configuracion de fusibles
   ////////////////////////////////
   setup_oscillator(OSC_4MHZ|OSC_INTRC|OSC_PLL_OFF); // deshabilita el PLL
   enable_interrupts(GLOBAL); //Habilita interrupción global 
   enable_interrupts(INT_RDA); //Habilita interrupción por resepción de puerto serialS
  
   //////////////////////////////////
   /// Configuración de puertos
   //////////////////////////////////
   TRISA = 0B00010000; //RA4 Entrada, los demás como salida
   TRISB = 0B11111111; // Puerto B como entrada
   TRISC = 0B10000000; // Puerto C como Salida
   TRISD = 0B00000000; // Puerto D como Salida
   TRISE = 0B00000000; // Puerto E como Salida
   
   ////////////////////////////////////////  
   // Inicialización de puertos de salida
   /////////////////////////////////////  
   
   PORTA = 0;
   PORTC = 0;
   PORTD = 0;
   PORTE = 0;
   tiempo= 0;
   conteo1=0;
   conteo2=0;
   encoderact1=0;
   encoderact2=0;
   encoderact11=0;
   encoderact22=0;
   neg1=0;
   neg2=0;
   step1=0;
   step2=0;
   int flag= 0;
   int flag1=0;
   int flag2=0;
   int tiemp, dir1, dir2, dir11, dir22;
   dir1=0;
   dir11=0;
   dir2=0;
   dir22=0;
   /////////////////////////
   // Bucle infinito
   /////////////////////////
   inicio();                                 //sincronizar brazo en angulo 0
   while (true)
   {  
      step1 = serialMotor1();                //preguntar por posicion a llegar
      //step2 = serialMotor2();
      tiempo = velocidad();                  //preguntar por tiempo de retardo
      flag1=0;
      flag2=1;
      flag=0;
      
      while(flag==0)
      {
         printf("%s%d%s%d","pasos: ",step1,"    velocidad:",tiemp);
         numero(tiempo);                           //mostrar la velocidad en el display
         flag1=accion1();                          // accionar motor y encoder
         //flag2=accion2();
         if ((flag1==1)&&(flag2==1)){flag=1;}      //indicar que llego a la posicion
         dir1 =direccion1();                       //preguntar posicion anterior
         while (!bit_test(btn1)) {numero(step1);}  //mostrar pasos
         //dir2 =direccion2();
         leds1(dir1, dir11);                       //activar led de posicion
         dir11=dir1;                               
         //leds2(dir2, dir22);
         //dir22=dir2;
         
      }
      neutro1();
      //neutro2();
   }
}

void inicio()                          
{  
   printf("inicio sincro");
   int setpoint1, setpoint2;
   setpoint1=0;
   setpoint2=0;
   while (setpoint1 == 0)
   {
      if (!bit_test(FC1))setpoint1 = 1;
      motor1Neg(15);
   }
   /*while (setpoint2 == 0)
   {
      if (!bit_test(enc2))setpoint2 = 1;
         motor2Neg(50); 
   }*/
   conteo1=0;
   conteo1=0;
   neutro1();
   neutro2();
   if (!bit_test(enc1)) {encoderact1=1;}
   if (!bit_test(enc11)) {encoderact2=1;}
   //if (!bit_test(enc2)) {encoderact11=1;}
   //if (!bit_test(enc22)) {encoderact22=1;}
   printf("fin sincro");
}
void neutro1()                   //motor 1 en neutro
{
   portc=0x00;
}
void neutro2()                   //motor 2 en neutro
{
   portc=0x00;
}

void motor1Pos(int tiempo)       //activar motor1 antihorario
{
   int time=tiempo;
   int i;
   for (i=0; i<=3; i++)
   {
      portc= paso[i];
      delay_ms(time);
   }
}
void motor1Neg(int tiempo)       //activar motor1 horario
{
   int time=tiempo;
   int i;
   for (i=0; i<=3; i++)
   {
      portc= paso[3-i];
      delay_ms(time);
   }
}

void motor2Pos(int tiempo)       //activar motor2 antihorario
{  
   int time=tiempo;
   int i;
   for (i=0; i<=3; i++)
   {
      porta= paso[i];
      delay_ms(time);
   }
}
void motor2Neg(int tiempo)       //activar motor2 horario
{  
   int time=tiempo;
   int i;
   for (i=0; i<=3; i++)
   {
      porta= paso[3-i];
      delay_ms(time);
   }
}

void numero(int conte)           //mostrar dato en display
{
   int count = conte;
   bit_set(disp1);
   bit_set(disp1);
   portd=count;
}

int serialmotor1(){              //leer siguiente posion del brazo1 desde el pc
   int dato, result;
   puts("posicion motor 1");
   dato=getc();
   //dato=dato-48;
   if (dato < conteo1)
   {
      neg1= 1;
      result = conteo1-dato;
   }
   else
   {
      neg1=0;
      result= dato-conteo1;
   }
   printf("%d",result);
   return(result);
}
int serialmotor2(){              //leer siguiente posion del brazo2 desde el pc
   int dato, result;
   puts("posicion motor 2");
   dato=getc();
   //dato=dato-48;
   if (dato < conteo2)
   {
      neg2= 1;
      result = conteo2-dato;
   }
   else
   {
      result= dato-conteo2;
   }
   return(result);
}

int velocidad(){                 //leer tiempo de retardo entre pasos
   int dato, result;
   puts("velocidad");
   dato=getc();
   result = (int)(-0.7*pow(dato,2))+53+dato;
   return(result);
}

int accion1()                    //accionar el motor 1 segun encoder1
{
   int flag=0;
   if (step1 != 0)
   {  
      conteo1=encoder1(conteo1);
      if (neg1==1) 
      {  
         motor1Neg(tiempo);
      }
      else
      {  
         motor1Pos(tiempo);
      }
   }
   else
   {
      flag=1;
   }
   return (flag);
}
int accion2()                    //accionar el motor 2 segun encoder2
{
   int flag=0;
   if (step2 != 0)
   {
      conteo2=encoder2(conteo2);
      if (neg2==1)
      {  
         motor2Neg(tiempo); 
      }
      else
      {  
         motor2Pos(tiempo); 
      }
   }
   else
   {
      flag=1;
   }
   return (flag);
}

int encoder1(int step)          //preguntar si los encoders del brazo1 tiene flanco de subida
{
   int conteo = step;
   int estado = encoderact1;
   int dir1, dir11;
      if (!bit_test(enc1))     
      {  
        if (estado!=1)
        {
            if (neg1==1)
            {
               dir1=direccion1();
               conteo--;
            }
            else
            {
               conteo++;
            }
            encoderact1=1;
            dir1=direccion1();
            leds1(dir1,dir11);
            dir11=dir1;
            //printf("%d",conteo);
            step1--;
        }
      }
      else {encoderact1=0;}
      if (!bit_test(enc1)){encoderact11=1;}
      else {encoderact11=0;}
    return(conteo);
}
int encoder2(int step)          //preguntar si los encoders del brazo2 tiene flanco de subida
{
   int conteo = step;
   int estado = encoderact2;
      if (!bit_test(enc2))     
      {  
        if (estado!=1)
        {
            if (neg2==1)
            {
               conteo--;
            }
            else
            {
               conteo++;
            }
            encoderact2=1;
            step2--;
            //printf("%d",conteo);
        }
      }
     else {encoderact2=0;}
     if (!bit_test(enc2)){encoderact22=1;}
     else {encoderact22=0;}
    return(conteo);
}

int direccion1()                //preguntar estado anterior y el actual de los encoders del brazo 1
{
    int dato;
   if (encoderact1==0)
      {if(encoderact11==0)
      {dato=0;}}                    //posicion 00
   if (encoderact1==0)
      {if(encoderact11==1)
      {dato=1;}}                    //posicion 01
   if (encoderact1==1)
      {if(encoderact11==0)
      {dato=2;}}                    //posicion 10
   if (encoderact1==1)
      {if(encoderact11==1)
      {dato=3;}}                    //posicion 11
   return (dato);
}
int direccion2()                //preguntar estado anterior y el actual de los encoders del brazo 2
{
   int dato;
   if (encoderact2==0)
      {if(encoderact22==0)
      {dato=0;}}
   if (encoderact2==0)
      {if(encoderact22==1)
      {dato=1;}}
   if (encoderact2==1)
      {if(encoderact22==0)
      {dato=2;}}
   if (encoderact2==1)
      {if(encoderact22==1)
      {dato=3;}}
    return (dato);
}

void leds1 (int dato1, int dato11)  //activa el led que indica la direccion del motor1
{
   if (dato11 == 0){
         if (dato1== 2) {bit_set(led1);bit_clear(led2);}    //derecha
         if (dato1== 1) {bit_set(led2);bit_clear(led1);}    //izquierda
   }
   if (dato11 == 1){
         if (dato1== 3) {bit_set(led1);bit_clear(led2);}    //derecha
         if (dato1== 0) {bit_set(led2);bit_clear(led1);}    //izquierda
   }
   if (dato11 == 2){
         if (dato11== 0) {bit_set(led1);bit_clear(led2);}    //derecha
         if (dato11== 3) {bit_set(led2);bit_clear(led1);}    //izquierda
   }
   if (dato1 == 3){
         if (dato11== 2) {bit_set(led1);bit_clear(led2);}    //derecha
         if (dato11== 1) {bit_set(led2);bit_clear(led1);}    //izquierda
   }
}
void leds2 ( int dato2, int dato22) //activa el led que indica la direccion del motor2
{
   if (dato2 == 0){
         if (dato22== 2) {bit_set(led3);bit_clear(led4);}    //derecha
         if (dato22== 1) {bit_set(led4);bit_clear(led3);}    //izquierda
   }
   if (dato2 == 1){
         if (dato22== 0) {bit_set(led3);bit_clear(led4);}    //derecha
         if (dato22== 3) {bit_set(led4);bit_clear(led3);}    //izquierda
   }
   if (dato2 == 2){
         if (dato22== 3) {bit_set(led3);bit_clear(led4);}    //derecha
         if (dato22== 0) {bit_set(led4);bit_clear(led3);}    //izquierda
   }
   if (dato2 == 3){
         if (dato22== 1) {bit_set(led3);bit_clear(led4);}    //derecha
         if (dato22== 2) {bit_set(led4);bit_clear(led3);}    //izquierda
   }
}
