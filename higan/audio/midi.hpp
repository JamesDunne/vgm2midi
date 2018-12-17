typedef uint64_t midi_tick_t;

struct MIDIDevice {
  struct {
    maybe<uint7> note;
    maybe<uint7> noteKeyPressure[128];
    maybe<uint7> program;
    maybe<uint7> control[128];
    maybe<uint14> pitchBend;
  } channels[16];

  virtual auto noteOff(uint4 channel, uint7 note, uint7 velocity) -> void {
    channels[channel].note = nothing;
  };
  virtual auto noteOn(uint4 channel, uint7 note, uint7 velocity) -> void {
    channels[channel].note = note;
    channels[channel].noteKeyPressure[note] = velocity;
  };
  virtual auto keyPressureChange(uint4 channel, uint7 note, uint7 velocity) -> void {};
  virtual auto controlChange(uint4 channel, uint7 control, uint7 value) -> void {
    channels[channel].control[control] = value;
  };
  virtual auto programChange(uint4 channel, uint7 program) -> void {
    channels[channel].program = program;
  };
  virtual auto channelPressureChange(uint4 channel, uint7 velocity) -> void {};
  virtual auto pitchBendChange(uint4 channel, uint14 pitchBend) -> void {
    channels[channel].pitchBend = pitchBend;
  };

  virtual auto meta(uint7 event, const vector<uint8_t> &data) -> void {};

  virtual auto setTick(midi_tick_t tick) -> void = 0;
  virtual auto tick() -> midi_tick_t const = 0;

  struct Null;
};

struct MIDIDevice::Null : MIDIDevice {
  virtual auto setTick(midi_tick_t tick) -> void override {}
  virtual auto tick() -> midi_tick_t const override { return 0; }
};
