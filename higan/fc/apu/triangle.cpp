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

  if(haltLengthCounter == 0) reloadLinear = false;
}

auto APU::Triangle::clock() -> uint8 {
  uint8 result = stepCounter & 0x0f;
  if((stepCounter & 0x10) == 0) result ^= 0x0f;
  if(lengthCounter == 0 || linearLengthCounter == 0) return result;

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

auto APU::Triangle::midiChannel() -> uint4 {
  return 8;
}

auto APU::Triangle::midiNote() -> uint7 {
  double f = system.frequency() / (16 * (period + 1));
  double n = (log(f / 54.99090178) / log(2)) * 12;

  // MIDI 21 = A 1
  return round(n) + 21;
}

auto APU::Triangle::midiPitchBend() -> uint14 {
  double f = system.frequency() / (16 * (period + 1));
  double n = (log(f / 54.99090178) / log(2)) * 12;

  return (uint14)((n - round(n)) * 0xFFF) + 0x2000;
}
