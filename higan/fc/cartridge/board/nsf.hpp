
struct NSF : Board {
  NSF(Markup::Node& document);

  auto readPRG(uint addr) -> uint8 override;
  auto writePRG(uint addr, uint8 data) -> void override;
  auto readPRGforced(uint addr) -> bool override;
  auto writePRGforced(uint addr) -> bool override;

  auto readCHR(uint addr) -> uint8 override;
  auto writeCHR(uint addr, uint8 data) -> void override;

  auto power() -> void override;

  auto serialize(serializer& s) -> void override;

  struct Settings {
    bool mirror;      //0 = horizontal, 1 = vertical
    uint16 addr_init;
    uint16 addr_play;
  } settings;

  uint8 nmiFlags;
  uint8 doreset;

  uint8 song_reload;
  uint8 song_index;

  bool playing;

  /*
  00:8000:8D F4 3F  STA $3FF4 = #$00
  00:8003:A2 FF     LDX #$FF
  00:8005:9A        TXS
  00:8006:AD F0 3F  LDA $3FF0 = #$00
  00:8009:F0 09     BEQ $8014
  00:800B:AD F1 3F  LDA $3FF1 = #$00
  00:800E:AE F3 3F  LDX $3FF3 = #$00
  00:8011:20 00 00  JSR $0000
  00:8014:A9 00     LDA #$00
  00:8016:AA        TAX
  00:8017:A8        TAY
  00:8018:20 00 00  JSR $0000
  00:801B:8D F5 EF  STA $EFF5 = #$FF
  00:801E:90 FE     BCC $801E
  00:8020:8D F3 3F  STA $3FF3 = #$00
  00:8023:18        CLC
  00:8024:90 FE     BCC $8024
  */
  uint8 NSFROM[0x30+6] = {
    /* 0x00 - NMI */
    0x8D,0xF4,0x3F,       /* Stop play routine NMIs. */
    0xA2,0xFF,0x9A,       /* Initialize the stack pointer. */
    0xAD,0xF0,0x3F,       /* See if we need to init. */
    0xF0,0x09,            /* If 0, go to play routine playing. */

    0xAD,0xF1,0x3F,       /* Confirm and load A      */
    0xAE,0xF3,0x3F,       /* Load X with PAL/NTSC byte */

    0x20,0x00,0x00,       /* JSR to init routine     */

    0xA9,0x00,
    0xAA,
    0xA8,
    0x20,0x00,0x00,       /* JSR to play routine  */
    0x8D,0xF5,0x3F,       /* Start play routine NMIs. */
    0x90,0xFE,            /* Loopie time. */

    /* 0x20 */
    0x8D,0xF3,0x3F,       /* Init init NMIs */
    0x18,
    0x90,0xFE             /* Loopie time. */
  };
};
