#include <functions.h>
#include <components.h>

#define sizeof_array(array) ((int)(sizeof(array) / sizeof(array[0])))

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
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

const int BIT_INDICES[] =
{
	//15, 14, 13, 4, 3, 2, 1, 0
	//10, 9, 8, 7, 6, 5, 4, 3, 2, 1
	15, 12, 11, 10, 9, 8, 4, 3, 2, 1
};

typedef struct
{
	WORD timestamp;
	HWORD value;
} Entry;

#define MAX_ENTRIES 800

static Entry Entries[MAX_ENTRIES];

int main()
{
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
	
	// Main loop
	while (1)
	{
		WORD captureTotalTicks;
		HWORD regValue, prevRegValue;
		Entry *entryPtr;
		int numEntries;

		// Reset pending interrupts
		VIP_REGS[INTCLR] = 0xffff;//VIP_REGS[INTPND];

		// Wait for start of game frame and display frame
		while ((VIP_REGS[INTPND] & (GAMESTART | FRAMESTART)) != (GAMESTART | FRAMESTART))
			;
		
		// Reset pending interrupts
		VIP_REGS[INTCLR] = 0xffff;//VIP_REGS[INTPND];

		// Grab initial reg value
		regValue =
			//VIP_REGS[INTPND]
			//VIP_REGS[DPSTTS]
			VIP_REGS[XPSTTS] & 0x7fff
			;
		
		// Start capture
		captureTotalTicks = 0;
		prevRegValue = regValue;
		entryPtr = Entries;
		numEntries = 0;
		
		// Append initial value
		entryPtr->timestamp = captureTotalTicks;
		entryPtr->value = regValue;
		entryPtr++;
		numEntries++;
		
		// Capture loop
		do
		{
			// Grab reg value
			regValue =
				//VIP_REGS[INTPND]
				//VIP_REGS[DPSTTS]
				VIP_REGS[XPSTTS] & 0x7fff
				;

			// Reset bits
			//VIP_REGS[INTCLR] = regValue;
			
			// Check for diff and append to list
			if (regValue != prevRegValue)
			{
				if (numEntries > MAX_ENTRIES)
				{
					VIP_REGS[BKCOL] = 3;
					break;
				}

				entryPtr->timestamp = captureTotalTicks;
				entryPtr->value = regValue;
				entryPtr++;
				numEntries++;
				
				prevRegValue = regValue;
			}
			
			captureTotalTicks++;

			// Capture until start of next frame
		} while (!(VIP_REGS[INTPND] & FRAMESTART));
		
		// Render
		{
			int bitIndex;
			BYTE *rowPtr = BGMap(0);

			for (bitIndex = 0; bitIndex < sizeof_array(BIT_INDICES); bitIndex++)
			{
				BYTE *charPtr = rowPtr;
				int segmentStart = 0;
				int i;
				
				for (i = 0; i < numEntries; i++)
				{
					int x;
					BYTE char_index = ((Entries[i].value >> BIT_INDICES[bitIndex]) & 1) + 2;
					int nextEntryStart = i < numEntries - 1
						? (int)(((float)Entries[i + 1].timestamp / (float)captureTotalTicks) * 47.0f)
						: 47;
					int segmentLength = nextEntryStart - segmentStart;
					if (segmentLength < 1)
						segmentLength = 1;
					for (x = 0; x < segmentLength; x++)
					{
						*charPtr = char_index;
						charPtr++;
						*charPtr = 0x00;
						charPtr++;
					}
					segmentStart += segmentLength;
				}
				
				rowPtr += 64 * 2;
			}
		}
	}

    return 0;
}