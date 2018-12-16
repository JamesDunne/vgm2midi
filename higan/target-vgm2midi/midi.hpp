
struct MTrk {
  vector<uint8_t> bytes;
  midi_tick_t tick;

  auto varint(uint value) -> void;
  auto time(midi_tick_t abs_tick) -> void;

  auto meta(midi_tick_t abs_tick, uint8 event, uint len, const char *data) -> void;

  auto cmd2(midi_tick_t abs_tick, uint8 cmd, uint8 data1) -> void;
  auto cmd3(midi_tick_t abs_tick, uint8 cmd, uint8 data1, uint8 data2) -> void;
};

struct MIDIFile {
  vector<MTrk> tracks;
};
