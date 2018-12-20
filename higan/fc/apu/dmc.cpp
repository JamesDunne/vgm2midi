auto APU::DMC::start() -> void {
  if(lengthCounter == 0) {
    readAddr = 0x4000 + (addrLatch << 6);
    lengthCounter = (lengthLatch << 4) + 1;
    midiNoteOff();
    midiNoteOn();
  }
}

auto APU::DMC::stop() -> void {
  midiNoteOff();
  lengthCounter = 0;
  dmaDelayCounter = 0;
  cpu.rdyLine(1);
  cpu.rdyAddr(false);
}

auto APU::DMC::clock() -> uint8 {
  uint8 result = dacLatch;

  if(dmaDelayCounter > 0) {
    dmaDelayCounter--;

    if(dmaDelayCounter == 1) {
      cpu.rdyAddr(true, 0x8000 | readAddr);
    } else if(dmaDelayCounter == 0) {
      cpu.rdyLine(1);
      cpu.rdyAddr(false);

      dmaBuffer = cpu.mdr();
      dmaBufferValid = true;
      lengthCounter--;
      readAddr++;

      if(lengthCounter == 0) {
        if(loopMode) {
          start();
        } else {
          midiNoteOff();
          if(irqEnable) {
            irqPending = true;
            apu.setIRQ();
          }
        }
      }
    }
  }

  if(--periodCounter == 0) {
    if(sampleValid) {
      int delta = (((sample >> bitCounter) & 1) << 2) - 2;
      uint data = dacLatch + delta;
      if((data & 0x80) == 0) dacLatch = data;
    }

    if(++bitCounter == 0) {
      if(dmaBufferValid) {
        sampleValid = true;
        sample = dmaBuffer;
        dmaBufferValid = false;
      } else {
        sampleValid = false;
      }
    }

    periodCounter = Region::PAL() ? dmcPeriodTablePAL[period] : dmcPeriodTableNTSC[period];
  }

  if(lengthCounter > 0 && !dmaBufferValid && dmaDelayCounter == 0) {
    cpu.rdyLine(0);
    dmaDelayCounter = 4;
  }

  return result;
}

auto APU::DMC::power() -> void {
  lengthCounter = 0;
  irqPending = 0;

  period = 0;
  periodCounter = Region::PAL() ? dmcPeriodTablePAL[0] : dmcPeriodTableNTSC[0];
  irqEnable = 0;
  loopMode = 0;
  dacLatch = 0;
  addrLatch = 0;
  lengthLatch = 0;
  readAddr = 0;
  dmaDelayCounter = 0;
  bitCounter = 0;
  dmaBufferValid = 0;
  dmaBuffer = 0;
  sampleValid = 0;
  sample = 0;

  midi = platform->createMIDITrack();
}

auto APU::DMC::midiProgram() -> uint7 {
  auto sm = sampleMidiMap.find(addrLatch);
  if (!sm) {
    return 0;
  }

  return sm().midiProgram;
}

auto APU::DMC::midiChannel() -> uint4 {
  auto sm = sampleMidiMap.find(addrLatch);
  if (!sm) {
    return 9;
  }

  return sm().midiChannel;
}

auto APU::DMC::midiNote() -> double {
  auto sm = sampleMidiMap.find(addrLatch);
  if (!sm) {
    // MIDI Channel prefix:
    vector<uint8_t> mcp;
    mcp.appendm(midiChannel(), 1);
    midi->meta(0x20, mcp);
    // Instrumentation note:
    auto fmt = string("dmc sample=0x{0} period=0x{1}").format(string_format{hex(addrLatch, 2), hex(period, 2)});
    midi->meta(0x04, fmt);
    return period;
  }

  auto note = sm().periodMidiNote.find(period);
  if (!note) {
    // // MIDI Channel prefix:
    // vector<uint8_t> mcp;
    // mcp.appendm(midiChannel(), 1);
    // midi->meta(0x20, mcp);
    // // Instrumentation note:
    // auto fmt = string("dmc sample=0x{0} period=0x{1}").format(string_format{hex(addrLatch, 2), hex(period, 2)});
    // midi->meta(0x04, fmt);
    return sm().midiNote;
  }

  return note();
}

auto APU::DMC::midiNoteVelocity() -> uint7 {
  // NOTE: picking 32 as 1/4 of 128 (max sample)
  //  0.19789767009 = 159.79 / (100.0 + 1.0 / (32 / 22638))
  return (uint)(127 * log2(1 + pow(4.0 * 159.79 / (100.0 + 1.0 / (12 / 22638.0)), 0.75)));
}
