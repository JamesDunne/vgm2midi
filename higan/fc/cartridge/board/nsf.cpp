
NSF::NSF(Markup::Node& document) : Board(document) {
  settings.mirror = document["board/mirror/mode"].text() == "vertical" ? 1 : 0;

  // Get the init and play addresses for the NSF (generated in manifest by vgm2midi.cpp):
  settings.addr_init = document["board/nsf/init"].natural();
  settings.addr_play = document["board/nsf/play"].natural();

#if BUILD_DEBUG
  print("init={0} play={1}\n", string_format{hex(settings.addr_init,4), hex(settings.addr_play,4)});
#endif

  for (int x=0; x < 0x30+6; x++) {
    if (NSFROM[x] == 0x20) {
      NSFROM[x+1] = settings.addr_init & 0xFF;
      NSFROM[x+2] = settings.addr_init >> 8;
      NSFROM[x+8] = settings.addr_play & 0xFF;
      NSFROM[x+9] = settings.addr_play >> 8;
      break;
    }
  }

  // for (int x=0; x < 0x30+6; x++) {
  //   print("{0}\n", string_format(hex(NSFROM[x],2)));
  // }
}

auto NSF::readPRG(uint addr) -> uint8 {
#if BUILD_DEBUG
  print("NSF read  PRG 0x{0}\n", string_format{hex(addr,4)});
#endif
  if (((nmiFlags&1) && song_reload) || (nmiFlags&2) || doreset) {
    if (addr >= 0xFFFA && addr <= 0xFFFD) {
      // NMI:
      if (addr==0xFFFA) return(0x00);
      else if(addr==0xFFFB) return(0x38);
      // RESET:
      else if(addr==0xFFFC) return(0x20);
      else if(addr==0xFFFD) { doreset = 0; return(0x38); }
      // 0xFFFE is IRQ/BRK vector
    }
  }
  if (addr >= 0x3800 && addr <= 0x3835) {
    return NSFROM[addr-0x3800];
  }
  if (addr >= 0x3ff0 && addr <= 0x3fff) {
    if (addr == 0x3ff0) {
      // song reload:
      print("read 3ff0\n");
      uint8 x = song_reload;
      song_reload = 0;
      return x;
    } else if (addr == 0x3ff1) {
      print("read 3ff1: return song_index\n");

      // Clear RAM to 00:
      for (auto& data : cpu.ram) data = 0x00;

      // Reset APU:
      for (auto addr : range(0x4000, 0x4014)) {
        apu.writeIO(addr, 0x00);
      }
      // clear channels:
      apu.writeIO(0x4015, 0x00);
      apu.writeIO(0x4015, 0x0F);
      // 4-step mode:
      apu.writeIO(0x4017, 0x40);

      // Return current song index:
      return song_index;
    } else if (addr == 0x3ff3) {
      print("read 3ff3\n");
      return system.region() == System::Region::PAL ? 1 : 0;
    }

    return 0;
  }
  if(addr & 0x8000) return prgrom.read(addr);
  return cpu.mdr();
}

auto NSF::readPRGforced(uint addr) -> bool {
  if (((nmiFlags&1) && song_reload) || (nmiFlags&2) || doreset) {
    if (addr >= 0xFFFA && addr <= 0xFFFD) {
      return true;
    }
  }
  if (addr >= 0x3800 && addr <= 0x3835) {
    return true;
  }
  if (addr >= 0x3ff0 && addr <= 0x3fff) {
    return true;
  }
  if (addr & 0x8000) return true;
  return Board::readPRGforced(addr);
}

auto NSF::writePRG(uint addr, uint8 data) -> void {
#if 1 //BUILD_DEBUG
  print("NSF write PRG 0x{0} = 0x{1}\n", string_format{hex(addr,4), hex(data,2)});
#endif

  switch(addr)
  {
    case 0x3ff3: nmiFlags |= 1; break;
    case 0x3ff4: nmiFlags &= ~2;break;
    case 0x3ff5: nmiFlags |= 2; break;
  }
}

auto NSF::writePRGforced(uint addr) -> bool {
  return Board::writePRGforced(addr);
}


auto NSF::readCHR(uint addr) -> uint8 {
#if BUILD_DEBUG
  print("NSF read  CHR 0x{0}\n", string_format{hex(addr,4)});
#endif
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
  song_index = 0;
  song_reload = 0xFF;
  playing = false;
  nmiFlags = 0;
  doreset = 1;
}

auto NSF::serialize(serializer& s) -> void {
  Board::serialize(s);
}
