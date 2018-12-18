
struct MIDIFile;

struct MTrk : MIDIDevice {
  MTrk(MIDIFile& file_) : file(file_) {
    tick_ = 0;
  }

  virtual auto noteOff(uint4 channel, uint7 note, uint7 velocity) -> void override;
  virtual auto noteOn(uint4 channel, uint7 note, uint7 velocity) -> void override;
  virtual auto keyPressureChange(uint4 channel, uint7 note, uint7 velocity) -> void override;
  virtual auto controlChange(uint4 channel, uint7 control, uint7 value) -> void override;
  virtual auto programChange(uint4 channel, uint7 program) -> void override;
  virtual auto channelPressureChange(uint4 channel, uint7 velocity) -> void override;
  virtual auto pitchBendChange(uint4 channel, uint14 pitchBend) -> void override;

  virtual auto note(uint4 channel) -> maybe<uint7> override;
  virtual auto control(uint4 channel, uint7 control) -> maybe<uint7> override;
  virtual auto program(uint4 channel) -> maybe<uint7> override;
  virtual auto pitchBend(uint4 channel) -> maybe<uint14> override;

  virtual auto meta(uint7 event, const vector<uint8_t> &data) -> void override;

private:
  MIDIFile& file;
  vector<uint8_t> bytes;
  midi_tick_t tick_;

  auto writeVarint(uint value) -> void;
  auto writeTickDelta() -> void;

  auto advanceTicks(midi_tick_t ticks) -> void;

  friend struct MIDIFile;
};

struct MIDIFile : MIDITiming {
  vector<MTrk*> tracks;

  auto createTrack() -> shared_pointer<MTrk>;

  auto setTick(midi_tick_t tick) -> void;
  auto tick() -> midi_tick_t const;

  auto save(string path) -> void const;

  virtual auto advanceTicks(midi_tick_t ticks) -> void override;

private:
  struct {
    maybe<uint7> note;
    maybe<uint7> noteKeyPressure[128];
    maybe<uint7> program;
    maybe<uint7> control[128];
    maybe<uint14> pitchBend;
  } channels[16];

  friend struct MTrk;
};
