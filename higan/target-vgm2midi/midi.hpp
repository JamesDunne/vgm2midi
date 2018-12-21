
struct MIDIFile;

struct MTrk : MIDIDevice {
  MTrk(MIDIFile& file_) : file(file_) {
    tick_ = 0;
    tick_abs = 0;
    flushCount = 0;
  }

  virtual auto noteOff(uint4 channel, uint7 note, uint7 velocity) -> void override;
  virtual auto noteOn(uint4 channel, uint7 note, uint7 velocity) -> void override;
  virtual auto keyPressureChange(uint4 channel, uint7 note, uint7 velocity) -> void override;
  virtual auto controlChange(uint4 channel, uint7 control, uint7 value) -> void override;
  virtual auto programChange(uint4 channel, uint7 program) -> void override;
  virtual auto channelPressureChange(uint4 channel, uint7 velocity) -> void override;
  virtual auto pitchBendChange(uint4 channel, uint14 pitchBend) -> void override;

  virtual auto control(uint4 channel, uint7 control) -> maybe<uint7> override;
  virtual auto program(uint4 channel) -> maybe<uint7> override;
  virtual auto pitchBend(uint4 channel) -> maybe<uint14> override;

  virtual auto meta(uint7 event, const array_view<uint8_t> data) -> void override;

  virtual auto tick() -> midi_tick_t const override;

private:
  MIDIFile& file;
  vector<uint8_t> bytes;
  midi_tick_t tick_;
  midi_tick_t tick_abs;
  int flushCount;

  auto writeVarint(uint value) -> void;
  auto writeTickDelta() -> void;

  auto advanceTicks(midi_tick_t ticks) -> void;

  auto write(uint8_t data) -> void;
  auto write(array_view<uint8_t> memory) -> void;

  auto flush() -> void;

  friend struct MIDIFile;
};

struct MIDIFile : MIDITiming {
  MIDIFile(file_buffer& file_, int midiFormat_ = 1) : midiFormat(midiFormat_), file(file_) {
    // file = file_;
    tick_abs = 0;
    if (midiFormat == 0) {
      tracks.append(new MTrk(*this));
    }
    mtrk0LengthOffset = 0;
  }

  file_buffer& file;
  const int midiFormat;
  vector<shared_pointer<MTrk>> tracks;
  uint64_t mtrk0LengthOffset;

  auto createTrack() -> shared_pointer<MTrk>;

  auto writeHeader() -> void const;
  auto updateHeader() -> void const;
  auto save() -> void const;

  virtual auto advanceTicks(midi_tick_t ticks) -> void override;
  virtual auto tick() -> midi_tick_t const override;

private:
  struct {
    maybe<uint7> noteKeyPressure[128];
    maybe<uint7> program;
    maybe<uint7> control[128];
    maybe<uint14> pitchBend;
  } channels[16];

  midi_tick_t tick_abs;

  friend struct MTrk;
};
