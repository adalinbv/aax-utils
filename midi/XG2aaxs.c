#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>

static float XGMIDI_LFO_frequency_table[128] = {	// Hz
 0.f, 0.08f, 0.08f, 0.16f, 0.16f, 0.25f, 0.25f, 0.33f, 0.33f, 0.42f, 0.42f,
 0.5f, 0.5f, 0.58f, 0.58f, 0.67f, 0.67f, 0.75f, 0.75f, 0.84, 0.84f, 0.92f,
 0.92f, 1.f, 1.f, 1.09f, 1.09f, 1.17f, 1.17f, 1.26f, 1.26f, 1.34f, 1.34f,
 1.43f, 1.43f, 1.51f, 1.51f, 1.59f, 1.59f, 1.68f, 1.68f, 1.76f, 1.76f, 1.85f,
 1.85f, 1.93f, 1.93f, 2.01f, 2.01f, 2.1f, 2.1f, 2.18f, 2.18f, 2.27f, 2.27f,
 2.35f, 2.35f, 2.43f, 2.43f, 2.52f, 2.52f, 2.6f, 2.6f, 2.69f, 2.69f, 2.77f,
 2.86f, 2.94f, 3.02f, 3.11f, 3.19f, 3.28f, 3.36f, 3.44f, 3.53f, 3.61f, 3.7f,
 3.86f, 4.03f, 4.2f, 4.37f, 4.54f, 4.71f, 4.87f, 5.04f, 5.21f, 5.38f, 5.55f,
 5.72f, 6.05f, 6.39f, 6.72f, 7.06f, 7.4f, 7.73f, 8.07f, 8.1f, 8.74f, 9.08f,
 9.42f, 8.75f, 10.f, 11.4f, 12.1f, 12.7f, 13.4f, 14.1f, 14.8f, 15.4f, 16.1f,
 16.8f, 17.4f, 18.1f, 19.5f, 20.8f, 22.2f, 23.5f, 24.8f, 26.2f, 27.5f, 28.9f,
 30.2f, 31.6f, 32.9f, 34.3f, 37.f, 39.7f
};

static float XGMIDI_delay_offset_table[128] = {
 0.f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.f, 1.1f, 1.2f,
 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f, 2.f, 2.1f, 2.2f, 2.3f, 2.4f, 2.5f,
 2.6f, 2.7f, 2.8f, 2.9f, 3.f, 3.1f, 3.2f, 3.3f, 3.4f, 3.5f, 3.6f, 3.7f, 3.8f,
 3.9f, 4.f, 4.1f, 4.2f, 4.3f, 4.4f, 4.5f, 4.6f, 4.7f, 4.8f, 4.9f, 5.f, 5.1f,
 5.2f, 5.3f, 5.4f, 5.5f, 5.6f, 5.7f, 5.8f, 5.9f, 6.f, 6.1f, 6.2f, 6.3f, 6.4f,
 6.5f, 6.6f, 6.7f, 6.8f, 6.9f, 7.f, 7.1f, 7.2f, 7.3f, 7.4f, 7.5f, 7.6f, 7.7f,
 7.8f, 7.9f, 8.f, 8.1f, 8.2f, 8.3f, 8.4f, 8.5f, 8.6f, 8.7f, 8.8f, 8.9f, 9.f,
 9.1f, 9.2f, 9.3f, 9.4f, 9.5f, 9.6f, 9.7f, 9.8f, 9.9f, 10.f, 11.1f, 12.2f,
 13.3f, 14.4f, 15.5f, 17.1f, 18.6f, 20.2f, 21.8f, 23.3f, 24.9f, 26.5f, 28.f,
 29.6f, 31.2f, 32.8f, 34.3f, 35.9f, 37.f, 39.f, 40.6f, 42.2f, 43.7f, 45.3f,
 46.9f, 48.4f, 50.f
};

static float XGMIDI_delay_time_table[128] = {	// ms
 0.1f, 1.7f, 3.2f, 4.8f, 6.4f, 8.0f, 9.5f, 11.1f, 12.7f, 14.3f, 15.8f, 17.4f,
 19.f, 2.6f, 22.1f, 23.7f, 25.3f, 26.9f, 28.4f, 30.f, 31.6f, 33.2f, 34.7f,
 36.3f, 37.9f, 39.5f, 41.f, 42.6f, 44.2f, 45.7f, 47.3f, 48.9f, 50.5f, 52.f,
 53.6f, 55.2f, 56.8f, 58.3f, 59.9f, 61.5f, 63.1f, 64.4f, 66.2f, 67.8f, 69.4f,
 70.9f, 72.5f, 74.1f, 75.7f, 77.2f, 78.8f, 80.4f, 81.9f, 83.5f, 85.1f, 86.7f,
 88.2f, 89.8f, 91.4f, 93.f, 94.5f, 96.1f, 97.7f, 99.3f, 100.8f, 102.4f, 104.f,
 105.6f, 107.1f, 108.7f, 110.3f, 111.9f, 113.4f, 115.f, 116.6f, 118.2f, 119.7f,
 121.3f, 122.9f, 124.4f, 126.f, 127.6f, 129.2f, 130.7f, 132.3f, 133.9f, 135.5f,
 137.f, 138.6f, 1402.f, 141.8f, 143.3f, 144.9f, 146.5f, 148.1f, 149.6f, 151.2f,
 152.8f, 154.4f, 155.9f, 157.5f, 159.1f, 160.6f, 162.2f, 163.8f, 165.4f,
 166.9f, 168.5f, 170.1f, 171.7f, 173.2f, 174.8f, 176.4f, 178.f, 179.5f, 181.1f,
 182.7f, 184.3f, 185.8f, 187.4f, 189.f, 190.6f, 192.1f, 193.7f, 195.3f, 169.6f,
 198.4f, 200.f
};

static float XGMIDI_EQ_frequency_table[61] = {
 20.f, 22.f, 25.f, 28.f, 32.f, 36.f, 40.f, 45.f, 50.f, 56.f, 63.f, 70.f, 80.f,
 90.f, 100.f, 110.f, 125.f, 140.f, 160.f, 180.f, 200.f, 225.f, 250.f, 280.f,
 315.f, 355.f, 400.f, 450.f, 500.f, 560.f, 630.f, 700.f, 800.f, 900.f, 1000.f,
 1100.f, 1200.f, 1400.f, 1600.f, 1800.f, 2000.f, 2200.f, 2500.f, 2800.f,
 3200.f, 3600.f, 4000.f, 4500.f, 5000.f, 5600.f, 6300.f, 7000.0f, 8000.f,
 9000.f, 10000.f, 11000.0f, 12000.f, 14000.f, 16000.f, 18000.f, 20000.f
};

static float XGMIDI_room_size_table[45] = {
 0.1f, 0.3f, 0.4f, 0.6f, .7f, 0.9f, 1.f, 1.2f, 1.4f, 1.5f, 1.7f, 1.8f, 2.f,
 2.1f, 2.3f, 2.5f, 2.6f, 2.8f, 2.9f, 3.1f, 4.2f, 3.4f, 3.5f, 3.7f, 3.9f, 4.f,
 4.2f, 4.3f, 4.5f, 4.6f, 4.8f, 5.f, 5.1f, 5.3f, 5.4f, 5.6f, 5.7f, 5.9f, 6.1f,
 6.2f, 6.4f, 6.5f, 6.7f, 6.8f, 7.f
};

static float XGMIDI_compressor_release_time_table[16] = {
 10.f, 15.f, 25.f, 35.f, 45.f, 55.f, 65.f, 75.f, 85.f, 100.f, 115.f, 140.f,
 170.f, 230.f, 340.f, 680.f
};

static float XGMIDI_compressor_ratio_table[8] = {
 1.f, 1.5f, 2.f, 3.f, 5.f, 7.f, 10.f, 20.0f
};

static float XGMIDI_reverb_time[70] = {
 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f,
 1.6f, 1.7f, 1.8f, 1.9f, 2.f, 2.1f, 2.2f, 2.3f, 2.4f, 2.5f, 2.6f, 2.7f, 2.8f,
 2.9f, 3.f, 3.1f, 3.2f, 3.3f, 3.4f, 3.5f, 3.6f, 3.7f, 3.8f, 3.9f, 4.f, 4.1f,
 4.2f, 4.3f, 4.4f, 4.5f, 4.6f, 4.7f, 4.8f, 4.9f, 5.f, 5.5f, 6.f, 6.5f, 7.f,
 7.5f, 8.f, 8.5f, 9.f, 9.5f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f, 17.f,
 18.f, 19.f, 20.f, 25.f, 30.f
};

static float XGMIDI_reverb_dimensions[105] = {
 0.5f, 0.8f, 1.f, 1.3f, 1.5f, 1.8f, 2.f, 2.3f, 2.6f, 2.8f, 3.1f, 3.3f, 3.6f,
 3.9f, 4.1f, 4.4f, 4.6f, 4.9f, 5.2f, 5.4f, 5.7f, 5.9f, 6.2f, 6.5f, 6.7f, 7.f,
 7.2f, 7.5f, 7.8f, 8.f, 8.3f, 8.6f, 8.8f, 9.1f, 9.4f, 9.6f, 9.9f, 10.2f, 10.4f,
 10.7f, 11.f, 11.2f, 11.5f, 11.8f, 12.1f, 12.3f, 12.6f, 12.9f, 13.1f, 3.4f,
 14.f, 14.2f, 14.5f, 14.8f, 15.1f, 15.4f, 15.6f, 15.9f, 16.2f, 16.5f, 16.8f,
 17.1f, 17.3f, 17.6f, 17.9f, 18.2f, 18.5f, 18.8f, 19.1f, 19.4f, 19.7f, 2.0f,
 20.5f, 20.8f, 21.1f, 22.f, 22.4f, 22.7f, 23.f, 23.3f, 23.6f, 23.9f, 24.2f,
 24.5f, 24.9f, 25.2f, 25.5f, 25.8f, 27.1f, 27.5f, 27.8f, 28.1f, 28.5f, 28.8f,
 29.2f, 29.5f, 29.9f, 30.2f
};

typedef struct {
    const char* name;
    int param[16];
} XGMIDI_effect_t;

/* REVERB
 *  p	description	range 
 *  --	-----------	---------
 *  0	Reverb Time	0.3 ~ 30.0 sec
 *  1	Diffusiion	0 ~ 10
 *  2	Initial Delay	0 ~ 63
 *  3	HPF Cutoff	Thru ~ 8kHz
 *  4	LPF Cutoff	1kHz ~ Thru
 *  5	Width		0.5 ~ 10.2 meter
 *  6	Height		0.5 ~ 20.2 meter
 *  7	Depth		0.5 ~ 30.2 meter
 *  8	Wall Vary	0 ~ 30
 *  9	Dry/Wet		D63>W ~ D=W ~ D<W63
 * 10	Rev Delay	0 ~ 63
 * 11	Density		0 ~ 4
 * 12	Rev/Er Balance	R<E63 ~ R=E ~ R63>E
 * 13	High Damp	0.1 ~ 1.0
 * 14	Feedback Level	-63 ~ +63
 */
#define XGMIDI_MAX_REVERB_TYPES         12
static XGMIDI_effect_t XGMIDI_reverb_types[XGMIDI_MAX_REVERB_TYPES] = {
//      param:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
 { "hall1",     { 18, 10,  8, 13, 49,  0,  0,  0,  0, 40,  0,  4, 50,  8, 64,  0 } },
 { "hall2",     { 25, 10, 28,  6, 46,  0,  0,  0,  0, 40, 13,  3, 74,  7, 64,  0 } },
 { "room1",     {  5, 10, 16,  4, 49,  0,  0,  0,  0, 40,  5,  3, 64,  8, 64,  0 } },
 { "room2",     { 12, 10,  5,  4, 38,  0,  0,  0,  0, 40,  0,  4, 50,  8, 64,  0 } },
 { "room3",     {  9, 10, 47,  5, 36,  0,  0,  0,  0, 40,  0,  4, 60,  8, 64,  0 } },
 { "stage1",    { 19, 10, 16,  7, 54,  0,  0,  0,  0, 40,  0,  3, 64,  6, 64,  0 } },
 { "stage2",    { 11, 10, 16,  7, 51,  0,  0,  0,  0, 40,  2,  2, 64,  6, 64,  0 } },
 { "plate",     { 25, 10,  6,  8, 49,  0,  0,  0,  0, 40,  2,  3, 64,  5, 64,  0 } },
 { "whiteroom", {  9,  5, 11,  0, 46, 30, 50, 70,  7, 40, 34,  4, 64,  7, 64,  0 } },
 { "tunnel",    { 48,  6, 19,  0, 44, 33, 52, 70, 16, 40, 20,  4, 64,  7, 64,  0 } },
 { "canyon",    { 59,  6, 63,  0, 45, 34, 62, 91, 13, 40, 25,  4, 64,  4, 64,  0 } },
 { "basement",  {  3,  6,  3,  0, 34, 26, 29, 59, 15, 40, 32,  4, 64,  8, 64,  0 } }
};

#define XGMIDI_MAX_CHORUS_TYPES         13
static XGMIDI_effect_t XGMIDI_chorus_types[XGMIDI_MAX_CHORUS_TYPES] = {
 { "chorus1",  {  6,  54,  77, 106, 0, 28, 64, 46, 64,  64, 46, 64, 10, 0, 0, 0 } },
 { "chorus2",  {  8,  63,  64,  30, 0, 28, 62, 42, 58,  64, 46, 64, 10, 0, 0, 0 } },
 { "chorus3",  {  4,  44,  64, 110, 0, 28, 64, 46, 66,  64, 46, 64, 10, 0, 0, 0 } },
 { "chorus4",  {  9,  32,  69, 104, 0, 28, 64, 46, 64,  64, 46, 64, 10, 0, 1, 0 } },
 { "celeste1", { 12,  32,  64,   0, 0, 28, 64, 46, 64, 127, 40, 68, 10, 0, 0, 0 } },
 { "celeste2", { 28,  18,  90,   2, 0, 28, 62, 42, 60,  84, 40, 68, 10, 0, 0, 0 } },
 { "celeste3", {  4,  63,  44,   2, 0, 28, 64, 46, 68, 127, 40, 68, 10, 0, 0, 0 } },
 { "celeste4", {  8,  29,  64,   0, 0, 28, 64, 51, 66, 127, 40, 68, 10, 0, 1, 0 } },
 { "flanger1", { 14,  14, 104,   2, 0, 28, 64, 46, 64,  96, 40, 64, 10, 4, 0, 0 } },
 { "flanger2", { 32,  17,  26,   2, 0, 28, 64, 46, 60,  96, 40, 64, 10, 4, 0, 0 } },
 { "flanger3", {  4, 109, 109,   2, 0, 28, 64, 46, 64, 127, 40, 64, 10, 4, 0, 0 } },
  { "symphonic",{ 12,  25,  16,   0, 0, 28, 64, 46, 64, 127, 46, 64, 10, 0, 0, 0 } },
 { "rotary-speaker" , { 81, 35, 0, 0, 0, 24, 60, 45, 54, 127, 33, 52, 30, 0, 0, 0 } }
};

#define XGMIDI_MAX_PHASING_TYPES        2
static XGMIDI_effect_t XGMIDI_phasing_types[XGMIDI_MAX_PHASING_TYPES] = {
 { "phaser1",  {  8, 111,  74, 104, 0, 28, 64, 46, 64,  64,  6,  1, 64, 0, 0, 0 } },
 { "phaser2",  {  8, 111,  74, 104, 0, 28, 64, 46, 64,  64,  5,  1,  4, 0, 0, 0 } }
};

#define XGMIDI_MAX_DISTORTION_TYPES     2
static XGMIDI_effect_t XGMIDI_distortion_types[XGMIDI_MAX_DISTORTION_TYPES] = {
 { "distortion", { 40, 20, 72, 53, 48, 0, 43, 74, 10, 127, 120, 0, 0, 0, 0, 0 } },
 { "overdrive",  { 29, 24, 68, 45, 55, 0, 41, 72, 10, 127, 104, 0, 0, 0, 0, 0 } }
};

#define XGMIDI_MAX_EQ_TYPES             2
static XGMIDI_effect_t XGMIDI_EQ_types[XGMIDI_MAX_EQ_TYPES] = {
 { "3-band-eq", { 70, 34, 60, 10, 70, 28, 46, 0, 0, 127,  0,  0, 0,  0,  0,  0 } },
 { "2-band eq", { 28, 70, 46, 70,  0,  0,  0, 0, 0, 127, 34, 64, 10, 0,  0,  0 } }
};

#define LEVEL_60DB		0.0001f
#define MAX_REVERB_EFFECTS_TIME	0.210f

#define _MAX(a,b)       (((a)>(b)) ? (a) : (b))
#define _MIN(a,b)       (((a)<(b)) ? (a) : (b))
#define _MINMAX(a,b,c)  (((a)>(c)) ? (c) : (((a)<(b)) ? (b) : (a)))

float _lin2log(float v) { return log10f(v); }
float _log2lin(float v) { return powf(10.0f,v); }
float _lin2db(float v) { return 20.0f*log10f(v); }
float _db2lin(float v) { return _MINMAX(powf(10.0f,v/20.0f),0.0f,10.0f); }

int write_reverb()
{
   for (int i=0; i<XGMIDI_MAX_REVERB_TYPES; ++i)
   {
      char fname[256];

      XGMIDI_effect_t *type = &XGMIDI_reverb_types[i];

      snprintf(fname, 255, "%s.aaxs", type->name);
      printf("Generating reverb: %s\n", fname);

      FILE *stream = fopen(fname, "w+");
      if (stream)
      {
         int rt = type->param[0];	// Reverb Time
         int df = type->param[1];	// Diffusion
         int id = type->param[2];	// Initial Delay
         int fc = type->param[4];	// LPF Cutoff
//       int ds = type->param[11];	// Density
         int hd = type->param[13];	// High Damp

         /* room dimansions */
         int size = type->param[5];
         float width = XGMIDI_reverb_dimensions[size];

         size = type->param[6];
         float height = XGMIDI_reverb_dimensions[size];

         size = type->param[7];
         float depth = XGMIDI_reverb_dimensions[size];

         float density = 0.0f; // XGMIDI_reverb_types[i].param[9]/63.0f;

         /* basic parameters */
         float cutoff_freq = XGMIDI_EQ_frequency_table[fc];
         cutoff_freq = (1.1f - 0.1f*hd) * cutoff_freq;

         float delay_depth = XGMIDI_delay_time_table[id]*1e-3f;

         float reverb_time = XGMIDI_reverb_time[rt];

         float diffusion = 0.1f*MAX_REVERB_EFFECTS_TIME*df;
         float decay_level = powf(LEVEL_60DB, 0.5f*diffusion/reverb_time);

         fprintf(stream, "<?xml version=\"1.0\"?>\n\n");
         fprintf(stream, "<aeonwave>\n\n");
         fprintf(stream, " <audioframe>\n");
         fprintf(stream, "  <effect type=\"reverb\" src=\"inverse\">\n");
         fprintf(stream, "   <slot n=\"0\">\n");
         fprintf(stream, "    <param n=\"0\">%5.1f</param>\n", cutoff_freq);
         fprintf(stream, "    <param n=\"1\">%5.4f</param>\n", delay_depth);
         fprintf(stream, "    <param n=\"2\">%4.3f</param>\n", decay_level);
         fprintf(stream, "    <param n=\"3\">%4.3f</param>\n", diffusion);
         fprintf(stream, "   </slot>\n");
         fprintf(stream, "   <slot n=\"1\">\n");
         fprintf(stream, "    <param n=\"0\">%3.1f</param>\n", width);
         fprintf(stream, "    <param n=\"1\">%3.1f</param>\n", height);
         fprintf(stream, "    <param n=\"2\">%3.1f</param>\n", depth);
         fprintf(stream, "    <param n=\"3\">%4.3f</param>\n", density);
         fprintf(stream, "   </slot>\n");
         fprintf(stream, "  </effect>\n");
         fprintf(stream, " </audioframe>\n\n");

         fprintf(stream, " <mixer>\n");
         fprintf(stream, "  <effect type=\"reverb\" src=\"inverse\">\n");
         fprintf(stream, "   <slot n=\"0\">\n");
         fprintf(stream, "    <param n=\"0\">%5.1f</param>\n", cutoff_freq);
         fprintf(stream, "    <param n=\"1\">%5.4f</param>\n", delay_depth);
         fprintf(stream, "    <param n=\"2\">%4.3f</param>\n", decay_level);
         fprintf(stream, "    <param n=\"3\">%4.3f</param>\n", diffusion);
         fprintf(stream, "   </slot>\n");
         fprintf(stream, "   <slot n=\"1\">\n");
         fprintf(stream, "    <param n=\"0\">%3.1f</param>\n", width);
         fprintf(stream, "    <param n=\"1\">%3.1f</param>\n", height);
         fprintf(stream, "    <param n=\"2\">%3.1f</param>\n", depth);
         fprintf(stream, "    <param n=\"3\">%4.3f</param>\n", density);
         fprintf(stream, "   </slot>\n");
         fprintf(stream, "  </effect>\n");
         fprintf(stream, " </mixer>\n\n");
         fprintf(stream, "</aeonwave>\n");
         fclose(stream);
      }
      else printf(" Failed to open for writing: %s\n", strerror(errno));
   }

   return 0;
}

int write_chorus()
{
   for (int i=0; i<XGMIDI_MAX_CHORUS_TYPES; ++i)
   {
      char fname[256];

      XGMIDI_effect_t *type = &XGMIDI_chorus_types[i];

      snprintf(fname, 255, "%s.aaxs", type->name);
      printf("Generating chorus: %s\n", fname);

      FILE *stream = fopen(fname, "w+");
      if (stream)
      {
         int lf = type->param[0];	// LFO Frequency
         int pd = type->param[1];	// LFO Phase Modulation Depth
         int fb = type->param[2];	// Feedback Level
         int dt = type->param[3];	// Delay Offset
         int fl = type->param[5];	// EQ Low Frequency
         int fh = type->param[7];	// EQ High frequency
         int dw = type->param[9];	// Dry/Wet

         float lfo_frequency = XGMIDI_LFO_frequency_table[lf];
         float lfo_offset = XGMIDI_delay_offset_table[dt]/50.0f;
         float lfo_depth = pd/127.0f;
         float gain = (dw - 1)/127.0f;

         float cutoff_freq_low = XGMIDI_EQ_frequency_table[fl];
         float cutoff_freq_high = XGMIDI_EQ_frequency_table[fh];
         float feedback = _MAX(fb - 64, 0)/127.0f;

         fprintf(stream, "<?xml version=\"1.0\"?>\n\n");
         fprintf(stream, "<aeonwave>\n\n");
         fprintf(stream, " <audioframe>\n");
         fprintf(stream, "  <effect type=\"chorus\" src=\"sine\">\n");
         fprintf(stream, "   <slot n=\"0\">\n");
         fprintf(stream, "    <param n=\"0\">%5.3f</param>\n", gain);
         fprintf(stream, "    <param n=\"1\">%5.3f</param>\n", lfo_frequency);
         fprintf(stream, "    <param n=\"2\">%5.3f</param>\n", lfo_depth);
         fprintf(stream, "    <param n=\"3\">%5.3f</param>\n", lfo_offset);
         fprintf(stream, "   </slot>\n");
         fprintf(stream, "   <slot n=\"1\">\n");
         fprintf(stream, "    <param n=\"0\">%5.1f</param>\n", cutoff_freq_low);
         fprintf(stream, "    <param n=\"1\">%5.1f</param>\n", cutoff_freq_high);
         fprintf(stream, "    <param n=\"2\">%5.3f</param>\n", feedback);
         fprintf(stream, "    <param n=\"3\">%2.1f</param>\n", 1.0f);
         fprintf(stream, "   </slot>\n");
         fprintf(stream, "  </effect>\n");
         fprintf(stream, " </audioframe>\n\n");

         fprintf(stream, " <mixer>\n");
         fprintf(stream, "  <effect type=\"chorus\" src=\"sine\">\n");
         fprintf(stream, "   <slot n=\"0\">\n");
         fprintf(stream, "    <param n=\"0\">%5.3f</param>\n", gain);
         fprintf(stream, "    <param n=\"1\">%5.3f</param>\n", lfo_frequency);
         fprintf(stream, "    <param n=\"2\">%5.3f</param>\n", lfo_depth);
         fprintf(stream, "    <param n=\"3\">%5.3f</param>\n", lfo_offset);
         fprintf(stream, "   </slot>\n");
         fprintf(stream, "   <slot n=\"1\">\n");
         fprintf(stream, "    <param n=\"0\">%5.1f</param>\n", cutoff_freq_low);
         fprintf(stream, "    <param n=\"1\">%5.1f</param>\n", cutoff_freq_high);
         fprintf(stream, "    <param n=\"2\">%5.3f</param>\n", feedback);
         fprintf(stream, "    <param n=\"3\">%2.1f</param>\n", 1.0f);
         fprintf(stream, "   </slot>\n");
         fprintf(stream, "  </effect>\n");
         fprintf(stream, " </mixer>\n\n");
         fprintf(stream, "</aeonwave>\n");
      }
      else printf(" Failed to open for writing: %s\n", strerror(errno));
   }

   return 0;
}

int write_phasing()
{
   for (int i=0; i<XGMIDI_MAX_PHASING_TYPES; ++i)
   {
      char fname[256];

      XGMIDI_effect_t *type = &XGMIDI_phasing_types[i];

      snprintf(fname, 255, "%s.aaxs", type->name);
      printf("Generating phasing: %s\n", fname);

      FILE *stream = fopen(fname, "w+");
      if (stream)
      {
         int lf = type->param[0];	// LFO Frequency
         int pd = type->param[1];	// LFO Depth
         int fb = type->param[3];	// Feedback Level
         int dt = type->param[2];	// Delay Offset
         int fl = type->param[5];	// EQ Low Frequency
         int fh = type->param[7];	// EQ High frequency
         int dw = type->param[9];	// Dry/Wet

         float lfo_frequency = XGMIDI_LFO_frequency_table[lf];
         float lfo_offset = dt/127.0f;
         float lfo_depth = pd/127.0f - lfo_offset;
         float gain = (dw - 1)/127.0f;

         float cutoff_freq_low = XGMIDI_EQ_frequency_table[fl];
         float cutoff_freq_high = XGMIDI_EQ_frequency_table[fh];
         float feedback = _MAX(fb - 64, 0)/127.0f;

         fprintf(stream, "<?xml version=\"1.0\"?>\n\n");
         fprintf(stream, "<aeonwave>\n\n");
         fprintf(stream, " <audioframe>\n");
         fprintf(stream, "  <effect type=\"phasing\" src=\"sine\">\n");
         fprintf(stream, "   <slot n=\"0\">\n");
         fprintf(stream, "    <param n=\"0\">%5.3f</param>\n", gain);
         fprintf(stream, "    <param n=\"1\">%5.3f</param>\n", lfo_frequency);
         fprintf(stream, "    <param n=\"2\">%5.3f</param>\n", lfo_depth);
         fprintf(stream, "    <param n=\"3\">%5.3f</param>\n", lfo_offset);
         fprintf(stream, "   </slot>\n");
         fprintf(stream, "   <slot n=\"1\">\n");
         fprintf(stream, "    <param n=\"0\">%5.1f</param>\n", cutoff_freq_low);
         fprintf(stream, "    <param n=\"1\">%5.1f</param>\n", cutoff_freq_high);
         fprintf(stream, "    <param n=\"2\">%5.3f</param>\n", feedback);
         fprintf(stream, "    <param n=\"3\">%2.1f</param>\n", 1.0f);
         fprintf(stream, "   </slot>\n");
         fprintf(stream, "  </effect>\n");
         fprintf(stream, " </audioframe>\n\n");

         fprintf(stream, " <mixer>\n");
         fprintf(stream, "  <effect type=\"phasing\" src=\"sine\">\n");
         fprintf(stream, "   <slot n=\"0\">\n");
         fprintf(stream, "    <param n=\"0\">%5.3f</param>\n", gain);
         fprintf(stream, "    <param n=\"1\">%5.3f</param>\n", lfo_frequency);
         fprintf(stream, "    <param n=\"2\">%5.3f</param>\n", lfo_depth);
         fprintf(stream, "    <param n=\"3\">%5.3f</param>\n", lfo_offset);
         fprintf(stream, "   </slot>\n");
         fprintf(stream, "   <slot n=\"1\">\n");
         fprintf(stream, "    <param n=\"0\">%5.1f</param>\n", cutoff_freq_low);
         fprintf(stream, "    <param n=\"1\">%5.1f</param>\n", cutoff_freq_high);
         fprintf(stream, "    <param n=\"2\">%5.3f</param>\n", feedback);
         fprintf(stream, "    <param n=\"3\">%2.1f</param>\n", 1.0f);
         fprintf(stream, "   </slot>\n");
         fprintf(stream, "  </effect>\n");
         fprintf(stream, " </mixer>\n\n");
         fprintf(stream, "</aeonwave>\n");
      }
      else printf(" Failed to open for writing: %s\n", strerror(errno));
   }

   return 0;
}

int main()
{
   write_reverb();
   write_chorus();
   write_phasing();

   return 0;
}
