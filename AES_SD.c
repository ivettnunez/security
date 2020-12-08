#include <io.h>
#include <delay.h>
#include <stdio.h>
#include <string.h>
#include <ff.h>
#include <display.h>

#include "encrypt_decrypt.c"

#asm
    .equ __lcd_port=0x11
    .equ __lcd_EN=4
    .equ __lcd_RS=5
    .equ __lcd_D4=0
    .equ __lcd_D5=1
    .equ __lcd_D6=2
    .equ __lcd_D7=3
#endasm

char NombreArchivo[17];
char Cadena[15];

unsigned long int mseg=0,tiempo;
char opcion;
unsigned char i;
unsigned char Llave[176];   //Llave original (0 a 15) y extendida el resto
unsigned long NumBloques,j,count;
unsigned char error;
unsigned char Buffer[16];
unsigned char Buffer2[16];


void ImprimeProgreso(unsigned long p)
{
    char buff[17];
    MoveCursor(0,1);
    StringLCD("               ");   
    sprintf(buff, "%lu/%lu",p, NumBloques);
    MoveCursor(0,1);
    StringLCDVar(buff);
}


void ImprimeError(int Err)
{
    switch (Err)
    {
        case 1:
            printf("(1) A hard error occured in the low level disk I/O layer");
            break;
        case 2:
            printf("(2) Assertion failed");
            break;
        case 3:
            printf("(3) The physical drive doesn't work ");
            break;   
        case 4:
            printf("(4) Could not find the file");
            break;   
        case 5:
            printf("(5) Could not find the path");
            break;   
        case 6:
            printf("(6) The path name format is invalid");
            break;
        case 7:
            printf("(7) Acces denied due to prohibited access or directory full ");
            break;
        case 8:
            printf("(8) Acces denied due to prohibited access ");
            break;   
        case 9:
            printf("(9) The file/directory object is invalid ");
            break;   
        case 10:
            printf("(10) The physical drive is write protected ");
            break;        
        case 11:
            printf("(11) The logical drive number is invalid ");
            break;
        case 12:
            printf("(12) The volume has no work area ");
            break;
        case 13:
            printf("(13) There is no valid FAT volume ");
            break;   
        case 14:
            printf("(14) f_mkfs() aborted due to a parameter error ");
            break;   
        case 15:
            printf("(15) Could not access the volume within the defined period ");
            break;                       
    }
}
interrupt [TIM1_COMPA] void timer1_compa_isr(void)
{
disk_timerproc();
/* MMC/SD/SD HC card access low level timing function */
}
  

// Timer2 output compare interrupt service routine
interrupt [TIM2_COMPA] void timer2_compa_isr(void)
{
  mseg++;
}  

void main()
{
    unsigned int  br;
         
    /* FAT function result */
    FRESULT res;
    
    /* will hold the information for logical drive 0: */
    FATFS drive;
    FIL archivo1, archivo2; // file objects
    
    CLKPR=0x80;
    CLKPR=1;    //Micro Trabajará a 8MHz
        
    NombreArchivo[0]='0';
    NombreArchivo[1]=':';
    
    // USART1 initialization
    // Communication Parameters: 8 Data, 1 Stop, No Parity
    // USART1 Receiver: On
    // USART1 Transmitter: On
    // USART1 Mode: Asynchronous
    // USART1 Baud Rate: 9600
    UCSR1A=(0<<RXC1) | (0<<TXC1) | (0<<UDRE1) | (0<<FE1) | (0<<DOR1) | (0<<UPE1) | (0<<U2X1) | (0<<MPCM1);
    UCSR1B=(0<<RXCIE1) | (0<<TXCIE1) | (0<<UDRIE1) | (1<<RXEN1) | (1<<TXEN1) | (0<<UCSZ12) | (0<<RXB81) | (0<<TXB81);
    UCSR1C=(0<<UMSEL11) | (0<<UMSEL10) | (0<<UPM11) | (0<<UPM10) | (0<<USBS1) | (1<<UCSZ11) | (1<<UCSZ10) | (0<<UCPOL1);
    UBRR1H=0x00;
    UBRR1L=0x33;
                 
    // Timer/Counter 2 initialization
    // Clock source: System Clock
    // Clock value: 250.000 kHz
    // Mode: CTC top=OCR2A
    // OC2A output: Disconnected
    // OC2B output: Disconnected
    TCCR2A=0x02;
    TCCR2B=0x03;       //Interrupción cada 1mseg para conteo de tiempo
    OCR2A=249;     
    TIMSK2=0x02;
    
    // Código para hacer una interrupción periódica cada 10ms que pide SD
    // Timer/Counter 1 initialization
    // Clock source: System Clock
    // Clock value: 1000.000 kHz
    // Mode: CTC top=OCR1A
    // Compare A Match Interrupt: On
    TCCR1B=0x0A;
    OCR1AH=0x27;
    OCR1AL=0x0F;
    TIMSK1=0x02;
    SetupLCD();
    StringLCD("Proyecto AES");
    #asm("sei")
    /* Inicia el puerto SPI para la SD */
    disk_initialize(0);
    delay_ms(200);
    
    /* mount logical drive 0: */
    if ((res=f_mount(0,&drive))==FR_OK){
       
      while(1){    
        while((UCSR1A&0x80)!=0){
                                //Limpiar buffer serial (útil para terminal real) 
           opcion=getchar(); 
        };
        printf("MENU CIFRADO AES   1)CIFRAR  2)DESCIFRAR\n\r");
        opcion=getchar();
        if (opcion=='1')
        {   
           br=0;    
           do{              //Lectura de llave
            printf("\n\rDa el nombre del archivo de la llave: ");
            scanf("%s",Cadena);
            i=2;
            for (i=2;i<17;i++)
              NombreArchivo[i]=Cadena[i-2];      //Copiar string con el que antecede con "0:"       
            res = f_open(&archivo1, NombreArchivo, FA_READ);
            if (res==FR_OK){  
               f_read(&archivo1, Llave, 16,&br);             //Leer 16 bytes de la llave
               }   
            else             
            {
                printf("Error en el archivo de la llave\n\r");
                ImprimeError(res);
            }       
           }while(br!=16);    
           f_close(&archivo1);
           //Hacer aquí la expansión de llave 
           //////////////////// 
           KeyExpansion(Llave);
           ///////////////////
        
           error=1;    
           do{              //Lectura de archivo y cifrado
            printf("Da el nombre del archivo a cifrar: ");
            scanf("%s",Cadena);
            i=2;
            for (i=2;i<17;i++)
              NombreArchivo[i]=Cadena[i-2];             
            res = f_open(&archivo1, NombreArchivo, FA_READ);
            if (res==FR_OK){      
               printf("Archivo encontrado\n\r");
               i=0;
               while(NombreArchivo[++i]!='.');  //Busca el punto
               NombreArchivo[i+1]='A'; 
               NombreArchivo[i+2]='E';
               NombreArchivo[i+3]='S';
               res = f_open(&archivo2, NombreArchivo, FA_READ | FA_WRITE | FA_CREATE_ALWAYS );
               if (res==FR_OK)
               {      
                    tiempo=mseg;
                    printf("Cifrando..."); 
                    i=archivo1.fsize%16;        //Calcula el número de bytes del último bloque 
                    NumBloques=archivo1.fsize/16;
                    if (i!=0)
                         NumBloques++;
                    Buffer[0]=i;
                    f_write(&archivo2, Buffer, 1,&br); //Escribe el número de bytes del último bloque
                    j=0;      
                    count = 1;
                    ImprimeProgreso(j); //estado inicial
                    while(j!=NumBloques)
                    {
                        f_read(&archivo1, Buffer, 16,&br);   //LeeBloque
                        //aquí se cifraría usando la llave expandida y bloque de entrada
                        ///////////////////////////// 
                        Cipher(Buffer,Buffer2,Llave);
                        ////////////////////////////
                        f_write(&archivo2,Buffer2, 16,&br);   //LeeBloque    
                        if(((j%200) == 0)&&((NumBloques - j)>200))
                        {
                            ImprimeProgreso(count*200); //progreso cada 200 bloques
                            count++;
                        }
                        j++;
                    }
                    ImprimeProgreso(j);//estado final
                    f_close(&archivo2);
                    tiempo=mseg-tiempo;
                    printf("Archivo crifrado en %lu mseg (%lu bloques)\n\r\n\r",tiempo,NumBloques);     
                    
                    error=0; 
               }
               else
               {                                           
                 printf("Error al generar archivo de salida %i");
                 ImprimeError(res);              
                 error=1;
               }
            }      
            else         
               printf("Archivo no encontrado\n\r");   
           }while(error==1);    
           f_close(&archivo1);    
              
        }
        
        if (opcion=='2')
        {   
           br=0;    
           do{              //Lectura de llave
            printf("\n\rDa el nombre del archivo de la llave: ");
            scanf("%s",Cadena);
            i=2;
            for (i=2;i<17;i++)
              NombreArchivo[i]=Cadena[i-2];             
            res = f_open(&archivo1, NombreArchivo, FA_READ);
            if (res==FR_OK){  
               f_read(&archivo1, Llave, 16,&br);   
               }   
            else
                printf("Error en el archivo de la llave %i\n\r");
                ImprimeError(res);   
           }while(br!=16);    
           f_close(&archivo1);
           //Hacer aquí la expansión de llave
           error=1;    
           do{              //Lectura de archivo y cifrado
            printf("Da el nombre del archivo a descifrar: ");
            scanf("%s",Cadena);
            i=2;
            for (i=2;i<17;i++)
              NombreArchivo[i]=Cadena[i-2];             
            res = f_open(&archivo1, NombreArchivo, FA_READ);
            if (res==FR_OK){        
               printf("Archivo encontrado\n\r");
               printf("Da el nombre del archivo de salida: "); 
               scanf("%s",Cadena);
               i=2;
               for (i=2;i<17;i++)
                 NombreArchivo[i]=Cadena[i-2];
               res = f_open(&archivo2, NombreArchivo, FA_READ | FA_WRITE | FA_CREATE_ALWAYS );
               if (res==FR_OK)
               {      
                    tiempo=mseg;
                    printf("Descifrando...");   
                    f_read(&archivo1, Buffer, 1,&br);   
                    i=Buffer[0];                        //No de bytes del último bloque
                    if (i==0)
                      i=16;
                    NumBloques=archivo1.fsize/16;
                    j=0;    
                    count = 1;
                    ImprimeProgreso(j); //estado inicial
                    if (NumBloques>1)
                    {
                        while(j!=(NumBloques-1))
                        {
                            f_read(&archivo1, Buffer, 16,&br);   //LeeBloque
                            //aquí se descifraría usando la llave expandida y bloque de entrada   
                            //////////////////////////// 
                            DeCipher(Buffer,Buffer2,Llave);
                            ////////////////////////////
                            f_write(&archivo2,Buffer2, 16,&br);   //LeeBloque   
                            if((j%200) == 0 && ((NumBloques - j)>200))
                            {
                                ImprimeProgreso(count*200); //progreso cada 200 bloques
                                count++;
                            } 
                            j++;
                        }
                    }    
                     f_read(&archivo1, Buffer, 16,&br);   //LeeBloque
                      //aquí se descifraría el último bloque 
                      //////////////////////////////////////
                      DeCipher(Buffer,Buffer2,Llave);
                      ///////////////////////////////////
                     f_write(&archivo2,Buffer2, i,&br);   //LeeBloque                  
                    ImprimeProgreso(j+1);//estado final
                    f_close(&archivo2);
                    tiempo=mseg-tiempo;
                    printf("Archivo descrifrado en %lu mseg (%lu bloques)\n\r\n\r",tiempo,NumBloques); 
                    error=0; 
               }
               else
               {                                           
                 printf("Error al generar archivo de salida %i");
                 ImprimeError(res);              
                 error=1;
               }
            }      
            else         
               printf("Archivo no encontrado\n\r");   
           }while(error==1);    
           f_close(&archivo1);    
              
        }
            
        
    }  
    }
    else{
         StringLCD("Drive NO Detectado");
         while(1);
    }
    f_mount(0,0); //Cerrar drive de SD
    while(1);
}

