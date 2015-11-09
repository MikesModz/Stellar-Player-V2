# StellarPlayer V2

A MOD & S3M Player for the Texas Instruments Stellaris® LM4F120 launch pad evaluation kit.

## Update

After having issues with the latest version of Code Composer Studio from TI I decided 
to port this project over to Keil uVision instead. While doing so I took the opportunity to 
tidy up the code a add a few new features.

## Additions

- Auto play next module. If enabled next module will play automatically. To enable auto play 
  place a blank file named "autoplay.dat" in the mods folder of the SD card.
  
Note. Auto play does not work with all modules. Auto play will only work for modules whose order 
index reaches the song length naturally. Some composers may have used JUMPTOORDER in order to 
automatically jump to a specific order and hence never reach the end of the module. In this 
case the module will continually loop.

## Features

- 128 x 160 SPI TFT module (ST7735 based).
- Displays list of .mod and .s3m files found on SD card.
- Next and previous modules can be selected.
- Shows current row, current pattern and total patterns while playing.

##Launch Pad Connections

Connections     | Launch Pad
:----------------|:-----------
| TFT_RESET     | PA7 |
| TFT_COMMAND   | PA6 |
| TFT_MOSI      | PB7	|			
| TFT_MISO      | PB6 |
| TFT_SCK       | PB4 |
| SD_SCK        | PA2 |
| SD_CS         | PA3 |
| SD_MISO       | PA4 |
| SD_MOSI       | PA5 |
| Left channel  | PB0 |
| Right channel | PB1 |

## Programming

Pre built images in both binary and hexadecimal format can be found in the "Builds" folder. Simply 
flash "Stellar_Player.bin" using the LM Flash Programmer.

## Note

Assumes StellarisWare Driverlib is installed in the folder "C:\StellarisWare\".
