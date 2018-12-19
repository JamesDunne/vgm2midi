auto APU::Noise::clockLength() -> void {
  if(envelope.loopMode == 0) {
    if(lengthCounter > 0) lengthCounter--;
  }
}

auto APU::Noise::clock() -> uint8 {
  if(lengthCounter == 0) {
    midiNoteOff();
    return 0;
  }

  auto volume = envelope.volume();
  uint8 result = (lfsr & 1) ? volume : 0;

  if (volume > 0) {
    midiNoteOn();
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

  lastEnvelopeVolume = 0;
  lastMidiNoteVelocity = 0;
}

auto APU::Noise::midiNote() -> double {
  uint5 p = period;
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
  return envelope.midiVolume() * 3 / 4;
}

auto APU::Noise::midiNoteOn() -> void {
  auto n = midiNote();
  auto m = round(n);
  auto newMidiChannel = midiChannel();
  auto newChannelVolume = midiChannelVolume();
  auto newMidiNoteVelocity = midiNoteVelocity();

  if (!lastMidiNote || m != lastMidiNote() || newMidiNoteVelocity > lastMidiNoteVelocity) {
    midiNoteOff();

    if (m >= 0) {
      // Update channel volume:
      midi->controlChange(newMidiChannel, 0x07, newChannelVolume);

      // Change program:
      midi->programChange(newMidiChannel, midiProgram());

      // Note ON:
      midi->noteOn(newMidiChannel, m, midiNoteVelocity());

      lastMidiChannel = newMidiChannel;
      lastMidiNote = m;
    }
  } else if (lastMidiChannel) {
    // Update last channel played on's volume since we don't really support switching
    // duty cycle without restarting the note (i.e. playing it across multiple channels).
    midi->controlChange(lastMidiChannel(), 0x07, newChannelVolume);
  }

  lastMidiNoteVelocity = newMidiNoteVelocity;
}
