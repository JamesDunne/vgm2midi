
struct MTrk : MIDITrack {
  auto setTick(midi_tick_t tick) -> void override;
  auto tick() -> midi_tick_t const override;

  auto meta(uint8 event, const vector<uint8_t> &data) -> void override;

  auto noteOff(uint4 channel, uint7 note, uint7 velocity) -> void override;
  auto noteOn(uint4 channel, uint7 note, uint7 velocity) -> void override;
  auto keyPressure(uint4 channel, uint7 note, uint7 velocity) -> void override;
  auto control(uint4 channel, uint7 control, uint7 value) -> void override;
  auto program(uint4 channel, uint7 program) -> void override;
  auto channelPressure(uint4 channel, uint7 program) -> void override;
  auto pitchBend(uint4 channel, uint14 wheel) -> void override;

private:
  vector<uint8_t> bytes;
  midi_tick_t tick_;

  auto varint(uint value) -> void;
};

struct MIDIFile {
  vector<MTrk> tracks;

  auto createTrack() -> shared_pointer<MTrk>;
};
