#include <functions.h>
#include <components.h>

/*void vipInterruptHandler()
{
	VIP_REGS[XPCTRL] = VIP_REGS[XPSTTS] | XPEN;
	// Comment this out; breaks in rustual boy
	//  BUT WORKS IN REALITY BOY I JUST FOUND OUT AFTER THE STREAM!!!
	VIP_REGS[DPCTRL] = VIP_REGS[DPSTTS] | (SYNCE | RE | DISP);
	VIP_REGS[BKCOL] = 0x03;

	vpu_vector = 0;
}*/

const BYTE CHAR_DATA[] =
{
	// Char 0 (empty)
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	
	// Char 1 (solid)
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	
	// Char 2 (bottom line)
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
	
	// Char 3 (top line)
	0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

typedef struct
{
	WORD timestamp;
	HWORD value;
} Entry;

#define MAX_ENTRIES 1000

static Entry Entries[MAX_ENTRIES];

int main()
{
	int gameFrameIndex = 0;
	
	//column table setup
	vbSetColTable();

	//display setup
	VIP_REGS[REST] = 0;
	VIP_REGS[XPCTRL] = VIP_REGS[XPSTTS] | XPEN;
	VIP_REGS[DPCTRL] = VIP_REGS[DPSTTS] | (SYNCE | RE | DISP);
	VIP_REGS[FRMCYC] = 1;
	VIP_REGS[INTCLR] = VIP_REGS[INTPND];
	while (!(VIP_REGS[DPSTTS] & 0x3C)); // Wait for VBLANK (probably)

	VIP_REGS[BRTA]  = 0;
	VIP_REGS[BRTB]  = 0;
	VIP_REGS[BRTC]  = 0;
	VIP_REGS[GPLT0] = 0xE4;	/* Set all eight palettes to: 11100100 */
	VIP_REGS[GPLT1] = 0xE4;	/* (i.e. "Normal" dark to light progression.) */
	VIP_REGS[GPLT2] = 0xE4;
	VIP_REGS[GPLT3] = 0xE4;
	VIP_REGS[JPLT0] = 0xE4;
	VIP_REGS[JPLT1] = 0xE4;
	VIP_REGS[JPLT2] = 0xE4;
	VIP_REGS[JPLT3] = 0xE4;
	VIP_REGS[BKCOL] = 0;	/* Clear the screen to black before rendering */
	
	SET_BRIGHT(32, 64, 32);
	
	// Set up worlds
	WA[31].head = (WRLD_LON | WRLD_RON | WRLD_OVR);
    WA[31].w = 384;
    WA[31].h = 224;
	
	WA[30].head = WRLD_END;
	
	// Set up char data
	copymem((void *)CharSeg0, (void *)CHAR_DATA, 4 * 16);
	
	// Clear bg map data
	{
		int x, y;
		BYTE *mapPtr = BGMap(0);

		for (y = 0; y < 64; y++)
		{
			for (x = 0; x < 64; x++)
			{
				*mapPtr = 0x00;
				mapPtr++;
				*mapPtr = 0x00;
				mapPtr++;
			}
		}
	}
	
	// Set up display
	VIP_REGS[DPCTRL] = VIP_REGS[DPSTTS] | (SYNCE | RE | DISP);
	
	// Set up drawing
	VIP_REGS[XPCTRL] = VIP_REGS[XPSTTS] | XPEN;
	
	// Set VIP interrupt
	//vpu_vector = (u32)(vipInterruptHandler);
	//VIP_REGS[INTENB] |= FRAMESTART;
	
	//set_intlevel(0);
	
	// Reset pending interrupts
	VIP_REGS[INTCLR] = 0xff;
	
	// Main loop
	while (1)
	{
		WORD captureTotalTicks;
		HWORD regValue, prevRegValue;
		Entry *entryPtr;
		
		// Wait for start of game frame and display frame
		do
		{
			regValue = VIP_REGS[INTPND];
		} while (!(regValue & (GAMESTART | FRAMESTART)));
		
		// Reset pending interrupts
		VIP_REGS[INTCLR] = 0xff;
		
		// Start capture
		captureTotalTicks = 0;
		prevRegValue = regValue;
		entryPtr = Entries;
		
		// Append initial value
		entryPtr->timestamp = captureTotalTicks;
		entryPtr->value = regValue;
		entryPtr++;
		
		// Capture loop
		do
		{
			// Grab reg value
			regValue = VIP_REGS[INTPND];
			
			// Clear new bits from pending reg
			VIP_REGS[INTCLR] = regValue;
			
			// Check for diff and append to list
			if (regValue != prevRegValue)
			{
				entryPtr->timestamp = captureTotalTicks;
				entryPtr->value = regValue;
				entryPtr++;
				
				prevRegValue = regValue;
			}
			
			captureTotalTicks++;

			// Capture until start of next frame
		} while (!(regValue & FRAMESTART));
		
		// Render
		{
			int x, y;
			BYTE *mapPtr = BGMap(0);

			// Crap simulation
			for (y = 0; y < 8; y++)
			{
				BYTE *rowPtr = mapPtr;
				
				for (x = 0; x < 48; x++)
				{
					BYTE char_index = (((gameFrameIndex + x + y) >> 4) & 0x01) + 2;

					*rowPtr = char_index;
					rowPtr++;
					*rowPtr = 0x00;
					rowPtr++;
				}
				
				mapPtr += 64 * 2;
			}
		}
		
		gameFrameIndex++;
	}

    return 0;
}