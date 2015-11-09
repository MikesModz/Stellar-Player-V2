 /**	
 * |----------------------------------------------------------------------
 * | Filename		: tft.c
 * | Description	: Display driver for ST7735 based TFT display.
 * | Author			: Michael Williamson
 * | Created		: 23/09/15
 * | Revised		: 23/09/15
 * | Version		: 0.1	 
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
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "string.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "drivers/buttons.h"

/* Additional includes */
#include "..\global.h"
#include "tft.h"
#include "glcdfont.h"
#include "integer.h"
#include "..\fatfs\ff.h"
#include "..\fatfs\diskio.h"

/* Static variables */
static SHORT cursor_x, cursor_y;
static USHORT textcolor, textbgcolor;
static UCHAR wrap;

/* Static function prototypes */
static void write_spi_tft(UCHAR b);
static void write_tft_command_tft(UCHAR b);
static void write_tft_data_tft(UCHAR b);
static void init_sequence_tft(void);
static void draw_fast_v_line_tft(SHORT x, SHORT y, SHORT h, USHORT color);
static void draw_fast_h_line_tft(SHORT x, SHORT y, SHORT w, USHORT color);
static void write_tft(UCHAR c);
static void TrimLeadingTrailingSpaces(char *string);

void InitialiseDisplayTFT(void)
{
	/* Enable peripherals */
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

	SysCtlDelay(100);

	/* Set command and reset pin as outputs */
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, TFT_RESET_PIN | TFT_COMMAND_PIN);

	/* Enable CS pin as output */
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_5);

	/* Enable pin PB7 for SSI2 SSI2TX */
	ROM_GPIOPinConfigure(GPIO_PB7_SSI2TX);
	ROM_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_7);

	/* Enable pin PB6 for SSI2 SSI2RX */
	ROM_GPIOPinConfigure(GPIO_PB6_SSI2RX);
	ROM_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_6);

	/* Enable pin PB4 for SSI2 SSI2CLK */
	ROM_GPIOPinConfigure(GPIO_PB4_SSI2CLK);
	ROM_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4);

	/* Configure the SSI2 port to run at 15.0MHz */
	ROM_SSIConfigSetExpClk(SSI2_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
			SSI_MODE_MASTER, 15000000, 8);
	ROM_SSIEnable(SSI2_BASE);

	/* Reset the display */
	TFT_RESET_HIGH;
	my_delay_tft(100);

	TFT_RESET_LOW;
	my_delay_tft(100);

	TFT_RESET_HIGH;
	my_delay_tft(100);

	/* Send command sequence */
	init_sequence_tft();

	cursor_y = cursor_x = 0;
	textcolor = 0xFFFF;
	textbgcolor = 0x0000;
	wrap = 1;
}

void ShowSplashScreen(void)
{
	CenterTextTFT("S3M/MOD32 Mod",20,Color565(255,0,0),Color565(0,0,255),0);
	CenterTextTFT("Player v1.0.1",30,Color565(255,0,0),Color565(0,0,255),0);
	CenterTextTFT("(c) 2015 by",50,Color565(255,255,255),Color565(0,0,0),0);
	CenterTextTFT("Michael Williamson",60,Color565(255,255,255),Color565(0,0,0),0);
	CenterTextTFT("Based on designs by",70,Color565(255,255,255),Color565(0,0,0),0);
	CenterTextTFT("Ronen K and",80,Color565(255,255,255),Color565(0,0,0),0);
	CenterTextTFT("Serveur Perso",90,Color565(255,255,255),Color565(0,0,0),0);
	CenterTextTFT("Loading files...",120,Color565(255,255,255),Color565(0,0,0),0);
}

void UpdateFileListBox(uint16_t current, uint16_t last)
{
	uint16_t i = 0;
	uint16_t line = 0;
	uint8_t tempBuffer[13];

	CenterTextTFT("Select a file", 10, Color565(255, 0, 0), Color565(0, 0, 0), 1);
	for (i = last - MAX_DISPLAY_ITEMS; i < last; i++)
	{
		getModFileNameNew(tempBuffer, i);
		if (i == current)
		{
			CenterTextTFT((char*) tempBuffer, (line * 10) + 30, Color565(255, 0, 0), Color565(0, 0, 255), 1);
		}
		else
		{
			CenterTextTFT((char*) tempBuffer, (line * 10) + 30, Color565(255, 255, 255), Color565(0, 0, 0), 1);
		}
		line++;
	}
}

void IncrementMenu(uint16_t *current, uint16_t *last, uint16_t totalFiles)
{
	uint16_t l_current = 0;
	uint16_t l_last = 0;

	l_current = *current;
	l_last = *last;

	if (l_current < totalFiles - 1)
	{
		l_current++;
		if (l_current >= l_last)
		{
			l_last++;
		}
	}
	else
	{
		l_current = 0;
		l_last = MAX_DISPLAY_ITEMS;
	}

	UpdateFileListBox(l_current, l_last);

	*current = l_current;
	*last = l_last;
}

void DecrementMenu(uint16_t *current, uint16_t *last)
{
	uint16_t l_current = 0;
	uint16_t l_last = 0;

	l_current = *current;
	l_last = *last;

	if (l_current > FIRST_FILENAME)
	{
		l_current--;
		if (l_current < (l_last - MAX_DISPLAY_ITEMS))
		{
			l_last--;
		}
	}
	UpdateFileListBox(l_current, l_last);

	*current = l_current;
	*last = l_last;
}

void CenterTextTFT(char *string, UINT y_pos, USHORT f_col, USHORT b_col, UCHAR fill)
{
	UINT w = 0;

	TrimLeadingTrailingSpaces(string);
	w = strlen(string) * 6;
	if (w <= _width)
	{
		if (fill) fill_rect_tft(0, y_pos, _width, 8, b_col);
		set_cursor_tft((_width - w) / 2, y_pos);
		set_text_wrap_tft(0);
		set_text_color_tft(f_col, b_col);
		print_tft(string);
	}

}

void fill_screen_tft(USHORT color)
{
	fill_rect_tft(0, 0, _width, _height, color);
}

void draw_rect_tft(SHORT x, SHORT y, SHORT w, SHORT h, USHORT color)
{
	draw_fast_h_line_tft(x, y, w, color);
	draw_fast_h_line_tft(x, y + h - 1, w, color);
	draw_fast_v_line_tft(x, y, h, color);
	draw_fast_v_line_tft(x + w - 1, y, h, color);
}

void draw_line_tft(SHORT x0, SHORT y0, SHORT x1, SHORT y1, USHORT color)
{
	SHORT dx, dy;
	SHORT err = 0;
	SHORT ystep;

	SHORT steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep)
	{
		swap(x0, y0);
		swap(x1, y1);
	}

	if (x0 > x1)
	{
		swap(x0, x1);
		swap(y0, y1);
	}

	dx = x1 - x0;
	dy = abs(y1 - y0);

	err = dx / 2;

	if (y0 < y1)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	for (; x0 <= x1; x0++)
	{
		if (steep)
		{
			draw_pixel_tft(y0, x0, color);
		}
		else
		{
			draw_pixel_tft(x0, y0, color);
		}
		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

void draw_char_tft(SHORT x, SHORT y, UCHAR c, USHORT color, USHORT bg)
{
	UCHAR i = 0;
	UCHAR j = 0;

	if ((x >= _width) ||		// Clip right
		(y >= _height) ||		// Clip bottom
		((x + 5 - 1) < 0) ||	// Clip left
		((y + 8 - 1) < 0))   	// Clip top
	{
		return;
	}

	c = c - 32;
	for (i = 0; i < 6; i++)
	{
		UCHAR line;
		if ((i == 5) || (c > (128 - 32)))
		{
			// All invalid characters will print as a space
			line = 0;
		}
		else
		{
			line = font[(c * 5) + i];
		}

		set_addr_window_tft(x + i, y, x + i, y + 7);
		TFT_COMMAND_HIGH;

		for (j = 0; j < 8; j++)
		{
			if (line & 0x1)
			{
				write_spi_tft(color >> 8);
				write_spi_tft(color & 0xff);
			}
			else
			{
				write_spi_tft(bg >> 8);
				write_spi_tft(bg & 0xff);
			}
			line >>= 1;
		}
	}
}

void print_tft(const char str[])
{
	int x = 0;
	while (str[x])
	{
		write_tft(str[x]);
		x++;
	}
}

void set_cursor_tft(SHORT x, SHORT y)
{
	cursor_x = x;
	cursor_y = y;
}

void set_text_color_tft(USHORT c, USHORT b)
{
	textcolor = c;
	textbgcolor = b;
}

void set_text_wrap_tft(UCHAR w)
{
	wrap = w;
}

void fill_rect_tft(SHORT x, SHORT y, SHORT w, SHORT h, USHORT color)
{
	// rudimentary clipping (draw_char_tft w/big text requires this)
	if ((x >= _width) || (y >= _height))
		return;
	
	if ((x + w - 1) >= _width)
		w = _width - x;
	
	if ((y + h - 1) >= _height)
		h = _height - y;

	set_addr_window_tft(x, y, x + w - 1, y + h - 1);

	TFT_COMMAND_HIGH;

	for (y = h; y > 0; y--)
	{
		for (x = w; x > 0; x--)
		{
			spi_stream_pixel_tft(color);
		}
	}
}

void spi_stream_pixel_tft(USHORT color)
{
	write_spi_tft(color >> 8);
	write_spi_tft(color & 0xff);
}

void draw_pixel_tft(SHORT x, SHORT y, USHORT color)
{
	if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
		return;

	set_addr_window_tft(x, y, x + 1, y + 1);

	TFT_COMMAND_HIGH;
	spi_stream_pixel_tft(color);
}

void my_delay_tft(USHORT ms)
{
	USHORT x = 0;
	for (x = 0; x < ms; x += 5)
	{
		//_delay_ms(5);
		// 3 cycles per loop. 1/80 Mhz = 12.5 ns
		// 12.5 ns * 3 = 37.5 ns per loop.
		// 150000 * 37.5 ns = 5.625 ms
		SysCtlDelay(150000);
	}
}

void set_addr_window_tft(UCHAR x0, UCHAR y0, UCHAR x1, UCHAR y1)
{
	write_tft_command_tft(ST7735_CASET);		// Column addr set
	write_tft_data_tft(0x00);
	write_tft_data_tft(x0);						// XSTART
	write_tft_data_tft(0x00);
	write_tft_data_tft(x1);						// XEND

	write_tft_command_tft(ST7735_RASET);		// Row addr set
	write_tft_data_tft(0x00);
	write_tft_data_tft(y0);						// YSTART
	write_tft_data_tft(0x00);
	write_tft_data_tft(y1);						// YEND

	write_tft_command_tft(ST7735_RAMWR);		// write_tft to RAM
}

static void write_spi_tft(BYTE b)
{
	DWORD rcvdat;

	TFT_CS_LOW;
	ROM_SSIDataPut(SSI2_BASE, b); /* Write the data to the tx fifo */
	ROM_SSIDataGet(SSI2_BASE, &rcvdat); /* flush data read during the write */
	TFT_CS_HIGH;
}

static void write_tft_command_tft(UCHAR b)
{
	TFT_COMMAND_LOW;
	write_spi_tft(b);
}

static void write_tft_data_tft(UCHAR b)
{
	TFT_COMMAND_HIGH;
	write_spi_tft(b);
}

static void init_sequence_tft(void)
{
	write_tft_command_tft(ST7735_SWRESET);		//  1: Software reset, 0 args, w/delay
	my_delay_tft(150);

	write_tft_command_tft(ST7735_SLPOUT);		//  2: Out of sleep mode, 0 args, w/delay
	my_delay_tft(500);

	write_tft_command_tft(ST7735_FRMCTR1);		//  3: Frame rate ctrl - normal mode, 3 args:
	write_tft_data_tft(0x01);					//     Rate = fosc/(1x2+40) * (LINE+2C+2D)
	write_tft_data_tft(0x2C);
	write_tft_data_tft(0x2D);

	write_tft_command_tft(ST7735_FRMCTR2);		//  4: Frame rate control - idle mode, 3 args:
	write_tft_data_tft(0x01);					//     Rate = fosc/(1x2+40) * (LINE+2C+2D)
	write_tft_data_tft(0x2C);
	write_tft_data_tft(0x2D);

	write_tft_command_tft(ST7735_FRMCTR3);		//  5: Frame rate ctrl - partial mode, 6 args:
	write_tft_data_tft(0x01);					//     Dot inversion mode
	write_tft_data_tft(0x2C);					//     Line inversion mode
	write_tft_data_tft(0x2D);
	write_tft_data_tft(0x01);
	write_tft_data_tft(0x2C);
	write_tft_data_tft(0x2D);

	write_tft_command_tft(ST7735_INVCTR);		//  6: Display inversion ctrl, 1 arg, no delay:
	write_tft_data_tft(0x07);					//     No inversion

	write_tft_command_tft(ST7735_PWCTR1);		//  7: Power control, 3 args, no delay:
	write_tft_data_tft(0xA2);
	write_tft_data_tft(0x02);					//     -4.6V
	write_tft_data_tft(0x84);					//     AUTO mode

	write_tft_command_tft(ST7735_PWCTR2);		//  8: Power control, 1 arg, no delay:
	write_tft_data_tft(0xC5);					//     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD

	write_tft_command_tft(ST7735_PWCTR3);		//  9: Power control, 2 args, no delay:
	write_tft_data_tft(0x0A);					//     Opamp current small
	write_tft_data_tft(0x00);					//     Boost frequency

	write_tft_command_tft(ST7735_PWCTR4);		// 10: Power control, 2 args, no delay:
	write_tft_data_tft(0x8A);					//     BCLK/2, Opamp current small & Medium low
	write_tft_data_tft(0x2A);

	write_tft_command_tft(ST7735_PWCTR5);		// 11: Power control, 2 args, no delay:
	write_tft_data_tft(0x8A);
	write_tft_data_tft(0xEE);

	write_tft_command_tft(ST7735_VMCTR1);		// 12: Power control, 1 arg, no delay:
	write_tft_data_tft(0x0E);

	write_tft_command_tft(ST7735_INVOFF);		// 13: Don't invert display, no args, no delay

	write_tft_command_tft(ST7735_MADCTL);		// 14: Memory access control (directions), 1 arg:
	write_tft_data_tft(0xC8);					//     row addr/col addr, bottom to top refresh

	write_tft_command_tft(ST7735_COLMOD);		// 15: set color mode, 1 arg, no delay:
	write_tft_data_tft(0x05);					//     16-bit color

	write_tft_command_tft(ST7735_CASET);		//  1: Column addr set, 4 args, no delay:
	write_tft_data_tft(0x00);					//     XSTART = 0
	write_tft_data_tft(0x02);
	write_tft_data_tft(0x00);					//     XEND = 127
	write_tft_data_tft(0x7F + 0x02);

	write_tft_command_tft(ST7735_RASET);		//  2: Row addr set, 4 args, no delay:
	write_tft_data_tft(0x00);					//     XSTART = 0
	write_tft_data_tft(0x01);
	write_tft_data_tft(0x00);
	write_tft_data_tft(0x9F + 0x01);

	write_tft_command_tft(ST7735_GMCTRP1);		//  1: Magical unicorn dust, 16 args, no delay:
	write_tft_data_tft(0x02);
	write_tft_data_tft(0x1c);
	write_tft_data_tft(0x07);
	write_tft_data_tft(0x12);
	write_tft_data_tft(0x37);
	write_tft_data_tft(0x32);
	write_tft_data_tft(0x29);
	write_tft_data_tft(0x2d);
	write_tft_data_tft(0x29);
	write_tft_data_tft(0x25);
	write_tft_data_tft(0x2b);
	write_tft_data_tft(0x39);
	write_tft_data_tft(0x00);
	write_tft_data_tft(0x01);
	write_tft_data_tft(0x03);
	write_tft_data_tft(0x10);

	write_tft_command_tft(ST7735_GMCTRN1);		//  2: Sparkles and rainbows, 16 args, no delay:
	write_tft_data_tft(0x03);
	write_tft_data_tft(0x1d);
	write_tft_data_tft(0x07);
	write_tft_data_tft(0x06);
	write_tft_data_tft(0x2e);
	write_tft_data_tft(0x2c);
	write_tft_data_tft(0x29);
	write_tft_data_tft(0x2d);
	write_tft_data_tft(0x2e);
	write_tft_data_tft(0x2e);
	write_tft_data_tft(0x37);
	write_tft_data_tft(0x3f);
	write_tft_data_tft(0x00);
	write_tft_data_tft(0x00);
	write_tft_data_tft(0x02);
	write_tft_data_tft(0x10);

	write_tft_command_tft(ST7735_NORON);		//  3: Normal display on, no args, w/delay
	my_delay_tft(10);							//     10 ms delay

	write_tft_command_tft(ST7735_DISPON);		//  4: Main screen turn on, no args w/delay
	my_delay_tft(100);							//     100 ms delay
}

static void draw_fast_v_line_tft(SHORT x, SHORT y, SHORT h, USHORT color)
{
	// Rudimentary clipping
	if ((x >= _width) || (y >= _height))
		return;
	
	if ((y + h - 1) >= _height)
		h = _height - y;
	
	set_addr_window_tft(x, y, x, y + h - 1);

	TFT_COMMAND_HIGH;

	while (h--)
	{
		spi_stream_pixel_tft(color);
	}
}

static void draw_fast_h_line_tft(SHORT x, SHORT y, SHORT w, USHORT color)
{
	// Rudimentary clipping
	if ((x >= _width) || (y >= _height))
		return;
	
	if ((x + w - 1) >= _width)
		w = _width - x;
	
	set_addr_window_tft(x, y, x + w - 1, y);

	TFT_COMMAND_HIGH;

	while (w--)
	{
		spi_stream_pixel_tft(color);
	}
}

static void write_tft(UCHAR c)
{
	if (c == '\n') {
		cursor_y += 8;
		cursor_x = 0;
	}
	else if (c == '\r')
	{
		// skip em
	}
	else
	{
		draw_char_tft(cursor_x, cursor_y, c, textcolor, textbgcolor);
		cursor_x += 6;
		if (wrap && (cursor_x > (_width - 6)))
		{
			cursor_y += 8;
			cursor_x = 0;
		}
	}
}

static void TrimLeadingTrailingSpaces(char *string)
{
	const char *firstNonSpace = string;
	char* endOfString = NULL;
	size_t len = 0;
	while (*firstNonSpace != '\0' && isspace(*firstNonSpace))
	{
		firstNonSpace++;
	}

	len = strlen(firstNonSpace);
	memmove(string, firstNonSpace, len);

	endOfString = string + len - 1;
	while (string < endOfString && isspace(*endOfString))
	{
		endOfString--;
	}
	*++endOfString = '\0';
}
