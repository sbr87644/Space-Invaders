// SpaceInvaders.c
// Runs on TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the ECE319K Lab 10
// Written by Sathvik Reddy and Lakshya Domyan 

// Last Modified: 1/2/2023 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php

// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// buttons connected to PE0-PE3
// 32*R resistor DAC bit 0 on PB0 (least significant bit)
// 16*R resistor DAC bit 1 on PB1
// 8*R resistor DAC bit 2 on PB2 
// 4*R resistor DAC bit 3 on PB3
// 2*R resistor DAC bit 4 on PB4
// 1*R resistor DAC bit 5 on PB5 (most significant bit)
// LED on PD1
// LED on PD0


#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/ST7735.h"
#include "Random.h"
#include "TExaS.h"
#include "../inc/ADC.h"
#include "Images.h"
#include "../inc/wave.h"
#include "Timer1.h"
#include "../inc/DAC.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds


uint16_t score = 0; // Instance of global variables
uint16_t lifeCount = 1; 
uint16_t lang = 0;





struct Alien { // Struct for the Aliens
	uint32_t x;
	uint32_t y;
	uint8_t life;
	uint16_t points;
};

typedef struct Alien Alien_t;



Alien_t aliens[] = 
{
    {0, 40, 1, 10}, // Positioning of all aliens
    {20, 40, 1, 20},
    {40, 40, 1, 30},
    {60, 40, 1, 30},
    {80, 40, 1, 20},
    {100, 40, 1, 10}
};

struct Ship { // Struct for ship
	uint32_t x;
	uint32_t y;
	uint8_t life;
};

typedef struct Ship Ship_t;


Ship_t Ship = {0, 140, 1}; // Positioning of ship 

uint16_t end = 0;



void EndScreen(uint16_t score) {
	ST7735_FillScreen(0x0000);   
	
	if(end == 1){ // Texts to be displayed depending on language selected 
		
		if(!lang) {
				ST7735_SetCursor(2, 4);
				ST7735_OutString("Game Over!");
				
				ST7735_SetCursor(2, 7);
				ST7735_OutString("Score: ");
				
				ST7735_SetCursor(10, 7);
				ST7735_OutUDec(score);
			} else {
				ST7735_SetCursor(2, 4);
				ST7735_OutString("Juego terminado!");
				
				ST7735_SetCursor(2, 7);
				ST7735_OutString("Puntaje: ");
				
				ST7735_SetCursor(10, 7);
				ST7735_OutUDec(score);
			}
	} else{
		if(!lang) {
				ST7735_SetCursor(2, 4);
				ST7735_OutString("You won!");
				
				ST7735_SetCursor(2, 7);
				ST7735_OutString("Score: ");
				
				ST7735_SetCursor(10, 7);
				ST7735_OutUDec(score);
			} else {
				ST7735_SetCursor(2, 4);
				ST7735_OutString("Ganaste!");
				
				ST7735_SetCursor(2, 7);
				ST7735_OutString("Puntaje: ");
				
				ST7735_SetCursor(10, 7);
				ST7735_OutUDec(score);
			}
	}
}



uint8_t pauseFlag = 0; // Instance of pause flag and movement of alien 
uint8_t alienMove = 1;

void DrawAliens() {
	for(int i = 0; i < 6; i ++) {
		ST7735_DrawBitmap(aliens[i].x, aliens[i].y-alienMove, BlackCover, 16,10);
		if(aliens[i].life) {
			if(i == 5 || i == 0){
				ST7735_DrawBitmap(aliens[i].x, aliens[i].y, SmallEnemy10pointA, 16,10);
			
			} else if(i == 4 || i == 1){
				ST7735_DrawBitmap(aliens[i].x, aliens[i].y, SmallEnemy20pointA, 16,10);
			
			} else{
				ST7735_DrawBitmap(aliens[i].x, aliens[i].y, SmallEnemy30pointA, 16,10);
			}
		}
	}
}



void MoveInvaders() {
	uint8_t liveTrack = 0;
	
	if(pauseFlag == 0){ // If the pause is not pressed aliens move down 
    
		for (int i = 0; i < 6; i++) {
        aliens[i].y += alienMove;
			
        if (aliens[i].y > 140) { // If they reach the bottom the player losses and screen transitions
						
						end = 1;
        }
        if (aliens[i].life) {
            liveTrack++;
        }
    }
    if (liveTrack== 0) { // Game ends if no lives are left and screen transitions
        end = 2;
    }
	}
}




uint8_t oldShipX;

void DrawShip() {
	ST7735_DrawBitmap(oldShipX, Ship.y, PlayerCover, 18,8);
	ST7735_DrawBitmap(Ship.x, Ship.y, PlayerShip0, 18,8);
}

void MoveShip() {
	if(pauseFlag == 0){ // Movement of ship which takes output of ADC using slidepot 
	oldShipX = Ship.x;
	
	Ship.x = (ADC_In() * 127)/4096;
	}
}



struct ShipProjectile { // Struct for ship projectile 
	uint32_t oldX;
	uint32_t oldY;
	uint32_t x;
	uint32_t y;
	uint8_t life;
};

typedef struct ShipProjectile ShipProjectile_t;


uint8_t shootButton = 0; // Onstance variables for user projectiles 
uint8_t shotIndex = 0;
uint8_t beenPressed = 0;
ShipProjectile_t playerShots[30];

void drawShipShots() {
	for(int i = 0; i < 30; i++) {
		ST7735_DrawBitmap(playerShots[i].x, playerShots[i].oldY, PlayerBulletCover, 4,4);
		
		if(playerShots[i].life){
			ST7735_DrawBitmap(playerShots[i].x, playerShots[i].y, PlayerBullet, 4,4);
		}
	}
}

void MoveShipShots() {
	if(pauseFlag == 0){
	
		for(int i = 0; i < 30; i++) {
		
			if(playerShots[i].life){
			playerShots[i].oldY = playerShots[i].y;
			playerShots[i].y -= 8; 
		
		for(int j = 0; j < 6; j++) { // Condition checked to see if the alien is hit 
			
			if((playerShots[i].y < aliens[j].y + 10) && ((playerShots[i].x > aliens[j].x) && (playerShots[i].x < aliens[j].x + 16)) && aliens[j].life == 1){
				aliens[j].life = 0;
				playerShots[i].life = 0;
				score += aliens[j].points;
				break;
			}
		}
		
		}
	}
}
}



void reverseHelper(char *str, int l) {
    int start = 0;
    int end = l - 1;
    char temp;

    while (start < end) {
        temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

int integerToString(int num, char *str, int c) {
    int i = 0;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return i;
    }

    while (num != 0) {
        int rem = num % c;
        str[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        num = num / c;
    }

    str[i] = '\0';

    reverseHelper(str, i);

    return i;
}

void Pause() {
	while(!((GPIO_PORTE_DATA_R & 0x02) >> 1)) {
		continue;
	}
	
}
	

void Timer1A_Handler(void){ // can be used to perform tasks in background
  TIMER1_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER1A timeout
	MoveInvaders();
	
	MoveShip();
	
	MoveShipShots();
	
	beenPressed = 0;
}

int main1(void){
  DisableInterrupts();
  TExaS_Init(NONE);       // Bus clock is 80 MHz 
  Random_Init(1);

  Output_Init();
  ST7735_FillScreen(0x0000);            // set screen to black
  
  ST7735_DrawBitmap(22, 159, PlayerShip0, 18,8); 
  ST7735_DrawBitmap(53, 151, Bunker0, 18,5);
  ST7735_DrawBitmap(42, 159, PlayerShip1, 18,8); 
  ST7735_DrawBitmap(62, 159, PlayerShip2, 18,8); 
  ST7735_DrawBitmap(82, 159, PlayerShip3, 18,8); 

  ST7735_DrawBitmap(0, 9, SmallEnemy10pointA, 16,10);
  ST7735_DrawBitmap(20,9, SmallEnemy10pointB, 16,10);
  ST7735_DrawBitmap(40, 9, SmallEnemy20pointA, 16,10);
  ST7735_DrawBitmap(60, 9, SmallEnemy20pointB, 16,10);
  ST7735_DrawBitmap(80, 9, SmallEnemy30pointA, 16,10);
  ST7735_DrawBitmap(100, 9, SmallEnemy30pointB, 16,10);

  Delay100ms(50);              // delay 5 sec at 80 MHz

  ST7735_FillScreen(0x0000);   // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString("GAME OVER");
  ST7735_SetCursor(1, 2);
  ST7735_OutString("Nice try,");
  ST7735_SetCursor(1, 3);
  ST7735_OutString("Earthling!");
  ST7735_SetCursor(2, 4);
  ST7735_OutUDec(1234);
  while(1){
  }

}


// You can't use this timer, it is here for starter code only 
// you must use interrupts to perform delays
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
      time--;
    }
    count--;
  }
}
typedef enum {English, Spanish, Portuguese, French} Language_t;
Language_t myLanguage=English;
typedef enum {HELLO, GOODBYE, LANGUAGE} phrase_t;
const char Hello_English[] ="Hello";
const char Hello_Spanish[] ="\xADHola!";
const char Hello_Portuguese[] = "Ol\xA0";
const char Hello_French[] ="All\x83";
const char Goodbye_English[]="Goodbye";
const char Goodbye_Spanish[]="Adi\xA2s";
const char Goodbye_Portuguese[] = "Tchau";
const char Goodbye_French[] = "Au revoir";
const char Language_English[]="English";
const char Language_Spanish[]="Espa\xA4ol";
const char Language_Portuguese[]="Portugu\x88s";
const char Language_French[]="Fran\x87" "ais";
const char *Phrases[3][4]={
  {Hello_English,Hello_Spanish,Hello_Portuguese,Hello_French},
  {Goodbye_English,Goodbye_Spanish,Goodbye_Portuguese,Goodbye_French},
  {Language_English,Language_Spanish,Language_Portuguese,Language_French}
};

void BeginningScreen() {
	ST7735_SetCursor(2, 4);
  ST7735_OutString("Space ");
	
	ST7735_SetCursor(8, 4);
	ST7735_OutString("Invaders");
	
	ST7735_SetCursor(1, 8);
	ST7735_OutString("Press Yellow for");
	
	ST7735_SetCursor(1, 9);
	ST7735_OutString("English");
	
	ST7735_SetCursor(1, 11);
	ST7735_OutString("Press White for");
	
	ST7735_SetCursor(1, 12);
	ST7735_OutString("Spanish");
}

uint8_t pausePressed= 0;
uint8_t pausePressed2 = 0;


uint8_t spanishException = 0;

int main(void){ char l;
  DisableInterrupts();
  TExaS_Init(NONE);       // Bus clock is 80 MHz 
	Random_Init(1);
  Output_Init();
  Timer1_Init(8000000, 1); // Initialize Timer1 with 1-second period and priority 1
	Wave_Init();
	Timer2A_Start();
	ADC_Init();
  ST7735_FillScreen(0x0000);         // set screen to black
	
	// Initialize Buttons
	SYSCTL_RCGCGPIO_R |= 0x10;
  while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R4) == 0) {}
	GPIO_PORTE_DIR_R &= ~0x03;
	GPIO_PORTE_DEN_R |= 0x03;
	
	for(int i = 0; i < 30; i++) {
		playerShots[i].life = 0;
	}


  BeginningScreen();
	
	
		
	while(1) {
		if((GPIO_PORTE_DATA_R & 0x01)) {
			lang = 0; 
			break;
		}
		if(((GPIO_PORTE_DATA_R & 0x02) >> 1)) {
			lang = 1; 
			spanishException = 1;
			break;
		}
	}
	
	ST7735_FillScreen(0x0000);       // set screen to black
	EnableInterrupts();
	
	
	while(1) {
		if(!end) {
			char lifeStr[10];
			integerToString(lifeCount, lifeStr, 10); // Convert lifeCount to a string

			ST7735_SetCursor(1, 1);
			if(!lang) {
				ST7735_OutString("Lives: ");
			} else {
				ST7735_OutString("Vidas: ");
			}
			ST7735_OutString(lifeStr); // Print the lifeCount string
			
			char scoreStr[10];
			integerToString(score, scoreStr, 10); // Convert lifeCount to a string
			
			ST7735_SetCursor(10, 1);
			if(!lang) {
				ST7735_OutString("Score: ");
			} else {
				ST7735_OutString("Puntaje:");
			}
			ST7735_OutString(scoreStr); // Print the lifeCount string
			
			if(GPIO_PORTE_DATA_R & 0x01 && beenPressed == 0) {
				shootButton = 1;
				beenPressed = 1;
			}
			
			if(shootButton) { // If shoot button pressed initiate sound and movement
				Wave_Shoot();
				playerShots[shotIndex].x = Ship.x + 5;
				playerShots[shotIndex].y = Ship.y - 5;
				playerShots[shotIndex].life = 1;
				shotIndex++;
			}
			
			int8_t pausebutton = ((GPIO_PORTE_DATA_R & 0x02) >> 1);
			
			
			if(pausePressed2 == 1 && !pausebutton){
				pausePressed = 0;
				pausePressed2 = 0;
				pauseFlag = 0;
			}
			if(pausebutton && pauseFlag == 1){
				pausePressed2 = 1;
			}
			if(pausePressed == 1 && !pausebutton){ 
				pauseFlag = 1;
				if(spanishException == 1){
					pauseFlag = 0;
					spanishException = 0;
					pausePressed = 0;
				}
			}
			if(pausebutton) {
				pausePressed = 1;
			}
			
			
			DrawAliens();
		
			DrawShip();
		
			drawShipShots();
			
			shootButton = 0;
			
		} else {
			break;
		}
	}
	EndScreen(score); // Transition to end screen with score displaying 
}



