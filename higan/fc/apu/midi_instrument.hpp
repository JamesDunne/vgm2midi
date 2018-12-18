struct MIDIInstrument {
  virtual auto midiNoteOn() -> void = 0;
  virtual auto midiNoteOff() -> void = 0;

  virtual auto midiProgram() -> uint7 = 0;

  virtual auto midiNote() -> double = 0;
  virtual auto midiNoteVelocity() -> uint7 { return 64; };

  virtual auto midiChannel() -> uint4 = 0;
  virtual auto midiChannelVolume() -> uint7 { return 64; };

  static auto midiPitchBend(double n) -> uint14 {
    return (uint14)((n - round(n)) * 0xFFF) + 0x2000;
  };

  shared_pointer<MIDIDevice> midi;
};

struct MIDIMelodic : MIDIInstrument {
  virtual auto midiNoteOn() -> void override;
  virtual auto midiNoteOff() -> void override;

  maybe<double> lastMidiNote;
  maybe<midi_tick_t> lastMidiNoteTick;
  maybe<uint4> lastMidiChannel;
};

struct MIDIRhythmic : MIDIMelodic {
  virtual auto midiProgram() -> uint7 override { return 0; };
  virtual auto midiChannel() -> uint4 override { return 9; };
};
