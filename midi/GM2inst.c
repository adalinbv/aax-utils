#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct {
  int program;
  int msb, lsb;
  const char *name;
  int low, high;
} _inst_t;

_inst_t inst_table[] = {
 {  1, 0x79, 0x00, "Acoustic Grand Piano", 21, 108 },
 {  1, 0x79, 0x01, "Acoustic Grand Piano (wide)", 21, 108 },
 {  1, 0x79, 0x02, "Acoustic Grand Piano (dark)", 21, 108 },
 {  2, 0x79, 0x00, "Bright Acoustic Piano", 21, 108, },
 {  2, 0x79, 0x01, "Bright Acoustic Piano (wide)", 21, 108 },
 {  3, 0x79, 0x00, "Electric Grand Piano", 21, 108 },
 {  3, 0x79, 0x01, "Electric Grand Piano (wide)", 21, 108 },
 {  4, 0x79, 0x00, "Honky, tonk Piano", 21, 108 },
 {  4, 0x79, 0x01, "Honky, tonk Piano (wide)", 21, 108 },
 {  5, 0x79, 0x00, "Electric Piano 1", 28, 103 },
 {  5, 0x79, 0x01, "Detuned Electric Piano 1", 28, 103 },
 {  5, 0x79, 0x02, "Electric Piano 1 (velocity mix)", 28, 103 },
 {  5, 0x79, 0x03, "60's Electric Piano", 28, 103 },
 {  6, 0x79, 0x00, "Electric Piano 2", 28, 103 },
 {  6, 0x79, 0x01, "Detuned Electric Piano 2", 28, 103 },
 {  6, 0x79, 0x02, "Electric Piano 2 (velocity mix)", 28, 103 },
 {  6, 0x79, 0x03, "EP Legend", 28, 103 },
 {  6, 0x79, 0x04, "EP Phase", 28, 103 },
 {  7, 0x79, 0x00, "Harpsichord", 41, 89 },
 {  7, 0x79, 0x01, "Harpsichord (octave mix)", 41, 89 },
 {  7, 0x79, 0x02, "Harpsichord (wide)", 41, 89 },
 {  7, 0x79, 0x03, "Harpsichord (with key off)", 41, 89 },
 {  8, 0x79, 0x00, "Clavi", 36, 96 },
 {  8, 0x79, 0x01, "Pulse Clavi", 36, 96 },
 {  9, 0x79, 0x00, "Celesta", 60, 108 },
 { 10, 0x79, 0x00, "Glockenspiel", 72, 108 },
 { 11, 0x79, 0x00, "Music Box", 60, 84 },
 { 12, 0x79, 0x00, "Vibraphone", 53, 89 },
 { 12, 0x79, 0x01, "Vibraphone (wide)", 53, 89 },
 { 13, 0x79, 0x00, "Marimba", 48, 84 },
 { 13, 0x79, 0x01, "Marimba (wide)", 48, 84 },
 { 14, 0x79, 0x00, "Xylophone", 65, 96 },
 { 15, 0x79, 0x00, "Tubular Bells", 60, 77 },
 { 15, 0x79, 0x01, "Church Bell", 60, 77 },
 { 15, 0x79, 0x02, "Carillon", 60, 77 },
 { 16, 0x79, 0x00, "Dulcimer", 60, 84 },
 { 17, 0x79, 0x00, "Drawbar Organ", 36, 96 },
 { 17, 0x79, 0x01, "Detuned Drawbar Organ", 36, 96 },
 { 17, 0x79, 0x02, "Italian 60's Organ", 36, 96 },
 { 17, 0x79, 0x03, "Drawbar Organ 2", 36, 96 },
 { 18, 0x79, 0x00, "Percussive Organ", 36, 96 },
 { 18, 0x79, 0x01, "Detuned Percussive Organ", 36, 96 },
 { 18, 0x79, 0x02, "Percussive Organ 2", 36, 96 },
 { 19, 0x79, 0x00, "Rock Organ", 36, 96 },
 { 20, 0x79, 0x00, "Church Organ", 21, 108 },
 { 20, 0x79, 0x01, "Church Organ (octave mix)", 21, 108 },
 { 20, 0x79, 0x02, "Detuned Church Organ", 21, 108 },
 { 21, 0x79, 0x00, "Reed Organ", 36, 96 },
 { 21, 0x79, 0x01, "Puff Organ", 36, 96 },
 { 22, 0x79, 0x00, "Accordion", 53, 89 },
 { 22, 0x79, 0x01, "Accordion 2", 53, 89 },
 { 23, 0x79, 0x00, "Harmonica", 60, 84 },
 { 24, 0x79, 0x00, "Tango Accordion", 53, 89 },
 { 25, 0x79, 0x00, "Acoustic Guitar (nylon)", 40, 84 },
 { 25, 0x79, 0x01, "Ukulele", 40, 84 },
 { 25, 0x79, 0x02, "Acoustic Guitar (nylon + key off)", 40, 84 },
 { 25, 0x79, 0x03, "Acoustic Guitar (nylon 2)", 40, 84 },
 { 26, 0x79, 0x00, "Acoustic Guitar (steel)", 40, 84 },
 { 26, 0x79, 0x01, "12 Strings Guitar", 40, 84 },
 { 26, 0x79, 0x02, "Mandolin", 40, 84 },
 { 26, 0x79, 0x03, "Steel Guitar with Body Sound", 40, 84 },
 { 27, 0x79, 0x00, "Electric Guitar (jazz)", 40, 86 },
 { 27, 0x79, 0x01, "Electric Guitar (pedal steel)", 40, 86 },
 { 28, 0x79, 0x00, "Electric Guitar (clean)", 40, 86 },
 { 28, 0x79, 0x01, "Electric Guitar (detuned clean)", 40, 86 },
 { 28, 0x79, 0x02, "Mid Tone Guitar", 40, 86 },
 { 29, 0x79, 0x00, "Electric Guitar (muted)", 40, 86 },
 { 29, 0x79, 0x01, "Electric Guitar (funky cutting)", 40, 86 },
 { 29, 0x79, 0x02, "Electric Guitar (muted velo, sw)", 40, 86 },
 { 29, 0x79, 0x03, "Jazz Man", 40, 86 },
 { 30, 0x79, 0x00, "Overdriven Guitar", 40, 86 },
 { 30, 0x79, 0x01, "Guitar Pinch", 40, 86 },
 { 31, 0x79, 0x00, "Distortion Guitar", 40, 86 },
 { 31, 0x79, 0x01, "Distortion Guitar (with feedback)", 40, 86 },
 { 31, 0x79, 0x02, "Distorted Rhythm Guitar", 40, 86 },
 { 32, 0x79, 0x00, "Guitar Harmonics", 40, 86 },
 { 32, 0x79, 0x01, "Guitar Feedback", 40, 86 },
 { 33, 0x79, 0x00, "Acoustic Bass", 28, 55 },
 { 34, 0x79, 0x00, "Electric Bass (finger)", 28, 55 },
 { 34, 0x79, 0x01, "Finger Slap Bass", 28, 55 },
 { 35, 0x79, 0x00, "Electric Bass (pick)", 28, 55 },
 { 36, 0x79, 0x00, "Fretless Bass", 28, 55 },
 { 37, 0x79, 0x00, "Slap Bass 1", 28, 55 },
 { 38, 0x79, 0x00, "Slap Bass 2", 28, 55 },
 { 39, 0x79, 0x00, "Synth Bass 1", 28, 55 },
 { 39, 0x79, 0x01, "Synth Bass (warm)", 28, 55 },
 { 39, 0x79, 0x02, "Synth Bass 3 (resonance)", 28, 55 },
 { 39, 0x79, 0x03, "Clavi Bass", 28, 55 },
 { 39, 0x79, 0x04, "Hammer", 28, 55 },
 { 40, 0x79, 0x00, "Synth Bass 2", 28, 55 },
 { 40, 0x79, 0x01, "Synth Bass 4 (attack)", 28, 55 },
 { 40, 0x79, 0x02, "Synth Bass (rubber)", 28, 55 },
 { 40, 0x79, 0x03, "Attack Pulse", 28, 55 },
 { 41, 0x79, 0x00, "Violin", 55, 96 },
 { 41, 0x79, 0x01, "Violin (slow attack)", 55, 96 },
 { 42, 0x79, 0x00, "Viola", 48, 84 },
 { 43, 0x79, 0x00, "Cello", 36, 72 },
 { 44, 0x79, 0x00, "Contrabass", 28, 55 },
 { 45, 0x79, 0x00, "Tremolo Strings", 28, 96 },
 { 46, 0x79, 0x00, "Pizzicato Strings", 28, 96 },
 { 47, 0x79, 0x00, "Orchestral Harp", 23, 103 },
 { 47, 0x79, 0x01, "Yang Chin", 23, 103 },
 { 48, 0x79, 0x00, "Timpani", 36, 57 },
 { 49, 0x79, 0x00, "String Ensembles 1", 28, 96 },
 { 49, 0x79, 0x01, "Strings and Brass", 28, 96 },
 { 49, 0x79, 0x02, "60s Strings", 28, 96 },
 { 50, 0x79, 0x00, "String Ensembles 2", 28, 96 },
 { 51, 0x79, 0x00, "Synth Strings 1", 36, 96 },
 { 51, 0x79, 0x01, "Synth Strings 3", 36, 96 },
 { 52, 0x79, 0x00, "Synth Strings 2", 36, 96 },
 { 53, 0x79, 0x00, "Choir Aahs", 48, 79 },
 { 53, 0x79, 0x01, "Choir Aahs 2", 48, 79 },
 { 54, 0x79, 0x00, "Voice Oohs", 48, 79 },
 { 54, 0x79, 0x01, "Humming", 48, 79 },
 { 55, 0x79, 0x00, "Synth Voice", 48, 84 },
 { 55, 0x79, 0x01, "Analog Voice", 48, 84 },
 { 56, 0x79, 0x00, "Orchestra Hit", 48, 72 },
 { 56, 0x79, 0x01, "Bass Hit Plus", 48, 72 },
 { 56, 0x79, 0x02, "6th Hit", 48, 72 },
 { 56, 0x79, 0x03, "Euro Hit", 48, 72 },
 { 57, 0x79, 0x00, "Trumpet", 58, 94 },
 { 57, 0x79, 0x01, "Dark Trumpet Soft", 58, 94 },
 { 58, 0x79, 0x00, "Trombone", 34, 75 },
 { 58, 0x79, 0x01, "Trombone 2", 34, 75 },
 { 58, 0x79, 0x02, "Bright Trombone", 34, 75 },
 { 59, 0x79, 0x00, "Tuba", 29, 55 },
 { 60, 0x79, 0x00, "Muted Trumpet", 58, 82 },
 { 60, 0x79, 0x01, "Muted Trumpet 2", 58, 82 },
 { 61, 0x79, 0x00, "French Horn", 41, 77 },
 { 61, 0x79, 0x01, "French Horn 2 (warm)", 41, 77 },
 { 62, 0x79, 0x00, "Brass Section", 36, 96 },
 { 62, 0x79, 0x01, "Brass Section 2 (octave mix)", 36, 96 },
 { 63, 0x79, 0x00, "Synth Brass 1", 36, 96 },
 { 63, 0x79, 0x01, "Synth Brass 3", 36, 96 },
 { 63, 0x79, 0x02, "Analog Synth Brass 1", 36, 96 },
 { 63, 0x79, 0x03, "Jump Brass", 36, 96 },
 { 64, 0x79, 0x00, "Synth Brass 2", 36, 96 },
 { 64, 0x79, 0x01, "Synth Brass 4", 36, 96 },
 { 64, 0x79, 0x02, "Analog Synth Brass 2", 36, 96 },
 { 65, 0x79, 0x00, "Soprano Sax", 54, 87 },
 { 66, 0x79, 0x00, "Alto Sax", 49, 80 },
 { 67, 0x79, 0x00, "Tenor Sax", 42, 75 },
 { 68, 0x79, 0x00, "Baritone Sax", 37, 68 },
 { 69, 0x79, 0x00, "Oboe", 58, 91 },
 { 70, 0x79, 0x00, "English Horn", 52, 81 },
 { 71, 0x79, 0x00, "Bassoon", 34, 72 },
 { 72, 0x79, 0x00, "Clarinet", 50, 91 },
 { 73, 0x79, 0x00, "Piccolo", 74, 108 },
 { 74, 0x79, 0x00, "Flute", 60, 96 },
 { 75, 0x79, 0x00, "Recorder", 60, 96 },
 { 76, 0x79, 0x00, "Pan Flute", 60, 96 },
 { 77, 0x79, 0x00, "Blown Bottle", 60, 96 },
 { 78, 0x79, 0x00, "Shakuhachi", 55, 84 },
 { 79, 0x79, 0x00, "Whistle", 60, 96 },
 { 80, 0x79, 0x00, "Ocarina", 60, 84 },
 { 81, 0x79, 0x00, "Lead 1 (square)", 21, 108 },
 { 81, 0x79, 0x01, "Lead 1a (square 2)", 21, 108 },
 { 81, 0x79, 0x02, "Lead 1b (sine)", 21, 108 },
 { 82, 0x79, 0x00, "Lead 2 (sawtooth)", 21, 108 },
 { 82, 0x79, 0x01, "Lead 2a (sawtooth 2)", 21, 108 },
 { 82, 0x79, 0x02, "Lead 2b (saw + pulse)", 21, 108 },
 { 82, 0x79, 0x03, "Lead 2c (double sawtooth)", 21, 108 },
 { 82, 0x79, 0x04, "Lead 2d (sequenced analog)", 21, 108 },
 { 83, 0x79, 0x00, "Lead 3 (calliope)", 36, 96 },
 { 84, 0x79, 0x00, "Lead 4 (chiff)", 36, 96 },
 { 85, 0x79, 0x00, "Lead 5 (charang)", 36, 96 },
 { 85, 0x79, 0x01, "Lead 5a (wire lead)", 36, 96 },
 { 86, 0x79, 0x00, "Lead 6 (voice)", 36, 96 },
 { 87, 0x79, 0x00, "Lead 7 (fifths)", 36, 96 },
 { 88, 0x79, 0x00, "Lead 8 (bass + lead)", 21, 108 },
 { 88, 0x79, 0x01, "Lead 8a (soft wire lead)", 21, 108 },
 { 89, 0x79, 0x00, "Pad 1 (new age)", 36, 96 },
 { 90, 0x79, 0x00, "Pad 2 (warm)", 36, 96 },
 { 90, 0x79, 0x01, "Pad 2a (sine pad)", 36, 96 },
 { 91, 0x79, 0x00, "Pad 3 (polysynth)", 36, 96 },
 { 92, 0x79, 0x00, "Pad 4 (choir)", 36, 96 },
 { 92, 0x79, 0x01, "Pad 4a (itopia)", 36, 96 },
 { 93, 0x79, 0x00, "Pad 5 (bowed)", 36, 96 },
 { 94, 0x79, 0x00, "Pad 6 (metallic)", 36, 96 },
 { 95, 0x79, 0x00, "Pad 7 (halo)", 36, 96 },
 { 96, 0x79, 0x00, "Pad 8 (sweep)", 36, 96 },
 { 97, 0x79, 0x00, "FX 1 (rain)", 36, 96 },
 { 98, 0x79, 0x00, "FX 2 (soundtrack)", 36, 96 },
 { 99, 0x79, 0x00, "FX 3 (crystal)", 36, 96 },
 { 99, 0x79, 0x01, "FX 3a (synth mallet)", 36, 96 },
 { 100, 0x79, 0x00, "FX 4 (atmosphere)", 36, 96 },
 { 101, 0x79, 0x00, "FX 5 (brightness)", 36, 96 },
 { 102, 0x79, 0x00, "FX 6 (goblins)", 36, 96 },
 { 103, 0x79, 0x00, "FX 7 (echoes)", 36, 96 },
 { 103, 0x79, 0x01, "FX 7a (echo bell)", 36, 96 },
 { 103, 0x79, 0x02, "FX 7b (echo pan)", 36, 96 },
 { 104, 0x79, 0x00, "FX 8 (sci, fi)", 36, 96 },
 { 105, 0x79, 0x00, "Sitar", 48, 77 },
 { 105, 0x79, 0x01, "Sitar 2 (bend)", 48, 77 },
 { 106, 0x79, 0x00, "Banjo", 48, 84 },
 { 107, 0x79, 0x00, "Shamisen", 50, 79 },
 { 108, 0x79, 0x00, "Koto", 55, 84 },
 { 108, 0x79, 0x01, "Taisho Koto", 55, 84 },
 { 109, 0x79, 0x00, "Kalimba", 48, 79 },
 { 110, 0x79, 0x00, "Bag pipe", 36, 77 },
 { 111, 0x79, 0x00, "Fiddle", 55, 96 },
 { 112, 0x79, 0x00, "Shanai", 48, 72 },
 { 113, 0x79, 0x00, "Tinkle Bell", 72, 84 },
 { 114, 0x79, 0x00, "Agogo", 60, 72 },
 { 115, 0x79, 0x00, "Steel Drums", 52, 76 },
 { 116, 0x79, 0x00, "Woodblock", 60, 60 },
 { 116, 0x79, 0x01, "Castanets", 60, 60 },
 { 117, 0x79, 0x00, "Taiko Drum", 60, 60 },
 { 117, 0x79, 0x01, "Concert Bass Drum", 60, 60 },
 { 118, 0x79, 0x00, "Melodic Tom", 60, 60 },
 { 118, 0x79, 0x01, "Melodic Tom 2 (power)", 60, 60 },
 { 119, 0x79, 0x00, "Synth Drum", 60, 60 },
 { 119, 0x79, 0x01, "Rhythm Box Tom", 60, 60 },
 { 119, 0x79, 0x02, "Electric Drum", 60, 60 },
 { 120, 0x79, 0x00, "Reverse Cymbal", 60, 60 },
 { 121, 0x79, 0x00, "Guitar Fret Noise", 60, 60 },
 { 121, 0x79, 0x01, "Guitar Cutting Noise", 60, 60 },
 { 121, 0x79, 0x02, "Acoustic Bass String Slap", 60, 60 },
 { 122, 0x79, 0x00, "Breath Noise", 60, 60 },
 { 122, 0x79, 0x01, "Flute Key Click", 60, 60 },
 { 123, 0x79, 0x00, "Seashore", 60, 60 },
 { 123, 0x79, 0x01, "Rain", 60, 60 },
 { 123, 0x79, 0x02, "Thunder", 60, 60 },
 { 123, 0x79, 0x03, "Wind", 60, 60 },
 { 123, 0x79, 0x04, "Stream", 60, 60 },
 { 123, 0x79, 0x05, "Bubble", 60, 60 },
 { 124, 0x79, 0x00, "Bird Tweet", 60, 60 },
 { 124, 0x79, 0x01, "Dog", 60, 60 },
 { 124, 0x79, 0x02, "Horse Gallop", 60, 60 },
 { 124, 0x79, 0x03, "Bird Tweet 2", 60, 60 },
 { 125, 0x79, 0x00, "Telephone Ring", 60, 60 },
 { 125, 0x79, 0x01, "Telephone Ring 2", 60, 60 },
 { 125, 0x79, 0x02, "Door Creaking", 60, 60 },
 { 125, 0x79, 0x03, "Door", 60, 60 },
 { 125, 0x79, 0x04, "Scratch", 60, 60 },
 { 125, 0x79, 0x05, "Wind Chime", 60, 60 },
 { 126, 0x79, 0x00, "Helicopter", 60, 60 },
 { 126, 0x79, 0x01, "Car Engine", 60, 60 },
 { 126, 0x79, 0x02, "Car Stop", 60, 60 },
 { 126, 0x79, 0x03, "Car Pass", 60, 60 },
 { 126, 0x79, 0x04, "Car Crash", 60, 60 },
 { 126, 0x79, 0x05, "Siren", 60, 60 },
 { 126, 0x79, 0x06, "Train", 60, 60 },
 { 126, 0x79, 0x07, "Jetplane", 60, 60 },
 { 126, 0x79, 0x08, "Starship", 60, 60 },
 { 126, 0x79, 0x09, "Burst Noise", 60, 60 },
 { 127, 0x79, 0x00, "Applause", 60, 60 },
 { 127, 0x79, 0x01, "Laughing", 60, 60 },
 { 127, 0x79, 0x02, "Screaming", 60, 60 },
 { 127, 0x79, 0x03, "Punch", 60, 60 },
 { 127, 0x79, 0x04, "Heart Beat", 60, 60 },
 { 127, 0x79, 0x05, "Footsteps", 60, 60 },
 { 128, 0x79, 0x00, "Gunshot", 60, 60 },
 { 128, 0x79, 0x01, "Machine Gun", 60, 60 },
 { 128, 0x79, 0x02, "Lasergun", 60, 60 },
 { 128, 0x79, 0x03, "Explosion", 60, 60 },
 { 0, 0, 0, "", 0, 0}
};


static char filename[256];
char *lowercase(const char *name)
{
   int i, j ,len = strlen(name);
   if (len > 256) len = 255;

   j = 1;
   filename[0] = tolower(name[0]);
   for (i=1; i<len; ++i)
   {
      if (name[i] != '(' && name[i] != ')')
      {
         if (name[i] == ' ') filename[j] = '-';
         else filename[j] = tolower(name[i]);
         j++;
      }
   }
   filename[j] = 0;
   return filename;
}

int main()
{
   int b;
   for (b=0; b<10; ++b)
   {
      char f = 0;
      int i = 0;
      do
      {
         if (!f)
         {  
            printf("  <bank n=\"0x79\" l=\"%i\">\n", b);
            f = 1;
         }

         if (inst_table[i].msb == 0x79 && inst_table[i].lsb == b )
         {
            printf("   <instrument n=\"%i\" name=\"%s\"",
                            inst_table[i].program-1, inst_table[i].name);

            printf(" file=\"instruments/%s\"", lowercase(inst_table[i].name));

            if (strstr(inst_table[i].name, "(wide)")) {
               printf(" wide=\"true\"");
            }
            printf("/>\n");
         }
      } while (inst_table[++i].program);
      if (f) printf("  </bank>\n\n");
   }

   return 0;
}
   
