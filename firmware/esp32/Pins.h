#pragma once
// Pin maps from LILYGO T-Deck utilities.h and T-Deck-Pro hardware branches.
#if defined(MESHIT_TDECK_PRO)
constexpr int RADIO_CS=3,RADIO_IRQ=5,RADIO_RESET=4,RADIO_BUSY=6;
constexpr int RADIO_SCK=36,RADIO_MISO=47,RADIO_MOSI=33;
constexpr int DECK_SDA=13,DECK_SCL=14,DECK_SD_CS=48,DECK_DISPLAY_CS=34;
constexpr float RADIO_TCXO=2.4f;
#if defined(MESHIT_TDECK_PRO_MAX)
constexpr int DECK_DISPLAY_RESET=9;
#define MESHIT_BOARD_NAME "LILYGO T-Deck Pro Max"
#elif defined(MESHIT_TDECK_PRO_V11)
constexpr int DECK_DISPLAY_RESET=16;
#define MESHIT_BOARD_NAME "LILYGO T-Deck Pro v1.1"
#else
constexpr int DECK_DISPLAY_RESET=-1;
#define MESHIT_BOARD_NAME "LILYGO T-Deck Pro v1.0"
#endif
#elif defined(MESHIT_TDECK)
constexpr int RADIO_CS=9,RADIO_IRQ=45,RADIO_RESET=17,RADIO_BUSY=13;
constexpr int RADIO_SCK=40,RADIO_MISO=38,RADIO_MOSI=41;
constexpr int DECK_SDA=18,DECK_SCL=8,DECK_SD_CS=39,DECK_DISPLAY_CS=12;
constexpr float RADIO_TCXO=1.8f;
#if defined(MESHIT_TDECK_PLUS)
#define MESHIT_BOARD_NAME "LILYGO T-Deck Plus"
constexpr uint8_t DECK_DISPLAY_ROTATION=3;
#else
#define MESHIT_BOARD_NAME "LILYGO T-Deck"
constexpr uint8_t DECK_DISPLAY_ROTATION=1;
#endif
#else
constexpr int RADIO_CS=8,RADIO_IRQ=14,RADIO_RESET=12,RADIO_BUSY=13;
constexpr int RADIO_SCK=9,RADIO_MISO=11,RADIO_MOSI=10;
constexpr float RADIO_TCXO=1.8f;
#if defined(MESHIT_HELTEC_V4_KCT8103L)
#define MESHIT_BOARD_NAME "Heltec V4.3"
#elif defined(MESHIT_HELTEC_V4)
#define MESHIT_BOARD_NAME "Heltec V4.2"
#else
#define MESHIT_BOARD_NAME "Heltec V3"
#endif
#endif
