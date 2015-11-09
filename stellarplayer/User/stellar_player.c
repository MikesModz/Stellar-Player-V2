 /**	
 * |----------------------------------------------------------------------
 * | Filename		: stellar_player.c
 * | Description	: Amiga module player implemented on the TI Stellaris 
 * |  				  Launchpad evaluation board LM4F120.
 * | Author			: Michael Williamson
 * | Created		: 23/09/15
 * | Revised		: 22/10/15
 * | Version		: 0.1	 
 * |
 * | Basically just a port Ronen K's mod player (see below) to Keil 
 * | uVision. Added TFT display for navigation and play back status as
 * | well general code tidy up.
 * |
 * | Based on Stellaris Launchpad MOD Player by Ronen K.
 * |	http://mobile4dev.blogspot.de/2012/11/stellaris-launchpad-mod-player.html
 * |	http://mobile4dev.blogspot.de/search/label/MOD%20Player
 * |
 * | which in turn is based on ...
 * |
 * | MOD & S3M Player engine for PIC32 by Serveur Perso.
 * |	https://www.youtube.com/watch?v=i3Yl0TISQBE
 * |
 * | This program is free software: you can redistribute it and/or modify
 * | it under the terms of the GNU General Public License as published by
 * | the Free Software Foundation, either version 3 of the License, or
 * | any later version.
 * |  
 * | This program is distributed in the hope that it will be useful,
 * | but WITHOUT ANY WARRANTY; without even the implied warranty of
 * | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * | GNU General Public License for more details.
 * | 
 * | You should have received a copy of the GNU General Public License
 * | along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * |----------------------------------------------------------------------
 */
 
/* Include core modules */
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_timer.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "drivers/buttons.h"

/* Additional includes */
#include "global.h"
#include "tft\tft.h"
//#include "tft\tft_il9341.h"
#include "fatfs\ff.h"
#include "fatfs\diskio.h"

/* Global variables */
uint16_t totalFiles = 0;
uint16_t last = MAX_DISPLAY_ITEMS;
uint16_t current = 0;
unsigned long g_ulFlags;
volatile char uButton = 0;
volatile char uButtonPrev = 0;
bool playNextMod = false;
bool autoPlayEnabled = false;

int main()
{
	WORD i = 0;
	unsigned long ulPeriod = 0;

	/* Enable lazy stacking for interrupt handlers */
	ROM_FPULazyStackingEnable();

	/* Initialise system clock for maximum system speed */
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

	/* Initialise the TFT display */
	InitialiseDisplayTFT();

	/* Clear the TFT display */
	fill_screen_tft(Color565(0, 0, 0));
	
	/* Enable required peripherals */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

	/* Configure timer for red LED */
	TimerConfigure(TIMER0_BASE,	TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);
	TimerLoadSet(TIMER0_BASE, TIMER_B, 0xFFFF);
	TimerMatchSet(TIMER0_BASE, TIMER_B, 0);

	/* Configure timer for blue LED */
	TimerConfigure(TIMER1_BASE,	TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);
	TimerLoadSet(TIMER1_BASE, TIMER_A, 0xFFFF);
	TimerMatchSet(TIMER1_BASE, TIMER_A, 0x0);

	/* Configure timer for green LED */
	TimerLoadSet(TIMER1_BASE, TIMER_B, 0xFFFF);
	TimerMatchSet(TIMER1_BASE, TIMER_B, 0);

	/* Invert inputs */
	HWREG(TIMER0_BASE + TIMER_O_CTL) |= 0x4000;
	HWREG(TIMER1_BASE + TIMER_O_CTL) |= 0x40;
	HWREG(TIMER1_BASE + TIMER_O_CTL) |= 0x4000;

	/* Enable timers */
	TimerEnable(TIMER0_BASE, TIMER_BOTH);
	TimerEnable(TIMER1_BASE, TIMER_BOTH);

	/* Configure red LED pin */
	GPIOPinConfigure(GPIO_PF1_T0CCP1);
	GPIOPinTypeTimer(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_8MA_SC, GPIO_PIN_TYPE_STD);
	
	/* Configure green LED pin */
	GPIOPinConfigure(GPIO_PF3_T1CCP1);
	GPIOPinTypeTimer(GPIO_PORTF_BASE, GPIO_PIN_3);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_STRENGTH_8MA_SC, GPIO_PIN_TYPE_STD);

	/* Configure blue LED pin */
	GPIOPinConfigure(GPIO_PF2_T1CCP0);
	GPIOPinTypeTimer(GPIO_PORTF_BASE, GPIO_PIN_2);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_STRENGTH_8MA_SC, GPIO_PIN_TYPE_STD);

	/* Configure left channel PWM pin */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	GPIOPinConfigure(GPIO_PB1_T2CCP1);
	GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_1);

	/* Configure right channel PWM pin */
	GPIOPinConfigure(GPIO_PB0_T2CCP0);
	GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_0);

	/* Enable TIMER2 (left and right PWM outputs) */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
	
	/* Configure TIMER2 split PWM enable */
	TimerConfigure(TIMER2_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);
	
	/* Configure TIMER2 left timer */
	TimerLoadSet(TIMER2_BASE, TIMER_A, 1 << BITDEPTH);
	TimerMatchSet(TIMER2_BASE, TIMER_A, 0);

	/* Configure TIMER2 right timer */
	TimerLoadSet(TIMER2_BASE, TIMER_B, 1 << BITDEPTH);
	TimerMatchSet(TIMER2_BASE, TIMER_B, 0);
	
	/* Enable both timers */
	TimerEnable(TIMER2_BASE, TIMER_BOTH);

	/* Enable TIMER3 (sample timer) */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
	
	/* Configure TIMER3 and set period */
	TimerConfigure(TIMER3_BASE, TIMER_CFG_32_BIT_PER);
	ulPeriod = SYSCLK / SAMPLERATE;
	TimerLoadSet(TIMER3_BASE, TIMER_A, ulPeriod + 1);

	/* Enable TIMER3 */
	TimerEnable(TIMER3_BASE, TIMER_A);

	/* Initialise button hardware */
	ButtonsInit();

	/* Initialise SysTick interrupt for handling key presses */
	SysTickPeriodSet(SysCtlClockGet() / 16);
	
	/* Enable SysTick */
	SysTickEnable();

	/* Set interrupt priorities */
	IntPrioritySet(INT_TIMER3A, 0x00);
	IntPrioritySet(FAULT_SYSTICK, 0x80);

	/* Enable interrupts */
	ROM_IntMasterEnable();

	IntEnable(INT_TIMER3A);
	TimerIntEnable(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
	SysTickIntEnable();
	
	/* Show the splash screen */
	ShowSplashScreen();

	/* Disable interrupts while we initialise the SD card */
	ROM_IntMasterDisable();
	
	/* Try to initialise the SD card */
	while (disk_initialize(0));
	
	/* Mount filesystem */
	f_mount(0, &fso);
	
	/* Change to default directory */
	f_chdir(PATH);
	
	/* Open default directory */
	f_opendir(&dir, ".");

	/* Check for autoplay enabled file */
	autoPlayEnabled = checkAutoPlayEnable();
	
	/* Update display */
	if(autoPlayEnabled) CenterTextTFT("Autoplay: Enabled",110,Color565(0,255,0),Color565(0,0,0),0);
	else CenterTextTFT("Autoplay: Disabled",110,Color565(0,255,0),Color565(0,0,0),0);
	
	/* Populate file list with mods found on SD card */
	totalFiles = loadFileList();
	
	/* Delay before starting playback */
	my_delay_tft(2000);

	/* If one or more mods found then */
	if (totalFiles > 0)
	{
		/* Clear the TFT display */
		fill_screen_tft(Color565(0, 0, 0));
		
		/* Show the file list */
		UpdateFileListBox(current, last);

		/* Load currently selected file */
		loadFile(current);
	}
	else
	{
		CenterTextTFT("No mods found!",130,Color565(255,0,0),Color565(0,0,0),0);
		while(1);
	}

	/* Reenable interrupts to start play back */
	ROM_IntMasterEnable();

	for (;;)
	{
		while ((SoundBuffer.writePos + 1 & SOUNDBUFFERSIZE - 1) != SoundBuffer.readPos)
		{
			if (!i)
			{
				if (autoPlayEnabled && playNextMod)
				{
					IncrementMenu(&current, &last, totalFiles);
					loadFile(current);
					playNextMod = false;
				}
				else
				{
					/* Button states changed */
					if (uButton != uButtonPrev)
					{
						/* Save current button states */
						uButtonPrev = uButton;
						
						/* Check for left button pressed */
						if (uButton & LEFT_BUTTON)
						{
							/* Load previous mod file */
							DecrementMenu(&current, &last);
							loadFile(current);
						}
						
						/* Check for right button pressed */
						if (uButton & RIGHT_BUTTON)
						{
							/* Load next mod file */
							IncrementMenu(&current, &last, totalFiles);
							loadFile(current);
						}
					}
				}
				player();
				i = getSamplesPerTick();
			}
			mixer();
			i--;
		}
	}
}

/**	
 * Function: 		Timer3IntHandler
 * Called with:  	Nothing.
 * Returns:      	Nothing.
 * Purpose:      	Sample timer interrupt handler.
 */
void Timer3IntHandler(void)
{
	/* Clear interrupt request */
	ROM_TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);

	/* Toggle the flag for the first timer */
	HWREGBITW(&g_ulFlags, 0) ^= 1;

	if (SoundBuffer.writePos != SoundBuffer.readPos)
	{
		/* Update left & right channel outputs */
		TimerMatchSet(TIMER2_BASE, TIMER_B, SoundBuffer.left[SoundBuffer.readPos]);
		TimerMatchSet(TIMER2_BASE, TIMER_A, SoundBuffer.right[SoundBuffer.readPos]);

		/* Update red LED visualizer */
		TimerMatchSet(TIMER0_BASE, TIMER_B, (SoundBuffer.left[SoundBuffer.readPos] - 850) << 5);
		
		/* Update blue LED visualizer */
		TimerMatchSet(TIMER1_BASE, TIMER_A, (SoundBuffer.right[SoundBuffer.readPos] - 850) << 5);
		
		/* Update read position */
		SoundBuffer.readPos++;
		SoundBuffer.readPos &= SOUNDBUFFERSIZE - 1;
	}
}

/**	
 * Function: 		AppButtonHandler
 * Called with:  	ulButtons = New button states.
 * Returns:      	Nothing.
 * Purpose:      	Sets the left and right button state flags.
 */
void AppButtonHandler(unsigned long ulButtons)
{
	/* Select button press if any */
	switch (ulButtons & ALL_BUTTONS)
	{
		case LEFT_BUTTON:
			uButton = LEFT_BUTTON;
			break;
		
		case RIGHT_BUTTON:
			uButton = RIGHT_BUTTON;
			break;
		
		case ALL_BUTTONS:
			uButton = ALL_BUTTONS;
			break;
		
		default:
			uButton = 0;
			break;
	}
}

/**	
 * Function: 		SysTickIntHandler
 * Called with:  	Nothing.
 * Returns:      	Nothing.
 * Purpose:      	SysTick interrupt handler.
 */
void SysTickIntHandler(void)
{
	unsigned long ulButtons = 0;

	/* Read button states */
	ulButtons = ButtonsPoll(0, 0);
	
	/* Update flags */
	AppButtonHandler(ulButtons);
}

bool checkAutoPlayEnable(void)
{
	FIL file;
	/* Open file */
	if(f_open(&file, (const TCHAR*) "autoplay.dat", FA_READ) != FR_OK)
	{
		return false;
	}
	f_close(&file);
	return true;
}
