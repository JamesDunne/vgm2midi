auto APU::Noise::clockLength() -> void {
  if(envelope.loopMode == 0) {
    if(lengthCounter > 0) lengthCounter--;
  }
}

auto APU::Noise::clock() -> uint8 {
  if(written) written--;

  if(lengthCounter == 0) {
    if ( /* written == 0 && */ lastMidiNote) midiNoteOff();
    return 0;
  }

  auto volume = envelope.volume();
  uint8 result = (lfsr & 1) ? volume : 0;

  if (volume != lastEnvelopeVolume) {
    if (!lastMidiNote) midiNoteOn();
  } else if (lastMidiNote) {
    if (midiNote() != lastMidiNote()) midiNoteOn();
    // else midiNoteOff();
  }
  lastEnvelopeVolume = volume;

  if(--periodCounter == 0) {
    uint feedback;

    if(shortMode) {
      feedback = ((lfsr >> 0) & 1) ^ ((lfsr >> 6) & 1);
    } else {
      feedback = ((lfsr >> 0) & 1) ^ ((lfsr >> 1) & 1);
    }

    lfsr = (lfsr >> 1) | (feedback << 14);
    periodCounter = Region::PAL() ? apu.noisePeriodTablePAL[period] : apu.noisePeriodTableNTSC[period];
  }

  return result;
}

auto APU::Noise::power() -> void {
  lengthCounter = 0;

  envelope.speed = 0;
  envelope.useSpeedAsVolume = 0;
  envelope.loopMode = 0;
  envelope.reloadDecay = 0;
  envelope.decayCounter = 0;
  envelope.decayVolume = 0;

  period = 0;
  periodCounter = 1;
  shortMode = 0;
  lfsr = 1;

  midi = platform->createMIDITrack();

  written = 0;
  lastEnvelopeVolume = 0;
  lastEnvelopeDirection = 1;
}

auto APU::Noise::midiNote() -> double {
  uint p = period;
  if (shortMode) {
    p |= 0x10;
  }

  auto note = periodMidiNote.find(p);
  if (!note) {
    // // MIDI Channel prefix:
    // vector<uint8_t> mcp;
    // mcp.appendm(9, 1);
    // midi->meta(0x20, mcp);
    // // Instrumentation note:
    // auto fmt = string("noise period=0x{0}").format(string_format{hex(p, 2)});
    // midi->meta(0x04, fmt);
    return p;
  }

  return note();
}

auto APU::Noise::midiNoteVelocity() -> uint7 {
  auto x = envelope.volume();
  return apu.noiseMIDI[x];
}
