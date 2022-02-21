/*******************************************************/
/** 					 RamViewer       			  **/
/**                    ZHONX 12/2021                  **/
/*******************************************************/

#include <string>
#include <errno.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <inttypes.h>
#include <unistd.h>

#include <iostream>
#include "bitmap_image.hpp"
#include "smallfonts.h"

#define PRINT_IFFT_FREQUENCY
#define PRINT_IFFT_FREQUENCY_FULL

#define CIS_ACTIVE_PIXELS_PER_LINE				(576)
#define CIS_ADC_OUT_LINES						(3)
#define PIXELS_PER_NOTE							(16)
#define NUMBER_OF_NOTES     					(((CIS_ACTIVE_PIXELS_PER_LINE) * (CIS_ADC_OUT_LINES)) / PIXELS_PER_NOTE)

#define STM32_START_WAVES_ADDR					0x24001270
#define STM32_START_UNITARY_WAVEFORM_ADDR		0x24001c90
#define START_UNITARY_WAVEFORM_OFFSET			(((STM32_START_UNITARY_WAVEFORM_ADDR) - (STM32_START_WAVES_ADDR)) / (2))

#define IMAGE_HEIGHT							1000
#define IMAGE_WAVEFORMS_SCALE					0.8

#define TEXT_POSITION							(10)
#define WAVES_POSITION							(((IMAGE_HEIGHT) / (2)) + (60))

//__attribute__ ((packed))
struct wave {
	int start_ptr;
	uint16_t current_idx;
	uint16_t area_size;
	uint16_t octave_coeff;
	int current_volume;
	int phase_polarisation;
	float frequency;
};

const char* filename = "raw_waveforms";

static void drawChar(image_drawer *draw, unsigned int x, unsigned int y, unsigned char c, const FONT_DEF *font)
{
	unsigned int xoffset;
	unsigned char  yoffset;
	unsigned char  col;
	unsigned char  column[font->u8Width];

	// Capitalize character if lower case is not available
	if ((c > font->u8LastChar) && (font->u8LastChar < 128))
	{
		c -= 32;
	}

	// Check if the requested character is available
	if ((c >= font->u8FirstChar) && (c <= font->u8LastChar))
	{
		// Retrieve appropriate columns from font data
		for (col = 0; col < font->u8Width; col++)
		{
			column[col] = font->au8FontTable[((c - 32) * font->u8Width) + col]; // Get first column of appropriate character
		}
	}
	else
	{
		// Requested character is not available in this font ... send a space instead
		for (col = 0; col < font->u8Width; col++)
		{
			column[col] = 0xFF;    // Send solid space
		}
	}

	// Render each column
	for (xoffset = 0; xoffset < font->u8Width; xoffset++)
	{
		for (yoffset = 0; yoffset < (font->u8Height + 1); yoffset++)
		{
			unsigned char  bit = 0x00;
			bit = (column[xoffset] << (8 - (yoffset + 1))); // Shift current row bit left
			bit = (bit >> 7); // Shift current row but right (results in 0x01 for black, and 0x00 for white)
			if (bit)
			{
				draw->plot_pixel(x + xoffset, y + yoffset);
			}
		}
	}
}

void drawString(image_drawer *draw, unsigned int x, unsigned int y, const char *text, const FONT_DEF *font)
{
	for (uint l = 0; l < strlen((const char*) text); l++)
	{
		// Do not send non-printable characters
		if ((text[l] <= 0x0E) || (text[l] == 0x7F))
		{
			continue;
		}
		drawChar(draw, x + (l * (font->u8Width + 1)), y, text[l], font);
	}
}

int getWaveformY(char* file_contents, int x)
{
	return WAVES_POSITION + (((unsigned char)file_contents[2 * x] | ((char)file_contents[2 * x + 1] << 8)) / (65535.0 / (IMAGE_HEIGHT * IMAGE_WAVEFORMS_SCALE)));
}

int main( int argc, char* args[] )
{
	puts("Start SSS Waveform Printer\n");

	//Open and read waveform file
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("open\n");
		exit(EXIT_FAILURE);
	}

	struct stat sb;
	if (stat(filename, &sb) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}

	char* file_contents = (char*)malloc(sb.st_size);
	read(fd, file_contents, sb.st_size);

	struct wave *waves;
	waves = (wave*)file_contents;
	char str[200] = {0};
	int octave = 0;

	//	int pixelNumber = sb.st_size / 2;
	int pixelNumber = ((waves[NUMBER_OF_NOTES - 1].start_ptr - STM32_START_UNITARY_WAVEFORM_ADDR) / 2) + waves[NUMBER_OF_NOTES - 1].area_size + 2;


	bitmap_image image(pixelNumber, IMAGE_HEIGHT);

	// set background to grey
	image.set_all_channels(75, 60, 60);

	image_drawer draw(image);

	draw.pen_width(2);

	draw.pen_color(75, 90, 60);
	draw.horiztonal_line_segment(0, pixelNumber, WAVES_POSITION);

	draw.pen_color(255, 255, 255);

	for(int x = 0; x < pixelNumber; x++)
	{
		int y = getWaveformY(file_contents, x + START_UNITARY_WAVEFORM_OFFSET);
		draw.plot_pen_pixel(x, y);
	}

	for (unsigned int note = 0; note < NUMBER_OF_NOTES; note++)
	{
		int noteOffset = (waves[note].start_ptr - STM32_START_UNITARY_WAVEFORM_ADDR) / 2;

		octave = log(waves[note].octave_coeff) / log(2);

		switch(octave)
		{
		case 0:
			draw.pen_color(10, 255, 255);
			break;
		case 1:
			draw.pen_color(250, 110, 200);
			break;
		case 2:
			draw.pen_color(100, 150, 110);
			break;
		case 3:
			draw.pen_color(255, 110, 60);
			break;
		case 4:
			draw.pen_color(255, 55, 125);
			break;
		case 5:
			draw.pen_color(255, 125, 100);
			break;
		case 6:
			draw.pen_color(155, 200, 100);
			break;
		case 7:
			draw.pen_color(200, 155, 70);
			break;
		case 8:
			draw.pen_color(0, 255, 55);
			break;
		case 9:
			draw.pen_color(255, 0, 100);
			break;
		case 10:
			draw.pen_color(255, 240, 0);
			break;
		case 11:
			draw.pen_color(90, 150, 200);
			break;
		case 12:
			draw.pen_color(200, 100, 50);
			break;
		case 13:
			draw.pen_color(125, 0, 200);
			break;
		case 14:
			draw.pen_color(200, 0, 0);
			break;
		case 15:
			draw.pen_color(95, 0, 250);
			break;
		}

		draw.vertical_line_segment(60 * octave, IMAGE_HEIGHT, noteOffset);

		sprintf(str, "Oct %d", octave);
		drawString(&draw, noteOffset + 2, (TEXT_POSITION) + (60 * octave), str, &Font_8x8Thin);

		sprintf(str, "Frq %.1fHz", waves[note].frequency);
		drawString(&draw, noteOffset + 2, (TEXT_POSITION) + 10 + (60 * octave), str, &Font_8x8Thin);

		sprintf(str, "Siz %d", waves[note].area_size);
		drawString(&draw, noteOffset + 2, (TEXT_POSITION) + 20 + (60 * octave), str, &Font_8x8Thin);

		sprintf(str, "Idx %d", waves[note].current_idx);
		drawString(&draw, noteOffset + 2, (TEXT_POSITION) + 30 + (60 * octave), str, &Font_8x8Thin);

		sprintf(str, "Vol %d", waves[note].current_volume);
		drawString(&draw, noteOffset + 2, (TEXT_POSITION) + 40 + (60 * octave), str, &Font_8x8Thin);

		draw.circle(noteOffset + waves[note].current_idx, getWaveformY(file_contents, noteOffset + waves[note].current_idx + START_UNITARY_WAVEFORM_OFFSET), 4);
	}

	image.save_image("waveform.bmp");

	puts("Waveform Printer Success !"); /* prints !!!Hello World!!! */


	return 0;
}
