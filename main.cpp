#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#include "io.h"
#endif
#include <avr/eeprom.h>

volatile signed char TimerFlag = 0;

unsigned long _avr_timer_M = 0;
unsigned long _avr_timer_cntcurr = 0;
enum Game_States{init1, DisplayMenu, reswait, DisplayPlay, start, DisplayStat, waiter3, DisplayPScore, waiter1, DisplayHScore, waiter2}; //Text related items
enum Player1{waitP12, P1_I, PlayingP1}; //Character1 Models, potentiall melee and response
enum Player2{waitP22, P2_I, PlayingP2};
enum Trump{waitW, walls};
enum Rendering{waitR, displayee}; //Rendering all models as well as Game Logic

unsigned char Character1[8] = { 0x04, 0x1E, 0x08, 0x0C, 0x1F, 0x08, 0x1E, 0x12 };
unsigned char Character2[8] = { 0x04, 0x1E, 0x08, 0x1F, 0x0C, 0x08, 0x1E, 0x12 };
unsigned char wall[8] = { 0x0B, 0x05, 0x0A, 0x1D, 0x1D, 0x0A, 0x05, 0x0B };
unsigned char flag[8] = { 0x1F, 0x1E, 0x18, 0x10, 0x1F, 0x1E, 0x18, 0x10 };
unsigned char melee11[8] = { 0x01, 0x02, 0x04, 0x1B, 0x1B, 0x04, 0x02, 0x01 };
unsigned char explode[8] = { 0x11, 0x11, 0x0A, 0x15, 0x15, 0x0A, 0x11, 0x11 };

#define A0 (~PINA & 0x01)
#define P1A (~PINB & 0x01)
#define P1B (~PINB & 0x02)
#define P1X (~PINB & 0x04)
#define P2A (~PINB & 0x08)
#define P2B (~PINB & 0x10)
#define P2X (~PINB & 0x20)

char EEMEM EEVar;
char totalScore = 0;
char totalHScore = 0;

void WriteE(char x){
	eeprom_write_block((const void*)&x, (void*)&EEVar, sizeof(char));
}

char ReadE(void){
	char temp;
	eeprom_read_block((void*)&temp, (const void*)&EEVar, sizeof(char));
	return temp;
}

typedef struct task{
        signed char state;
        unsigned long int period;
        unsigned long int elapsedTime;
        int (*TickFct)(int);
} task;

void TimerOn(){
        TCCR1B = 0x0B;
        OCR1A = 125;
        TIMSK1 = 0x02;
        TCNT1 = 0;
        _avr_timer_cntcurr = _avr_timer_M;
        SREG |= 0x80;
}

void TimerOff(){
        TCCR1B = 0x00;
}

void TimerISR(){
        TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect){
        _avr_timer_cntcurr--;
        if(_avr_timer_cntcurr == 0){
                TimerISR();
                _avr_timer_cntcurr = _avr_timer_M;
        }
}

void TimerSet(unsigned long M){
        _avr_timer_M = M;
        _avr_timer_cntcurr = _avr_timer_M;
}

unsigned char powah = 0;
volatile unsigned char deaded = 0;
unsigned char score_Ones = 0;
unsigned char score_Tens = 0;
unsigned char hold0 = 10;
unsigned char hold1 = 10;
unsigned char hold2 = 10;
unsigned char levels_Done = 0;

int GameTick(int state){
    switch(state){
        case init1: state = DisplayMenu; break;

	case DisplayMenu:
			state = reswait;
			break;

        case reswait:
			if(A0) state = DisplayPlay;
			else state = reswait;
			break;
				
        case DisplayPlay:
			state = start;
			break;
			
        case start:
            if(deaded) state = DisplayStat;
            else state = start;
            break;

	case DisplayStat:
	    state = waiter3; break;

	case waiter3:
	    if(hold2 <= 0) state = DisplayPScore;
	    else if(A0) state = DisplayMenu;
	    else state = waiter3;
	    break;

        case DisplayPScore:
            state = waiter1; break;

        case waiter1:
            if(hold0 <= 0) state = DisplayHScore;
            else if(A0) state = DisplayMenu;
            else state = waiter1;
            break;
  
	case DisplayHScore:
            state = waiter2; break;

        case waiter2:
            if(hold1 <= 0 || A0) state = DisplayMenu;
            else state = waiter2;
            break;

        default:
            state = DisplayMenu; break;
        }


    switch(state){
	case init1: break;

        case DisplayMenu:
			LCD_ClearScreen();
            LCD_DisplayString(1, "Welcome to TMStGPress A0 to GoGo");
			break;

        case reswait:
            hold0 = 10;
            hold1 = 10;
	    hold2 = 10;
            powah = 0;
            deaded = 0;
            score_Ones = 0;
	    score_Tens = 0;
	    levels_Done = 0;
            break;
				
        case DisplayPlay:
	    powah = 1;
	    break;				
				
        case start:
	    break;

	case DisplayStat:
	    LCD_ClearScreen();
	    if(levels_Done < 3) LCD_DisplayString(1, "Bad Synergy");
	    else LCD_DisplayString(1,"Good Synergy");
	    break;

	case waiter3:
	    hold2--;
	    break;

        case DisplayPScore:
	    LCD_ClearScreen();
            LCD_DisplayString(1, "Your Score: ");
	    LCD_WriteData(score_Tens + '0');
	    LCD_WriteData(score_Ones + '0');	
            break;

        case waiter1:
            hold0--;
            break;
        
	case DisplayHScore:
            LCD_ClearScreen();
            LCD_DisplayString(1, "High Score: ");
	    totalScore = (score_Tens*10)+ score_Ones;
	    totalHScore = ReadE();
	    if(totalScore > totalHScore){
	    	WriteE(totalScore);
		LCD_WriteData(score_Tens + '0');
		LCD_WriteData(score_Ones + '0');
	    }
	    else{ 
	    	score_Ones = totalHScore % 10;
		score_Tens = 0;
		if(totalHScore > 9){
			while(totalHScore > 0){
				totalHScore-=10;
				score_Tens++;
			}
			if(score_Ones != 0) score_Tens--;
		}
	    	LCD_WriteData(score_Tens + '0');
	    	LCD_WriteData(score_Ones + '0');
	    }
	    break;

        case waiter2:
            hold1--;
            break;

        }

        return state;

}
unsigned char P1_Pos = 1;
unsigned char P2_Pos = 17;
unsigned char melee1 = 0;
unsigned char melee2 = 0;
unsigned char melee1Uses = 3;
unsigned char melee2Uses = 3;
unsigned char completed = 0;

int Player1Ticks(int state){
	switch(state){
		case waitP12:
			if(powah) state = P1_I;
			else state = waitP12;
			break;
			
		case P1_I:
			if(!deaded)state = PlayingP1;
			else state = waitP12;
			break;
			
		case PlayingP1:
			if(completed) state = P1_I;
			else if(deaded ) state = waitP12;
			else state = PlayingP1;
			break;
			
		default: 
			state = waitP12;
			break;
	}
	
	switch(state){
		case waitP12:
			melee1Uses = 3;
			break;
		
		case P1_I:
			P1_Pos = 1;
			melee1Uses = 3;
			break;
		case PlayingP1:
			if(melee1) melee1 = 0;
			if(P1A){
				if(P1_Pos == 14);
				else P1_Pos++;
			}	
	
			if(P1B){
				if((melee1Uses > 0) && P1_Pos < 13){
					 melee1 = 1; melee1Uses--;
				}
			}	

			if(P1X){
				if(melee1Uses > 0 && melee2Uses < 3){
				 	melee1Uses--; melee2Uses ++;
				}
			}	
			break;
	}

	return state;
}

int Player2Ticks(int state){
	switch(state){
		case waitP22:
			if(powah) state = P2_I;
			else state = waitP22;
			break;

	case P2_I:
		if(!deaded)state = PlayingP2;
		else state = waitP22;
		break;


	case PlayingP2:
		if(completed) state = P2_I;
		else if(deaded) state = waitP22;
		else state = PlayingP2;
		break;

	default:
		state = waitP22;
		break;
        }

	switch(state){
	case waitP22:
		melee2Uses = 3;
		break;

	case P2_I:
		P2_Pos = 17;
		melee2Uses = 3;
		break;

	case PlayingP2:
		if(melee2) melee2 = 0;
		if(P2A){
			if(P2_Pos == 30);
			else P2_Pos++;
		}

		if(P2B){
			if(melee2Uses > 0 && P2_Pos < 30){ melee2 = 1; melee2Uses--;}
		}

		if(P2X){
			if(melee2Uses > 0 && melee1Uses < 3){ melee2Uses --; melee1Uses ++;}
		}
		break;
	}

	return state;
}

unsigned char walls_Pos[15] = {11, 12, 14, 15, 29, 13, 15, 22, 23 , 0, 9, 10, 12, 22, 23}; //test1
unsigned char wall_Clone[5];
unsigned int j = 0;
unsigned int top = 0;
unsigned char broken1 = 0;
unsigned char broken2 = 0;
//unsigned char wally1 = 12;
//unsigned char wally2 = 25;

int WallTick(int state){
	switch(state){
		case waitW:
			if(powah) state = walls;
			else state = waitW;
			break;
			
		case walls:
			if(deaded) state = waitW;
			else state = walls;
			break;
	}
	
	switch(state){
		case waitW:
			top = 0;
			for(j = 0; j < 5; j++){
				wall_Clone[j] = walls_Pos[j];
			}
			break;
			
		case walls:
			
			if(completed){
			  if(top < 2){
				top++;
				for(j = 0; j < 5; j++){
					wall_Clone[j] = walls_Pos[5*top+j];
				}
			  }
			  else if(top >= 2){ 
				top = 0;
				for(j = 0; j < 5; j++){wall_Clone[j] = walls_Pos[j];}

			  }
			}
			for(j = 0; j < 5; j++){
				if(wall_Clone[j] != 0) wall_Clone[j]--;
				if(wall_Clone[j] == 16) wall_Clone[j] = 0;

				if(melee1 && (P1_Pos+1 == wall_Clone[j])) {broken1 = 1; score_Ones++; wall_Clone[j] = 0;}
				if(melee2 && (P2_Pos+1 == wall_Clone[j])) {broken2 = 1; score_Ones++; wall_Clone[j] = 0;}
			}



			break;
	}
	return state;
}

unsigned char LEDs1 = 0x00;
unsigned char LEDs2 = 0x00;
int RenderingTick(int state){
	switch(state){
		case waitR:
			if(powah) state = displayee;
			else state = waitR;
			break;
		
		case displayee:
			if(deaded) state = waitR;
			else state = displayee;
			break;
		
		default: state = waitR; break;
	}
	
	switch(state){
		case waitR:
			break;

		case displayee:
			if(completed) completed = 0;
			powah = 0;

			if(melee1Uses == 0) LEDs1 = 0x00;
			else if(melee1Uses == 1) LEDs1 = 0x01;
			else if(melee1Uses == 2) LEDs1 = 0x03;
			else LEDs1 = 0x07;

			if(melee2Uses == 0) LEDs2 = 0x00;
                        else if(melee2Uses == 1) LEDs2 = 0x08;
                        else if(melee2Uses == 2) LEDs2 = 0x18;
                        else LEDs2 = 0x38;

			PORTD = LEDs1 | LEDs2;
                        if(score_Ones >= 10){ score_Tens++; score_Ones -= 10;}			
			if(P1_Pos == 14 && P2_Pos == 30){
				completed = 1;
				score_Ones += 2;
				levels_Done++;
				score_Ones += (melee1Uses + melee2Uses);
			}

			LCD_ClearScreen();
		   	
			unsigned char place = 0;
                        for(j = 0; j < 4; j++){
			    place = wall_Clone[j];
                            if(place == P1_Pos || place == P2_Pos){
				 deaded = 1; break;}
                            if(place != 0){ 
			    	LCD_Cursor(place); LCD_WriteData(3);
			    }
                        }	

			LCD_Cursor(P1_Pos);
			LCD_WriteData(0);
			LCD_Cursor(P2_Pos);
			LCD_WriteData(1);
			if(melee1){
				LCD_Cursor(P1_Pos + 1);
				if(!broken1) LCD_WriteData(2);
				else LCD_WriteData(4);
			}
			
			if(melee2){
				LCD_Cursor(P2_Pos + 1);
				if(!broken2) LCD_WriteData(2);
				else LCD_WriteData(4);
			}
			broken1 = 0;
			broken2 = 0;
			LCD_Cursor(14);
			LCD_WriteData(5);
			LCD_Cursor(30);
			LCD_WriteData(5);
			LCD_Cursor(5);
			LCD_Cursor(15);
			LCD_WriteData(score_Tens + '0');
			LCD_Cursor(16);
			LCD_WriteData(score_Ones + '0');
			LCD_Cursor(32);
			 break;
	}
	
	return state;
}

int main(){
		DDRA = 0x00; PORTA = 0xFF;
	        DDRB = 0x00; PORTB = 0xFF;
		DDRC = 0xFF; PORTC = 0x00;
		DDRD = 0xFF; PORTD = 0x00;

		static task task1, task2, task3, task4, task5;
		task *tasks[] = {&task1, &task2, &task3, &task4, &task5};
		const unsigned short numTasks = 5; //sizeof(tasks)/sizeof(task*);
		
		task1.state = init1;
		task1.period = 500;
		task1.elapsedTime = task1.period;
		task1.TickFct = &GameTick;
		
                task2.state = waitP12;
                task2.period = 300;
                task2.elapsedTime = task2.period;
                task2.TickFct = &Player1Ticks;

                task3.state = waitP22;
                task3.period = 300;
                task3.elapsedTime = task3.period;
                task3.TickFct = &Player2Ticks;

                task4.state = waitW;
                task4.period = 300;
                task4.elapsedTime = task4.period;
                task4.TickFct = &WallTick;

                task5.state = waitR;
                task5.period = 300;
                task5.elapsedTime = task5.period;
                task5.TickFct = &RenderingTick;

		TimerSet(50);
		TimerOn();

		unsigned short i;

		LCD_init();
		LCD_Cust_Char(0, Character1);
		LCD_Cust_Char(1, Character2);
		LCD_Cust_Char(2, melee11);
		LCD_Cust_Char(3, wall);
		LCD_Cust_Char(4, explode);
		LCD_Cust_Char(5, flag);
	while(1){

		for(i = 0; i < numTasks; i++){
			if(tasks[i]->elapsedTime >= tasks[1]->period){
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 50;
		}
	
		while(!TimerFlag);
		TimerFlag = 0;
        }
		
		return 0;
}
