#include <functions.h>
#include <languages.h>
#include "gameLoop.h"

void gameLoop() 
{
    /*clearScreen();

	WA[31].head = WRLD_ON|WRLD_OVR;
	WA[31].w = 384;
	WA[31].h = 224;
	WA[30].head = WRLD_END;

	printString(0, 14, 10, getString(STR_HELLO_WORLD));*/

	SET_BRIGHT(32, 64, 32);//vbFXFadeIn(0);
	
	VIP_REGS[BKCOL] = 0x03;

	while(1) {}
}