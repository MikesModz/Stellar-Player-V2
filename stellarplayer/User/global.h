 /**	
 * |----------------------------------------------------------------------
 * | Filename		: global.h
 * | Author			: Ronen K
 * | Created		: 10/12
 * |----------------------------------------------------------------------
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

/* Include core modules */
#include <stdbool.h>
#include <stdint.h>

/* Additional includes */
#include ".\fatfs\ff.h"

/* System clock frequency */
#define SYSCLK 80000000L

/* Default path on SD card for files */
#define PATH "/mods"

/* Playback configuration definitions */
#define BITDEPTH 11                               					// 11 bits PWM
//#define SAMPLERATE 			(SYSCLK / (1 << BITDEPTH))     		// Best audio quality 80MHz / (1 << 11) = 39062.5Hz
#define SAMPLERATE 				(SYSCLK / (1 << BITDEPTH) / 2) 		// Medium audio quality 80MHz / (1 << 11) / 2 = 19531.25Hz

#define SOUNDBUFFERSIZE 		512                      	// Stereo circular sound buffer 2 * 2048 = 4096 bytes of memory
#define FATBUFFERSIZE 			1024                     	// File system buffers CHANNELS * 512 = 1152 bytes of memory
#define DIVIDER 				10                       	// Fixed-point mantissa used for integer arithmetic
#define STEREOSEPARATION 		32                       	// 0 (max) to 64 (mono)

// Hz = 14317056 / amigaPeriod
#define AMIGA 					(14317056 / SAMPLERATE << DIVIDER)
#define AMIGA_MOD 				(7093789 / 2 / SAMPLERATE << DIVIDER)

#define ROWS 					64
#define INSTRUMENTS 			99
#define CHANNELS 				14
#define NONOTE 					108
#define KEYOFF 					109
#define NOVOLUME 				255

#define SAMPLES 				31
#define NONOTE_MOD 				0xFFFF

/* Maximum number of mods supported (for display purposes) */
#define MAX_NUMBER_OF_MODS		80

/* Maximum number of mods shown in the menu */
#define MAX_DISPLAY_ITEMS		8

/* First mod */
#define FIRST_FILENAME			0

/* Visualiszer LED definitions */
#define RED_LED   				GPIO_PIN_1
#define BLUE_LED  				GPIO_PIN_2
#define GREEN_LED 				GPIO_PIN_3

#define min(X,Y) ((X) < (Y) ? (X) : (Y))

typedef struct
{
	uint8_t 		name[22];
	uint16_t 		length;
	int8_t 			fineTune;
	uint8_t 		volume;
	uint16_t 		loopBegin;
	uint16_t 		loopLength;
} MOD_Sample;

typedef struct
{
	uint8_t 		sampleNumber[ROWS][CHANNELS];
	uint16_t 		note[ROWS][CHANNELS];
	uint8_t 		effectNumber[ROWS][CHANNELS];
	uint8_t 		effectParameter[ROWS][CHANNELS];
} MOD_Pattern;

struct mod
{
	uint8_t 		name[20];
	MOD_Sample 		samples[SAMPLES];
	uint8_t 		songLength;
	uint8_t 		numberOfPatterns;
	uint8_t 		order[128];
	uint8_t 		numberOfChannels;
};

struct mod_player
{
	MOD_Pattern 	currentPattern;
	uint32_t 		amiga;
	uint16_t 		samplesPerTick;
	uint8_t 		speed;
	uint8_t 		tick;
	uint8_t 		row;
	uint8_t 		lastRow;
	uint8_t 		orderIndex;
	uint8_t 		oldOrderIndex;
	uint8_t 		patternDelay;
	uint8_t 		patternLoopCount[CHANNELS];
	uint8_t 		patternLoopRow[CHANNELS];
	uint8_t 		lastSampleNumber[CHANNELS];
	int8_t 			volume[CHANNELS];
	uint16_t 		lastNote[CHANNELS];
	uint16_t 		amigaPeriod[CHANNELS];
	int16_t 		lastAmigaPeriod[CHANNELS];
	uint16_t 		portamentoNote[CHANNELS];
	uint8_t 		portamentoSpeed[CHANNELS];
	uint8_t 		waveControl[CHANNELS];
	uint8_t 		vibratoSpeed[CHANNELS];
	uint8_t 		vibratoDepth[CHANNELS];
	int8_t 			vibratoPos[CHANNELS];
	uint8_t 		tremoloSpeed[CHANNELS];
	uint8_t 		tremoloDepth[CHANNELS];
	int8_t 			tremoloPos[CHANNELS];
};

typedef struct
{
	uint8_t 		name[28];
	uint16_t 		sampleParapointer;
	uint16_t 		length;
	uint16_t 		loopBegin;
	uint16_t 		loopEnd;
	uint8_t 		volume;
	uint16_t 		middleC;
	bool 			loop;
} S3M_Instrument;

typedef struct
{
	uint16_t 		note[ROWS][CHANNELS];
	uint8_t 		instrumentNumber[ROWS][CHANNELS];
	uint8_t 		volume[ROWS][CHANNELS];
	uint8_t 		effectNumber[ROWS][CHANNELS];
	uint8_t 		effectParameter[ROWS][CHANNELS];
} S3M_Pattern;

struct s3m
	{
	uint8_t 		name[28];
	S3M_Instrument 	instruments[INSTRUMENTS];
	uint16_t 		songLength;
	uint16_t 		numberOfInstruments;
	uint16_t 		numberOfPatterns;
	bool 			fastVolumeSlides;
	uint8_t 		globalVolume;
	uint8_t 		order[256];
	uint8_t 		numberOfChannels;
	uint8_t 		channelRemapping[CHANNELS];
	uint16_t 		instrumentParapointers[INSTRUMENTS];
	uint16_t 		patternParapointers[256];
};

struct s3m_player
{
	S3M_Pattern 	currentPattern;
	uint32_t 		amiga;
	uint16_t 		samplesPerTick;
	uint8_t 		speed;
	uint8_t 		tick;
	uint8_t 		row;
	uint8_t 		lastRow;
	uint8_t 		orderIndex;
	uint8_t 		oldOrderIndex;
	uint8_t 		patternDelay;
	uint8_t 		patternLoopCount[CHANNELS];
	uint8_t 		patternLoopRow[CHANNELS];
	uint8_t 		lastInstrumentNumber[CHANNELS];
	int8_t 			volume[CHANNELS];
	uint16_t 		lastNote[CHANNELS];
	uint16_t 		amigaPeriod[CHANNELS];
	int16_t 		lastAmigaPeriod[CHANNELS];
	uint8_t 		lastVolumeSlide[CHANNELS];
	uint8_t 		lastPortamento[CHANNELS];
	uint16_t 		portamentoNote[CHANNELS];
	uint8_t 		portamentoSpeed[CHANNELS];
	uint8_t 		waveControl[CHANNELS];
	uint8_t 		vibratoSpeed[CHANNELS];
	uint8_t 		vibratoDepth[CHANNELS];
	int8_t 			vibratoPos[CHANNELS];
	uint8_t 		tremoloSpeed[CHANNELS];
	uint8_t 		tremoloDepth[CHANNELS];
	int8_t 			tremoloPos[CHANNELS];
	uint8_t 		tremorOnOff[CHANNELS];
	uint8_t 		tremorCount[CHANNELS];
	uint8_t 		retriggerVolumeSlide[CHANNELS];
	uint8_t 		retriggerSpeed[CHANNELS];
};

union umod
{
	struct mod 		Mod;
	struct s3m 		S3m;
};

union player
{
	struct mod_player 	Mod_player;
	struct s3m_player 	S3m_player;
};

struct mixer
{
	uint32_t 		sampleBegin[INSTRUMENTS];
	uint32_t 		sampleEnd[INSTRUMENTS];
	uint32_t 		sampleLoopBegin[INSTRUMENTS];
	uint16_t 		sampleLoopLength[INSTRUMENTS];
	uint32_t 		sampleLoopEnd[INSTRUMENTS];

	uint8_t 		channelSampleNumber[CHANNELS];
	uint32_t 		channelSampleOffset[CHANNELS];
	uint16_t 		channelFrequency[CHANNELS];
	uint8_t 		channelVolume[CHANNELS];
	uint8_t 		channelPanning[CHANNELS];
};

struct fatBuffer
{
	uint8_t 		channels[CHANNELS][FATBUFFERSIZE];
	uint32_t 		samplePointer[CHANNELS];
	uint8_t 		channelSampleNumber[CHANNELS];
};

struct soundBuffer
{
	uint16_t 			left[SOUNDBUFFERSIZE];
	uint16_t 			right[SOUNDBUFFERSIZE];
	uint16_t 			writePos;
	volatile uint16_t 	readPos;
};

typedef struct
{
	char szFileExt[5];
	void (*player)(void);
	void (*mixer)(void);
	void (*loader)(void);
	uint16_t (*getSamplesPerTick)(void);
}FileHandler;

extern union 	player uPlayer;
extern union 	umod uMod;
extern struct 	soundBuffer SoundBuffer;
extern struct 	mixer Mixer;
extern struct 	fatBuffer FatBuffer;

extern FATFS 	fso;
extern DIR 		dir;
extern FILINFO 	fileInfo;
extern FIL 		file;

extern bool playNextMod;
extern bool autoPlayEnabled;
	
void player(void);
void mixer(void);
void loadNextFile(void);
void loadPreviousFile(void);
uint16_t loadFileList(void);
uint16_t getSamplesPerTick(void);
uint8_t *getModFileName(uint16_t number);
void getModFileNameNew(uint8_t *buffer, uint16_t number);
void loadFile(uint16_t number);
bool checkAutoPlayEnable(void);

#endif /* GLOBAL_H_ */
