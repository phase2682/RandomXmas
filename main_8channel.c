/******************************************************************************
*                                                                             *
*  File:                          main_8channel.c                   		  *
*  Project:  Eight Channel controller for pole xmas tree	                  *
*                                                                             *
*  Description: This file contains C code for a PIC16F690 using HiTech PICC   *
*               lite compiler version 9.83.
*				
*				Eight channels to control lights is on Port C with either active
				high or active low selectable.  There are several different 
				modes that determine which channels are enabled/blanked.  Each
				mode selects a pattern that can be the binary representation 
				or the channel selected to be enabled or blanked.

				The pattern (mode), speed and selected channel are each random
				numbers within the appropriate range.  This allows for fully 
				automatic and random operation.
*								
				USART:  RX-RB5 and TX-RB7 left free for outputting various 
				values for monitoring and debugging.  

*
*******************************************************************************
*                                                                             *
*  Created By:  Dan Koellen   November 2013                                   *
*                                                                             *
*  Versions:                                                                  *
*                                                                             *
*  MAJ.MIN - AUTHOR - CHANGE DESCRIPTION                                      *
*                                                                             *
*   1.0    -  remove serial interface used for debug
*
*   2.0    -   use __delay_ms in pause function                               *
******************************************************************************/

#include <pic.h>
#include <stdio.h>
#include <stdlib.h>

#define _XTAL_FREQ 4000000

char portclogic = 1;				// define portclogic = 1 for positive (high turns on lights)on port C
//char portclogic = 0;				// define portclogic = 0 for negative (low turns on lights)on port C

unsigned int seed;					//seed value for srand() for random number generation
int n;

char RandomNumber(unsigned char range);									//Random Number Generator function declaration
char PortCDriver(char selected, char nchan, char onchan, char logic);	//Port C driver function declaration
void pause(char dd);													//pause function declaration


__CONFIG(FOSC_INTRCIO & WDTE_OFF & PWRTE_ON & MCLRE_OFF & CP_OFF & BOREN_OFF & IESO_OFF & FCMEN_OFF);

__EEPROM_DATA(0,0,0,0,0,0,0,0);						//sets first eight eeprom addresses when programming the ucontroller


main()
{
PORTA = 0;			//Clear PortA 
TRISA = 0b00000000;
TRISB4=0;TRISB6=0;TRISB5=0;TRISB7=0;	// Port I/O inputs, A0,1,2 3,4,5 outputs, RB4, RB5(RX), RB6, RB7(TX) outputs
CM1CON0 = 0;			//C1 Comparator off
ANSEL = 0b00000000;		// no analog
TRISC = 0; 				//All PortC is output



/* assigns value to variable seed at each power up.  The first power up
assigns seed = 0x00 read from initial values at eeprom locations 0 and 1
as defined by __eeprom_data configuration statement
0x01 is added to the seed value and stored in the eeprom so that the seed 
will be greater by 1 for each power up.
seed resets to 0x0000 after seed = 0xfffe
*/
unsigned int seedl = eeprom_read(0);					//seed low bits read from eeprom
unsigned int seedh = eeprom_read(1);					//seed high bits read from eeprom
seed = seedl + ((seedh<<8) & 0xff00);					//seed is constructed from the high bits and low bits
unsigned int newseed = seed +1;							//seed for next power up will be one greater
if (newseed >= 0xffff){newseed = 0;}					//reset after 0xfffe
char newseedl = newseed & 0x00ff;						//low bits for newseed
char newseedh = (newseed & 0xff00)>>8;					//high bits for newseed
eeprom_write (0,newseedl); eeprom_write(1,newseedh);	//write into appropriate eeprom locations

int i;	

char oldmode=0;											//prior mode value
char oldall=0;											//prior binary value
char oldeight=0;										//prior random modulus value
char reps;												//number of reps for each selected pattern


srand(seed);											//initialize the random number generator

while(1)
	{

// Set the duration to be used with pause() function
// This will set the speed that the display changes
		char dd = RandomNumber(2);
	

// Set the mode (pattern)to be used
		char mode=RandomNumber(15);
			if (mode==oldmode){							//test if mode value is repeating
			mode=RandomNumber(15);
			}		//end if
		oldmode=mode;									//store mode value for comparison on next cycle 

// Set the number of reps for each pattern
reps = 32;

	

//Displays binary representation of generated number
	if (mode==0){								
		for(i=1;i<=reps;i++){   
		int n = rand();
		char all = n % 256;
			if (all==oldall){									//test if value is repeating
			n=rand();all=n%256;									//generate new value if a repeat
			}		//end if
		
		oldall=all;												//save value for comparison in next cycle
		if (portclogic == 0){all=all^0xFF;}						//toggle for negative logic
		PORTC=all;												//load into portC
		pause(dd);												//pause 250mSec for dd=0 500mSec for dd=1
		}		// end for
	}			// end if

	// Enables only channel corresponding to generated number
	if (mode==1){							
		for (i=1;i<=reps;i++)
		{   
		char eight = RandomNumber(8);
			if(eight==oldeight){									//test if value is repeating
			eight = RandomNumber(8);								//generate new value if a repeat
			}		//end if
		oldeight=eight;
		PortCDriver(eight,1,1,portclogic);							 
		pause(dd);
		}		// end for
	}			// end if

	//Enables all channels except for generated number	
	if (mode==2){								
		for (i=1;i<=reps;i++)
		{   
		char eight = RandomNumber(8);
			if(eight==oldeight){
				eight = RandomNumber(8);
			}		//end if
		oldeight=eight;
		PortCDriver(eight,1,0,portclogic);							
		pause(dd);

		}		// end for
	}			// end if

// Enables only channel corresponding to generated number plus one spaced channel
// For two enabled channels the spacing is four, for three enabled channels the spacing is three
	if (mode==3){							
		for (i=1;i<=reps;i++)
		{   
		char eight = RandomNumber(8);
			if(eight%4==oldeight%4){								//test that new value is not equal to or separated by four from prior value
				eight = RandomNumber(8);							//generate new value
			}		//end if
		oldeight=eight;
		PortCDriver(eight,2,1,portclogic);							//
		pause(dd);
		}		// end for
	}			// end if		

// Blanks only channel corresponding to generated number plus one spaced channel
// For two enabled channels the spacing is four, for three enabled channels the spacing is three
	if (mode==4){							
		for (i=1;i<=reps;i++)
		{   
		char eight = RandomNumber(8);
			if(eight%4==oldeight%4){
				eight = RandomNumber(8);
			}		//end 
		oldeight=eight;
		PortCDriver(eight,2,0,portclogic);
		pause(dd);
		}		// end for
	}			// end if

// Enables only channel corresponding to generated number plus two spaced channels
// For two enabled channels the spacing is four, for three enabled channels the spacing is three 
	if (mode==5){							
		for (i=1;i<=reps;i++)
		{   
		char eight = RandomNumber(8);
			if(eight==oldeight){
				eight = RandomNumber(8);
			}		//end if	
		oldeight=eight;
		PortCDriver(eight,3,1,portclogic);
		pause(dd);
		}		// end for
	}			// end if
	

// Blanks only channel corresponding to generated number plus two spaced channels
// For two enabled channels the spacing is four, for three enabled channels the spacing is three  
	if (mode==6){							
		for (i=1;i<=reps;i++)
		{   
		char eight = RandomNumber(8);
			if(eight==oldeight){
				eight = RandomNumber(8);
			}		//end if	
		oldeight=eight;
		PortCDriver(eight,3,0,portclogic);
		pause(dd);
		}		// end for
	}			// end if
	

// Enables only one channel increasing rotation
	if (mode==7){							
		for (i=1;i<=reps;i++)
		{   
		char eight = i%8;
		PortCDriver(eight,1,1,portclogic);							
		pause(dd);
		}		// end for
	}			// end if


// Blanks only one channel increasing rotation
	if (mode==8){							
		for (i=1;i<=reps;i++)
		{   
		char eight = i%8;
		PortCDriver(eight,1,0,portclogic);							
		pause(dd);
		}		// end for
	}			// end if


// Blanks only one channel decreasing rotation
	if (mode==9){							
		for (i=reps;i>=1;i--)
		{   
		char eight = i%8;
		PortCDriver(eight,1,0,portclogic);							
		pause(dd);
		}		// end for
	}			// end if

// Enables only one channel decreasing rotation
	if (mode==10){							
		for (i=reps;i>=1;i--)
		{   
		char eight = i%8;
		PortCDriver(eight,1,1,portclogic);							
		pause(dd);
		}		// end for
	}			// end if

// Enables only one channel and spaced channel increasing rotation
	if (mode==11){							
		for (i=1;i<=reps;i++)
		{   
		char eight = i%8;
		PortCDriver(eight,2,1,portclogic);							
		pause(dd);
		}		// end for
	}			// end if


// Blanks only one channel and spaced channel increasing rotation
	if (mode==12){							
		for (i=1;i<=reps;i++)
		{   
		char eight = i%8;
		PortCDriver(eight,2,0,portclogic);							
		pause(dd);
		}		// end for
	}			// end if


// Blanks only one channel and spaced channel decreasing rotation
	if (mode==13){							
		for (i=reps;i>=1;i--)
		{   
		char eight = i%8;
		PortCDriver(eight,2,0,portclogic);							
		pause(dd);
		}		// end for
	}			// end if

// Enables only one channel and spaced channel decreasing rotation
	if (mode==14){							
		for (i=reps;i>=1;i--)
		{   
		char eight = i%8;
		PortCDriver(eight,2,1,portclogic);							
		pause(dd);
		}		// end for
	}			// end if



	}  		//end while
}	//End Main



	/**********************************************************************************************
		Function Name: 	char RandomNumber(unsigned char range)
		Return Value:	random number
		Parameters:		desired range of random numbers e.g. for 0 - 7 range is 8
		Description:	Generates random number, srand() must be run prior
	
	************************************************************************************************/ 
	
	char RandomNumber(unsigned char range)
	{
		n = rand();
		char rnmbr = n % range;
		return rnmbr;
	}										//end function


	/**********************************************************************************************
		Function Name: 	char PortCDriver(char selected, char nchan, bit onchan, bit logic)
		Return Value:	PORTC
		Parameters:		selected is the channel selected (0 - 7)
						nchan is # of channels lit including selected
						onchan is 0 for select channels off or 1 for selected channels on 
						logic is 0 for negative logic or 1 for positive logic
		Description:	Generates values for eight channels on PORTC 
	
	************************************************************************************************/ 
	char space;
	char PortCDriver(char selected, char nchan, char onchan, char logic)
	{
		space = 0;
		if (nchan == 2){space = 4;}
		if (nchan == 3){space = 3;}
		char port=0xFF&0x01<<selected | 0xFF&0x01<<((selected+space)%8) | 0xFF&0x01<<((selected+space*2)%8);
		char flip=onchan^logic;			//determine if outputs need to be toggled
		if (flip){port=port^0xFF;}
		PORTC=port;
		return port;
	} 									// end function

/**********************************************************************************************
		Function Name: 	void pause(int dd)
		Return Value:	
		Parameters:		dd is 0 for 250mSec, 1 for 500mSec
		Description:	pauses for 250mSec or 500mSec
	
	************************************************************************************************/
	void pause(char dd)
	{
	__delay_ms (250);					//250mSec delay
	if (dd) __delay_ms (250);			//for dd = 1 display duration is an additional 250mSec
	}									//end function

