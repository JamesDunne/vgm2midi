
struct MTrk : MIDIDevice {
  virtual auto noteOff(uint4 channel, uint7 note, uint7 velocity) -> void override;
  virtual auto noteOn(uint4 channel, uint7 note, uint7 velocity) -> void override;
  virtual auto keyPressureChange(uint4 channel, uint7 note, uint7 velocity) -> void override;
  virtual auto controlChange(uint4 channel, uint7 control, uint7 value) -> void override;
  virtual auto programChange(uint4 channel, uint7 program) -> void override;
  virtual auto channelPressureChange(uint4 channel, uint7 velocity) -> void override;
  virtual auto pitchBendChange(uint4 channel, uint14 pitchBend) -> void override;

  virtual auto meta(uint7 event, const vector<uint8_t> &data) -> void override;

  virtual auto setTick(midi_tick_t tick) -> void override;
  virtual auto tick() -> midi_tick_t const override;

private:
  vector<uint8_t> bytes;
  midi_tick_t tick_;
  midi_tick_t lastTick;

  auto writeVarint(uint value) -> void;
  auto writeTickDelta() -> void;

  friend struct MIDIFile;
};

struct MIDIFile {
  vector<MTrk*> tracks;

  auto createTrack() -> shared_pointer<MTrk>;

  auto setTick(midi_tick_t tick) -> void;
  auto tick() -> midi_tick_t const;

  auto save(string path) -> void const;
};
