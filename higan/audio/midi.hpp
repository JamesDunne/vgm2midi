typedef uint64_t midi_tick_t;

struct MIDIDevice {
  virtual auto noteOff(uint4 channel, uint7 note, uint7 velocity) -> void = 0;
  virtual auto noteOn(uint4 channel, uint7 note, uint7 velocity) -> void = 0;
  virtual auto keyPressureChange(uint4 channel, uint7 note, uint7 velocity) -> void {};
  virtual auto controlChange(uint4 channel, uint7 control, uint7 value) -> void = 0;
  virtual auto programChange(uint4 channel, uint7 program) -> void = 0;
  virtual auto channelPressureChange(uint4 channel, uint7 velocity) -> void {};
  virtual auto pitchBendChange(uint4 channel, uint14 pitchBend) -> void = 0;

  virtual auto note(uint4 channel) -> maybe<uint7> = 0;
  virtual auto control(uint4 channel, uint7 control) -> maybe<uint7> = 0;
  virtual auto program(uint4 channel) -> maybe<uint7> = 0;
  virtual auto pitchBend(uint4 channel) -> maybe<uint14> = 0;

  virtual auto meta(uint7 event, const vector<uint8_t> &data) -> void {};

  virtual auto setTick(midi_tick_t tick) -> void = 0;
  virtual auto tick() -> midi_tick_t const = 0;

  struct Null;
};

struct MIDIDevice::Null : MIDIDevice {
  virtual auto noteOff(uint4 channel, uint7 note, uint7 velocity) -> void {};
  virtual auto noteOn(uint4 channel, uint7 note, uint7 velocity) -> void {};
  virtual auto controlChange(uint4 channel, uint7 control, uint7 value) -> void {};
  virtual auto programChange(uint4 channel, uint7 program) -> void {};
  virtual auto pitchBendChange(uint4 channel, uint14 pitchBend) -> void {};

  virtual auto note(uint4 channel) -> maybe<uint7> { return nothing; };
  virtual auto control(uint4 channel, uint7 control) -> maybe<uint7> { return nothing; };
  virtual auto program(uint4 channel) -> maybe<uint7> { return nothing; };
  virtual auto pitchBend(uint4 channel) -> maybe<uint14> { return nothing; };

  virtual auto setTick(midi_tick_t tick) -> void override {}
  virtual auto tick() -> midi_tick_t const override { return 0; }
};
