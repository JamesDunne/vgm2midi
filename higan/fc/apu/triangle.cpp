auto APU::Triangle::clockLength() -> void {
  if(midiTrigger && written == 0) {
    midiNoteOn();
    midiTrigger = false;
  }

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
  }
}

auto APU::Triangle::clock() -> uint8 {
  if(written) written--;

  uint8 result = stepCounter & 0x0f;
  if((stepCounter & 0x10) == 0) result ^= 0x0f;
  if(!reloadLinear && linearLengthCounter == 0) {
    if (written == 0) midiNoteOff();
  }

  if(lengthCounter == 0 || linearLengthCounter == 0) {
    return result;
  }
  if(period+1 < 3) {
    if (written == 0) midiNoteOff();
    return result;
  }

  if (written == 0) midiNoteContinue();

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

  midiTrigger = false;
  written = 0;
}

auto APU::Triangle::midiProgram() -> uint7 {
  return 33; // 34 Electric Bass (finger)
}

auto APU::Triangle::midiChannel() -> uint4 {
  return 8;
}

auto APU::Triangle::midiChannelVolume() -> uint7 {
  //  0.14161072338 = 159.79 / (100.0 + 1.0 / (8 / 8227.0))
  return (uint)(127 * log2(1 + pow(4.0 * 159.79 / (100.0 + 1.0 / (6 / 8227.0)), 0.75)));
}

auto APU::Triangle::midiNote() -> double {
  // MIDI 21 = A 1
  return apu.periodMidi[period] + 21;
}

auto APU::Triangle::midiNoteVelocity() -> uint7 {
  return 72;
}
