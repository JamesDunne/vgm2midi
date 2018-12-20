auto APU::Pulse::clockLength() -> void {
  if(envelope.loopMode == 0) {
    if(lengthCounter) lengthCounter--;
  }
}

auto APU::Pulse::clock() -> uint8 {
  if(written) written--;

  if(!sweep.checkPeriod()) {
    midiNoteOff();
    return 0;
  }
  if(lengthCounter == 0) {
    midiNoteOff();
    return 0;
  }

  static const uint dutyTable[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1},  //12.5%
    {0, 0, 0, 0, 0, 0, 1, 1},  //25.0%
    {0, 0, 0, 0, 1, 1, 1, 1},  //50.0%
    {1, 1, 1, 1, 1, 1, 0, 0},  //25.0% (negated)
  };

  auto volume = envelope.volume();
  uint8 result = dutyTable[duty][dutyCounter] ? volume : 0;
  if(sweep.pulsePeriod < 0x008) {
    midiNoteOff();
    result = 0;
  } else if (written == 0) {
    if (volume > 0) {
      bool trigger = midiTrigger || envelope.midiTrigger;
      if (!trigger && midiTriggerMaybe && !lastMidiNote) {
        trigger = true;
      }
      if (!trigger && duty != lastDuty) {
        trigger = true;
      }

      if (trigger) {
        if (envelope.midiTrigger) envelope.midiTrigger = false;
        if (midiTrigger) midiTrigger = false;

        midiNoteOn();
      }

      midiNoteContinue();
      lastDuty = duty;
    } else if (volume == 0) {
      midiNoteOff();
    }
  }

  if(--periodCounter == 0) {
    periodCounter = (sweep.pulsePeriod + 1) * 2;
    dutyCounter--;
  }

  return result;
}

auto APU::Pulse::power() -> void {
  envelope.power();
  sweep.power();

  lengthCounter = 0;

  duty = 0;
  dutyCounter = 0;
  period = 0;
  periodCounter = 1;

  midi = platform->createMIDITrack();

  midiTrigger = 0;
  midiTriggerMaybe = 0;
  written = 0;
  lastDuty = 0;
}

auto APU::Pulse::midiProgram() -> uint7 {
  switch (duty) {
    case 0: return 80; // 81 Lead 1 (square)
    case 1: return 29; // 30 Overdriven Guitar
    case 2: return 82; // 83 Lead 3 (calliope)
    case 3: return 29; // 30 Overdriven Guitar
    default: return 80;
  }
}

auto APU::Pulse::midiChannel() -> uint4 {
  return 4 * n + duty;
}

auto APU::Pulse::midiChannelVolume() -> uint7 {
  return envelope.midiVolume();
}

auto APU::Pulse::midiNote() -> double {
  // MIDI 33 = A 2
  return apu.periodMidi[sweep.pulsePeriod] + 33;
}

auto APU::Pulse::midiNoteVelocity() -> uint7 {
  return 72;
}
