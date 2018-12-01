
NSF::NSF(Markup::Node& document) : Board(document) {
  settings.mirror = document["board/mirror/mode"].text() == "vertical" ? 1 : 0;

  // Get the init and play addresses for the NSF (generated in manifest by vgm2midi.cpp):
  settings.addr_init = document["board/nsf/init"].natural();
  settings.addr_play = document["board/nsf/play"].natural();

  print("init={0} play={1}\n", string_format{hex(settings.addr_init,4), hex(settings.addr_play,4)});

  for (int x=0; x < 0x30+6; x++) {
    if (NSFROM[x] == 0x20) {
      NSFROM[x+1] = settings.addr_init & 0xFF;
      NSFROM[x+2] = settings.addr_init >> 8;
      NSFROM[x+8] = settings.addr_play & 0xFF;
      NSFROM[x+9] = settings.addr_play >> 8;
      break;
    }
  }

  for (int x=0; x < 0x30+6; x++) {
    print("{0}\n", string_format(hex(NSFROM[x],2)));
  }

  song_index = 0;
}

auto NSF::readPRG(uint addr) -> uint8 {
  print("NSF read  PRG 0x{0}\n", string_format{hex(addr,4)});
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

auto NSF::writePRG(uint addr, uint8 data) -> void {
  print("NSF write PRG 0x{0} = 0x{1}\n", string_format{hex(addr,4), hex(data,2)});
}

auto NSF::readCHR(uint addr) -> uint8 {
  print("NSF read  CHR 0x{0}\n", string_format{hex(addr,4)});
  if(addr & 0x2000) {
    if(settings.mirror == 0) addr = ((addr & 0x0800) >> 1) | (addr & 0x03ff);
    return ppu.readCIRAM(addr & 0x07ff);
  }
  if(chrram.size) return chrram.read(addr);
  return chrrom.read(addr);
}

auto NSF::writeCHR(uint addr, uint8 data) -> void {
  print("NSF write CHR 0x{0} = 0x{1}\n", string_format{hex(addr,4), hex(data,2)});
  if(addr & 0x2000) {
    if(settings.mirror == 0) addr = ((addr & 0x0800) >> 1) | (addr & 0x03ff);
    return ppu.writeCIRAM(addr & 0x07ff, data);
  }
  if(chrram.size) return chrram.write(addr, data);
}

auto NSF::power() -> void {
  prgBank = 0;
}

auto NSF::serialize(serializer& s) -> void {
  Board::serialize(s);

  s.integer(prgBank);
}
