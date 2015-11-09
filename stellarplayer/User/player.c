 /**	
 * |----------------------------------------------------------------------
 * | Filename		: player.c
 * | Author			: Ronen K
 * | Created		: 10/12
 * |----------------------------------------------------------------------
 */

/* Include core modules */
#include <string.h>

/* Additional includes */
#include "global.h"
#include "mod32.h"
#include "s3m32.h"

/* Common variables */
union player 		uPlayer;
union umod 			uMod;
struct soundBuffer 	SoundBuffer;
struct mixer 		Mixer;

/* Common variables */
FATFS fso;
DIR dir;
FILINFO fileInfo;
FIL file;
struct fatBuffer FatBuffer;

/* Array of mod files found on the SD card */
static uint8_t modFileNames[MAX_NUMBER_OF_MODS][13];

/* Current mod file */
static uint16_t modFileNumber = 0;

/* Prototype */
char *strcasestr(const char *s1, const char *s2);

/* Define file format handlers */
FileHandler g_fhHandlers[] =
{
	{ ".MOD", mod_player, mod_mixer, loadMod, mod_getSamplesPerTick },
	{ ".S3M", s3m_player, s3m_mixer, loadS3m, s3m_getSamplesPerTick } };

/* Current file handler */
FileHandler *g_pCurrentHandler;

void player()
{
	if (g_pCurrentHandler != NULL)
		(*(g_pCurrentHandler->player))();
}

void mixer()
{
	if (g_pCurrentHandler != NULL)
		(*(g_pCurrentHandler->mixer))();
}

uint16_t getSamplesPerTick()
{
	if (g_pCurrentHandler != NULL)
		return (*(g_pCurrentHandler->getSamplesPerTick))();
	return 0;
}

FileHandler *getHandler(const char *fileName)
{
	int i;
	if (fileName == NULL)
		return NULL;
	if (fileName[0] == '.' && fileName[1] == NULL)
		return NULL;

	if (fileName[0] == '.' && fileName[1] == '.' && fileName[2] == NULL)
		return NULL;

	for (i = 0; i < sizeof(g_fhHandlers); i++) {
		if (strcasestr(fileName, g_fhHandlers[i].szFileExt) != NULL) {
			return &(g_fhHandlers[i]);
		}
	}
	return NULL;
}

void loadNextFile()
{
	uint8_t *dotPointer;
	g_pCurrentHandler = NULL;

	do {
		f_readdir(&dir, &fileInfo);
		if (fileInfo.fname[0] == 0)
			break;
		dotPointer = (uint8_t*) strrchr((char*) fileInfo.fname, '.');
		if (dotPointer != NULL) {
			g_pCurrentHandler = getHandler((const char*) dotPointer);
			if (g_pCurrentHandler != NULL)
				break;
		}
	} while (dotPointer == NULL || g_pCurrentHandler == NULL);

	if (fileInfo.fname[0] != 0 && g_pCurrentHandler != NULL) {
		f_open(&file, fileInfo.fname, FA_READ);

		modFileNumber++;
		(*(g_pCurrentHandler->loader))();
	}
}

void loadPreviousFile() {
	uint16_t i;
	uint8_t *dotPointer;

	if (modFileNumber > 1) {
		modFileNumber--;
		f_readdir(&dir, NULL);
		for (i = 0; i < modFileNumber; i++) {
			do {
				f_readdir(&dir, &fileInfo);
				dotPointer = (uint8_t*) strrchr((const char*) fileInfo.fname,
						'.');
				if (dotPointer != NULL) {
					g_pCurrentHandler = getHandler(fileInfo.fname);

				}
			} while (dotPointer == NULL || g_pCurrentHandler == NULL);
		}
		f_open(&file, fileInfo.fname, FA_READ);
		(*(g_pCurrentHandler->loader))();
	}
}

void loadFile(uint16_t number) {
	uint8_t *dotPointer;

	getModFileNameNew((uint8_t*) fileInfo.fname, number);
	dotPointer = (uint8_t*) strrchr((const char*) fileInfo.fname, '.');
	if (dotPointer != NULL) {
		g_pCurrentHandler = getHandler((const char*) fileInfo.fname);
		if (fileInfo.fname[0] != 0 && g_pCurrentHandler != NULL) {
			f_open(&file, (const TCHAR*) fileInfo.fname, FA_READ);
			(*(g_pCurrentHandler->loader))();
		}
	}
}

uint16_t loadFileList() {
	uint8_t *dotPointer;
	uint16_t totalFiles = 0;

	do {
		f_readdir(&dir, &fileInfo);
		if (fileInfo.fname[0] == 0) {
			break;
		}
		dotPointer = (uint8_t*) strrchr(fileInfo.fname, '.');
		if (dotPointer != NULL) {
			if (strcasestr(fileInfo.fname, ".MOD") != NULL
					|| strcasestr(fileInfo.fname, ".S3M") != NULL) {
				strncpy((char*) modFileNames[totalFiles], fileInfo.fname, 13);
				totalFiles++;
			}
		}
	} while (totalFiles < MAX_NUMBER_OF_MODS);

	return totalFiles;
}

uint8_t *getModFileName(uint16_t number) {
	uint8_t *filePtr = NULL;
	if (number <= MAX_NUMBER_OF_MODS) {
		filePtr = modFileNames[number];
	}
	return filePtr;
}

void getModFileNameNew(uint8_t *buffer, uint16_t number) {
	if (number <= MAX_NUMBER_OF_MODS) {
		strncpy((char*) buffer, (const char*) modFileNames[number], 13);
	}
}
