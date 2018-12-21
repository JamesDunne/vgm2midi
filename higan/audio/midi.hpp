typedef uint64_t midi_tick_t;

struct MIDITiming {
  virtual auto advanceTicks(midi_tick_t ticks) -> void = 0;
  virtual auto tick() -> midi_tick_t const = 0;

  struct Null;
};

struct MIDITiming::Null : MIDITiming {
  virtual auto advanceTicks(midi_tick_t ticks) -> void override {};
  virtual auto tick() -> midi_tick_t const override { return 0; };
};

struct MIDIDevice {
  virtual auto noteOff(uint4 channel, uint7 note, uint7 velocity) -> void = 0;
  virtual auto noteOn(uint4 channel, uint7 note, uint7 velocity) -> void = 0;
  virtual auto keyPressureChange(uint4 channel, uint7 note, uint7 velocity) -> void {};
  virtual auto controlChange(uint4 channel, uint7 control, uint7 value) -> void = 0;
  virtual auto programChange(uint4 channel, uint7 program) -> void = 0;
  virtual auto channelPressureChange(uint4 channel, uint7 velocity) -> void {};
  virtual auto pitchBendChange(uint4 channel, uint14 pitchBend) -> void = 0;

  virtual auto control(uint4 channel, uint7 control) -> maybe<uint7> = 0;
  virtual auto program(uint4 channel) -> maybe<uint7> = 0;
  virtual auto pitchBend(uint4 channel) -> maybe<uint14> = 0;

  virtual auto meta(uint7 event, const array_view<uint8_t> data) -> void {};

  virtual auto channelPrefixMeta(uint4 channel, uint7 event, const array_view<uint8_t> data) -> void {
    // MIDI Channel prefix:
    vector<uint8_t> mcp;
    mcp.appendm(channel, 1);
    meta(0x20, mcp);
    // Meta event:
    meta(event, data);
  };

  virtual auto tick() -> midi_tick_t const = 0;

  struct Null;
};

struct MIDIDevice::Null : MIDIDevice {
  virtual auto noteOff(uint4 channel, uint7 note, uint7 velocity) -> void override {};
  virtual auto noteOn(uint4 channel, uint7 note, uint7 velocity) -> void override {};
  virtual auto controlChange(uint4 channel, uint7 control, uint7 value) -> void override {};
  virtual auto programChange(uint4 channel, uint7 program) -> void override {};
  virtual auto pitchBendChange(uint4 channel, uint14 pitchBend) -> void override {};

  virtual auto control(uint4 channel, uint7 control) -> maybe<uint7> override { return nothing; };
  virtual auto program(uint4 channel) -> maybe<uint7> override { return nothing; };
  virtual auto pitchBend(uint4 channel) -> maybe<uint14> override { return nothing; };

  virtual auto tick() -> midi_tick_t const override { return 0; };

  static MIDIDevice::Null instance;
};
