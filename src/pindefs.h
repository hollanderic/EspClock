#ifdef BOARD_ESP32DEV
// Pin signal connections:
#define R1_PIN 18
#define G1_PIN 25
#define B1_PIN 5
#define R2_PIN 17
#define G2_PIN 33
#define B2_PIN 16
#define A_PIN 4
#define B_PIN 3
#define C_PIN 0
#define D_PIN 21
#define E_PIN                                                                  \
  32 // required for 1/32 scan panels, like 64x64px. Any available pin would do,
     // i.e. IO32
#define LAT_PIN 19
#define OE_PIN 15
#define CLK_PIN 2
#endif

#ifdef BOARD_ESP32S3ZERO

#define R1_PIN 1
#define G1_PIN 2
#define B1_PIN 3
#define R2_PIN 4
#define G2_PIN 5
#define B2_PIN 6
#define A_PIN 7
#define B_PIN 8
#define C_PIN 9
#define D_PIN 10
#define E_PIN                                                                  \
  -1 // required for 1/32 scan panels, like 64x64px. Any available pin would do,
     // i.e. IO32
#define LAT_PIN 11
#define OE_PIN 12
#define CLK_PIN 13
#endif

#ifdef BOARD_ESP32S3DEV

#define R1_PIN 37
#define G1_PIN 6
#define B1_PIN 36
#define R2_PIN 35
#define G2_PIN 5
#define B2_PIN 0
#define A_PIN 45
#define B_PIN 1
#define C_PIN 48
#define D_PIN 2
#define E_PIN 4
#define LAT_PIN 38
#define OE_PIN 21
#define CLK_PIN 47
#endif

#if 0
// Adding dummy values to keep ide from showing errors since board defintion
// isn't set until compile time.
#define R1_PIN -1
#define G1_PIN -1
#define B1_PIN -1
#define R2_PIN -1
#define G2_PIN -1
#define B2_PIN -1
#define A_PIN -1
#define B_PIN -1
#define C_PIN -1
#define D_PIN -1
#define E_PIN -1
#define LAT_PIN -1
#define OE_PIN -1
#define CLK_PIN -1
#endif