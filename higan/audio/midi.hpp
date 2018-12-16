typedef uint64_t midi_tick_t;

struct MIDITrack {
  //virtual auto track() -> uint const = 0;

  virtual auto setTick(midi_tick_t tick) -> void = 0;
  virtual auto tick() -> midi_tick_t const = 0;

  virtual auto meta(uint8 event, const vector<uint8_t> &data) -> void = 0;

  virtual auto noteOff(uint4 channel, uint7 note, uint7 velocity) -> void = 0;
  virtual auto noteOn(uint4 channel, uint7 note, uint7 velocity) -> void = 0;
  virtual auto keyPressure(uint4 channel, uint7 note, uint7 velocity) -> void = 0;
  virtual auto control(uint4 channel, uint7 control, uint7 value) -> void = 0;
  virtual auto program(uint4 channel, uint7 program) -> void = 0;
  virtual auto channelPressure(uint4 channel, uint7 program) -> void = 0;
  virtual auto pitchBend(uint4 channel, uint14 wheel) -> void = 0;

  struct Null;
};

struct MIDITrack::Null : MIDITrack {
  auto setTick(midi_tick_t tick) -> void {}
  auto tick() -> midi_tick_t const { return 0; }

  auto meta(uint8 event, const vector<uint8_t> &data) -> void {}

  auto noteOff(uint4 channel, uint7 note, uint7 velocity) -> void {}
  auto noteOn(uint4 channel, uint7 note, uint7 velocity) -> void {}
  auto keyPressure(uint4 channel, uint7 note, uint7 velocity) -> void {}
  auto control(uint4 channel, uint7 control, uint7 value) -> void {}
  auto program(uint4 channel, uint7 program) -> void {}
  auto channelPressure(uint4 channel, uint7 program) -> void {}
  auto pitchBend(uint4 channel, uint14 wheel) -> void {}
};
