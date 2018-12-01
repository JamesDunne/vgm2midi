
struct NSF : Board {
  NSF(Markup::Node& document) : Board(document) {
    settings.mirror = document["board/mirror/mode"].text() == "vertical" ? 1 : 0;

    // Get the init and play addresses for the NSF (generated in manifest by vgm2midi.cpp):
    settings.addr_init = document["board/nsf/init"].natural();
    settings.addr_play = document["board/nsf/play"].natural();

    print("init={0} play={1}\n", string_format{hex(settings.addr_init,4), hex(settings.addr_play,4)});

    for (int x=0;x < 0x30+6;x++) {
      if (NSFROM[x]==0x20) {
        NSFROM[x+1] = settings.addr_init & 0xFF;
        NSFROM[x+2] = settings.addr_init >> 8;
        NSFROM[x+8] = settings.addr_play & 0xFF;
        NSFROM[x+9] = settings.addr_play >> 8;
        break;
      }
    }
  }

  auto readPRG(uint addr) -> uint8 {
    print("NSF read  0x{0}\n", string_format{hex(addr,4)});
    if (addr >= 0xFFFA && addr <= 0xFFFD) {
      print("  NSF read vector\n");
      if (addr==0xFFFA) return(0x00);
      else if(addr==0xFFFB) return(0x38);
      else if(addr==0xFFFC) return(0x20);
      else if(addr==0xFFFD) return(0x38);
    }
    if (addr >= 0x3800 && addr <= 0x3835) {
      print("  NSF read ROM\n");
      return NSFROM[addr-0x3800];
    }
    if(addr & 0x8000) return prgrom.read(addr);
    return cpu.mdr();
  }

  auto writePRG(uint addr, uint8 data) -> void {
    print("NSF write 0x{0} = 0x{1}\n", string_format{hex(addr,4), hex(data,2)});
  }

  auto readCHR(uint addr) -> uint8 {
    if(addr & 0x2000) {
      if(settings.mirror == 0) addr = ((addr & 0x0800) >> 1) | (addr & 0x03ff);
      return ppu.readCIRAM(addr & 0x07ff);
    }
    if(chrram.size) return chrram.read(addr);
    return chrrom.read(addr);
  }

  auto writeCHR(uint addr, uint8 data) -> void {
    if(addr & 0x2000) {
      if(settings.mirror == 0) addr = ((addr & 0x0800) >> 1) | (addr & 0x03ff);
      return ppu.writeCIRAM(addr & 0x07ff, data);
    }
    if(chrram.size) return chrram.write(addr, data);
  }

  auto power() -> void {
    prgBank = 0;
  }

  auto serialize(serializer& s) -> void {
    Board::serialize(s);

    s.integer(prgBank);
  }

  struct Settings {
    bool mirror;      //0 = horizontal, 1 = vertical
    uint16 addr_init;
    uint16 addr_play;
  } settings;

  uint4 prgBank;

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
