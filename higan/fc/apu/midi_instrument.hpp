struct MIDIInstrument {
  virtual auto midiNoteOn() -> void = 0;
  virtual auto midiNoteOff() -> void = 0;

  virtual auto midiNote() -> uint7 = 0;
  virtual auto midiVelocity() -> uint7 { return 64; };
  virtual auto midiPitchBend() -> uint14 { return 0x2000; };
  virtual auto midiPitchBendEnabled() -> bool { return false; };

  virtual auto midiChannel() -> uint4 = 0;
  virtual auto midiChannelVolume() -> uint7 { return 64; };

  shared_pointer<MIDIDevice> midi;
};

struct MIDIMelodic : MIDIInstrument {
  virtual auto midiPitchBendEnabled() -> bool override { return true; };
  virtual auto midiNoteOn() -> void override;
  virtual auto midiNoteOff() -> void override;
};

struct MIDIRhythmic : MIDIInstrument {
  virtual auto midiChannel() -> uint4 { return 9; };
  virtual auto midiNoteOn() -> void override;
  virtual auto midiNoteOff() -> void override;
};
