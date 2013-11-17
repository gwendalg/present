/*
 * DVOLUME must be set nonzero in WaveHC2.h to use this example.
 *
 * Adafruit SoftVolumeChange.pde modified to use WaveHC2 library.
 *
 * Play files with software volume control.

 */
#include <WaveUtil.h>
#include <WaveHC2.h>
#include "SPI.h"
#include "Adafruit_WS2801.h"

#define PIXELS_USED 14ULL
#define DATA_PIN 9      // Yellow wire on Adafruit Pixels
#define CLOCK_PIN 8     // Green wire on Adafruit Pixels


SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the volumes root directory
FatReader file;   // This object represent the WAV file
WaveHC2 wave;      // This is the only wave (audio) object, since we will only play one at a time

/*
 * Define macro to put error messages in flash memory
 */
#define error(msg) error_P(PSTR(msg))


// Color VU meter
typedef uint8_t ColorDef[3];
#if 0
#define RED { 0xFF, 0 ,0 }
#define BLUE { 0, 0xFF ,0 }
#define GREEN { 0, 0, 0xFF}
#define YELLOW { 0xFF, 0, 0xFF}
#define ORANGE { 0xFF, 0, 0x80}
#define WHITE { 0xFF, 0xFF, 0xFF }
#define WHITE { 0xC0, 0xC0, 0xC0 }
#endif
#define GREEN 0x0000FF
#define RED 0xFF0000
#define ORANGE 0xFF0080
#define BLACK 0

#define COLOR(a) (((a)[0] << 16) | ((a)[1] << 8) | (a)[2])

//ColorDef pixel_base_color_db_meter[PIXELS_USED] = { GREEN, GREEN, GREEN, GREEN, GREEN, GREEN, GREEN, GREEN, ORANGE, ORANGE, ORANGE, ORANGE, ORANGE, ORANGE, RED, RED, RED, RED};
//ColorDef pixel_base_color_google[PIXELS_USED] = { BLUE, BLUE, BLUE, RED, RED, RED, YELLOW, YELLOW, YELLOW, BLUE, BLUE, BLUE, GREEN, GREEN, GREEN, RED, RED, RED };
//ColorDef pixel_base_color_google[PIXELS_USED] = { WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE};

//uint32_t pixel_base_color_db_meter[PIXELS_USED] = { 0x0000FF, 0xFF0000 };
uint32_t pixel_base_color_db_meter[PIXELS_USED] = { GREEN, GREEN, GREEN, GREEN, GREEN, ORANGE, ORANGE, ORANGE, ORANGE, ORANGE, RED, RED, RED, RED};

Adafruit_WS2801 strip = Adafruit_WS2801(PIXELS_USED, (uint8_t)DATA_PIN, (uint8_t)CLOCK_PIN, WS2801_GRB);


//////////////////////////////////// SETUP
void setup() {
  Serial.begin(9600);
  Serial.println("Wave test!");

  // try card.init(true) if errors occur on V1.0 Wave Shield
  if (!card.init()) {
    error("Card init. failed!");
  }
  // enable optimize read - some cards may timeout
  card.partialBlockRead(true);
  
  if (!vol.init(card)) {
    error("No partition!");
  }
  if (!root.openRoot(vol)) {
    error("Couldn't open root"); return;
  }
  putstring_nl("Files found:");
  root.ls();
  
  strip.begin();
  strip.show();
}

// forward declarition
void play_song_completely(FatReader &file);

//////////////////////////////////// LOOP
void loop() { 
  uint8_t i, r;
  char c, name[15];
  dir_t dir;

  root.rewind();
  // scroll through the files in the directory
  while (root.readDir(dir) > 0) { 
    // only play .WAV files
    if (!strncmp_P((char *)&dir.name[8], PSTR("WAV"), 4)) {
      continue;
    }

    // glow here
    // if a button is pressed
    if (!file.open(vol, dir)){
      putstring("Can't open ");
      printEntryName(dir);
      Serial.println();
      continue;      
    }
    putstring("\n\rPlaying "); 
    printEntryName(dir);
    Serial.println();
    play_song_completely(file);
    file.close();    
  }
}

/////////////////////////////////// HELPERS
/*
 * print error message and halt
 */
void error_P(const char *str) {
  PgmPrint("Error: ");
  SerialPrint_P(str);
  sdErrorCheck();
  while(1);
}
/*
 * print error message and halt if SD I/O error, great for debugging!
 */
void sdErrorCheck(void) {
  if (!card.errorCode()) return;
  PgmPrint("\r\nSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  PgmPrint(", ");
  Serial.println(card.errorData(), HEX);
  while(1);
}
/*
 * Play files with software volume control
 */
void play_song_completely(FatReader &file) {
  int i,level,index;
  if (!wave.create(file)) {
     putstring_nl(" Not a valid WAV"); return;
  }
   // ok time to play!
  wave.play();
  while (wave.isplaying) {
    delay(150);
    // The logical foruma is
    // level = ((uint32_t)wave.Level * PIXELS_USED) >> 16;
    // However, only the 6th to 8th LED would be lit.
    // Alter the formula to expand that range.
    // Do not trust the compiler for compiling constant in.
    // Using 6 * (1 << 16) instead of 0x60000 would not work.
    level = ((((uint32_t)wave.Level * PIXELS_USED) - 0x60000ULL) * PIXELS_USED / 2) >> 16;
    //level = ((wave.Level >> 5) * PIXELS_USED) >> 13;
    for (i = 0; i < PIXELS_USED; i++) {
      if (i <= level) {
        strip.setPixelColor(i, pixel_base_color_db_meter[i]);
      } else {
        strip.setPixelColor(i, BLACK);
      }
    }
    strip.show();
    // Serial.print(char('A' + level));
#if 0
    putstring("Vol: ");
    
    // DVOLUME must be nonzero in WaveHC2.h to use volume.
    Serial.println(wave.volume, DEC);
     
    delay(2000);
    wave.volume++;
    if ( wave.volume == 12) {
      wave.volume = 0;
    }
#endif
  }
  sdErrorCheck();
}
