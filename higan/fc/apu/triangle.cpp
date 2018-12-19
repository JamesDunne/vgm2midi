auto APU::Triangle::clockLength() -> void {
  if(haltLengthCounter == 0) {
    if(lengthCounter > 0) lengthCounter--;
  }
}

auto APU::Triangle::clockLinearLength() -> void {
  if(reloadLinear) {
    linearLengthCounter = linearLength;
  } else if(linearLengthCounter) {
    linearLengthCounter--;
  }

  if(haltLengthCounter == 0) {
    reloadLinear = false;
    midiNoteOn();
  }
}

auto APU::Triangle::clock() -> uint8 {
  uint8 result = stepCounter & 0x0f;
  if((stepCounter & 0x10) == 0) result ^= 0x0f;
  if(lengthCounter == 0 || linearLengthCounter == 0 || (period+1 < 3)) {
    midiNoteOff();
    return result;
  }

  midiNoteContinue();

  if(--periodCounter == 0) {
    stepCounter++;
    periodCounter = period + 1;
  }

  return result;
}

auto APU::Triangle::power() -> void {
  lengthCounter = 0;

  linearLength = 0;
  haltLengthCounter = 0;
  period = 0;
  periodCounter = 1;
  stepCounter = 0;
  linearLengthCounter = 0;
  reloadLinear = 0;

  midi = platform->createMIDITrack();
}

auto APU::Triangle::midiProgram() -> uint7 {
  return 33; // Electric Bass (finger)
}

auto APU::Triangle::midiChannel() -> uint4 {
  return 8;
}

auto APU::Triangle::midiChannelVolume() -> uint7 {
  return 9 * 8;
}

auto APU::Triangle::midiNote() -> double {
  // MIDI 21 = A 1
  return apu.periodMidi[period] + 21;
}
