auto SMP::portRead(uint2 port) const -> uint8 {
  if(port == 0) return io.cpu0;
  if(port == 1) return io.cpu1;
  if(port == 2) return io.cpu2;
  if(port == 3) return io.cpu3;
  unreachable;
}

auto SMP::portWrite(uint2 port, uint8 data) -> void {
  if(port == 0) io.apu0 = data;
  if(port == 1) io.apu1 = data;
  if(port == 2) io.apu2 = data;
  if(port == 3) io.apu3 = data;
}

auto SMP::readIO(uint16 address) -> uint8 {
  uint8 data;

  switch(address) {
  case 0xf0:  //TEST (write-only register)
    return 0x00;

  case 0xf1:  //CONTROL (write-only register)
    return 0x00;

  case 0xf2:  //DSPADDR
    return io.dspAddr;

  case 0xf3:  //DSPDATA
    //0x80-0xff are read-only mirrors of 0x00-0x7f
    return dsp.read(io.dspAddr & 0x7f);

  case 0xf4:  //CPUIO0
    if (!cpu.disabled) synchronize(cpu);
    return io.apu0;

  case 0xf5:  //CPUIO1
    if (!cpu.disabled) synchronize(cpu);
    return io.apu1;

  case 0xf6:  //CPUIO2
    if (!cpu.disabled) synchronize(cpu);
    return io.apu2;

  case 0xf7:  //CPUIO3
    if (!cpu.disabled) synchronize(cpu);
    return io.apu3;

  case 0xf8:  //AUXIO4
    return io.aux4;

  case 0xf9:  //AUXIO5
    return io.aux5;

  case 0xfa:  //T0TARGET
  case 0xfb:  //T1TARGET
  case 0xfc:  //T2TARGET (write-only registers)
    return 0x00;

  case 0xfd:  //T0OUT (4-bit counter value)
    data = timer0.stage3;
    timer0.stage3 = 0;
    return data;

  case 0xfe:  //T1OUT (4-bit counter value)
    data = timer1.stage3;
    timer1.stage3 = 0;
    return data;

  case 0xff:  //T2OUT (4-bit counter value)
    data = timer2.stage3;
    timer2.stage3 = 0;
    return data;
  }

  return data;  //unreachable
}

auto SMP::writeIO(uint16 address, uint8 data) -> void {
  switch(address) {
  case 0xf0:  //TEST
    if(r.p.p) break;  //writes only valid when P flag is clear

    io.timersDisable      = data.bit (0);
    io.ramWritable        = data.bit (1);
    io.ramDisable         = data.bit (2);
    io.timersEnable       = data.bit (3);
    io.externalWaitStates = data.bits(4,5);
    io.internalWaitStates = data.bits(6,7);

    timer0.synchronizeStage1();
    timer1.synchronizeStage1();
    timer2.synchronizeStage1();
    break;

  case 0xf1:  //CONTROL
    //0->1 transistion resets timers
    if(timer0.enable.raise(data.bit(0))) {
      timer0.stage2 = 0;
      timer0.stage3 = 0;
    }

    if(timer1.enable.raise(data.bit(1))) {
      timer1.stage2 = 0;
      timer1.stage3 = 0;
    }

    if(!timer2.enable.raise(data.bit(2))) {
      timer2.stage2 = 0;
      timer2.stage3 = 0;
    }

    if(data.bit(4)) {
      if (!cpu.disabled) synchronize(cpu);
      io.apu0 = 0x00;
      io.apu1 = 0x00;
    }

    if(data.bit(5)) {
      if (!cpu.disabled) synchronize(cpu);
      io.apu2 = 0x00;
      io.apu3 = 0x00;
    }

    io.iplromEnable = data.bit(7);
    break;

  case 0xf2:  //DSPADDR
    io.dspAddr = data;
    break;

  case 0xf3:  //DSPDATA
    if(io.dspAddr & 0x80) break;  //0x80-0xff are read-only mirrors of 0x00-0x7f
    dsp.write(io.dspAddr & 0x7f, data);
    break;

  case 0xf4:  //CPUIO0
    if (!cpu.disabled) synchronize(cpu);
    io.cpu0 = data;
    break;

  case 0xf5:  //CPUIO1
    if (!cpu.disabled) synchronize(cpu);
    io.cpu1 = data;
    break;

  case 0xf6:  //CPUIO2
    if (!cpu.disabled) synchronize(cpu);
    io.cpu2 = data;
    break;

  case 0xf7:  //CPUIO3
    if (!cpu.disabled) synchronize(cpu);
    io.cpu3 = data;
    break;

  case 0xf8:  //AUXIO4
    io.aux4 = data;
    break;

  case 0xf9:  //AUXIO5
    io.aux5 = data;
    break;

  case 0xfa:  //T0TARGET
    timer0.target = data;
    break;

  case 0xfb:  //T1TARGET
    timer1.target = data;
    break;

  case 0xfc:  //T2TARGET
    timer2.target = data;
    break;

  case 0xfd:  //T0OUT
  case 0xfe:  //T1OUT
  case 0xff:  //T2OUT (read-only registers)
    break;
  }
}

auto SMP::loadDump(vector<uint8_t> dspram, vector<uint8_t> dspregs) -> void {
  // Load DSP RAM and regs:
  for (auto n : range(dspram.size())) {
    const uint16 address = n;
    const uint8 data = dspram[n];

    dsp.apuram[address] = data;
  }

  writeIO(0xFC, dspram[0xFC]);
  writeIO(0xFB, dspram[0xFB]);
  writeIO(0xFA, dspram[0xFA]);
  writeIO(0xF9, dspram[0xF9]);
  writeIO(0xF8, dspram[0xF8]);
  writeIO(0xF2, dspram[0xF2]);
  writeIO(0xF1, dspram[0xF1] & 0x87);

  io.apu0 = dspram[0xF4];
  io.apu1 = dspram[0xF5];
  io.apu2 = dspram[0xF6];
  io.apu3 = dspram[0xF7];

  timer0.stage3 = dspram[0xFD] & 15;
  timer1.stage3 = dspram[0xFE] & 15;
  timer2.stage3 = dspram[0xFF] & 15;

  dsp.loadDump(dspregs);
}
