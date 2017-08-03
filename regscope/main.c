#include <functions.h>
#include <components.h>

void vipInterruptHandler()
{
	VIP_REGS[XPCTRL] = VIP_REGS[XPSTTS] | XPEN;
	// Comment this out; breaks in rustual boy
	//  BUT WORKS IN REALITY BOY I JUST FOUND OUT AFTER THE STREAM!!!
	VIP_REGS[DPCTRL] = VIP_REGS[DPSTTS] | (SYNCE | RE | DISP);
	VIP_REGS[BKCOL] = 0x03;

	vpu_vector = 0;
}

int main()
{
	//column table setup
	vbSetColTable();

	//display setup
	VIP_REGS[REST] = 0;
	//VIP_REGS[XPCTRL] = VIP_REGS[XPSTTS] | XPEN;
	//VIP_REGS[DPCTRL] = VIP_REGS[DPSTTS] | (SYNCE | RE | DISP);
	VIP_REGS[FRMCYC] = 0;
	VIP_REGS[INTCLR] = VIP_REGS[INTPND];
	//while (!(VIP_REGS[DPSTTS] & 0x3C));  //required?

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
	
	// Set VIP interrupt
	//vpu_vector = (u32)(vipInterruptHandler);
	//VIP_REGS[INTENB] |= FRAMESTART;
	
	set_intlevel(0);

	while(1) {}

    return 0;
}