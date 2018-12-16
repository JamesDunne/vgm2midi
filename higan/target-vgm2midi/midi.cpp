
auto MIDIFile::createTrack() -> shared_pointer<MTrk> {
  tracks.resize(tracks.size() + 1);
  return &tracks[tracks.size() - 1];
}

auto MTrk::varint(uint value) -> void {
  uint8 chr1 = (uint8)(value & 0x7F);
  value >>= 7;
  if (value > 0) {
    uint8 chr2 = (uint8)((value & 0x7F) | 0x80);
    value >>= 7;
    if (value > 0) {
      uint8 chr3 = (uint8)((value & 0x7F) | 0x80);
      value >>= 7;
      if (value > 0) {
        uint8 chr4 = (uint8)((value & 0x7F) | 0x80);

        bytes.append(chr4);
        bytes.append(chr3);
        bytes.append(chr2);
        bytes.append(chr1);
      } else {
        bytes.append(chr3);
        bytes.append(chr2);
        bytes.append(chr1);
      }
    } else {
      bytes.append(chr2);
      bytes.append(chr1);
    }
  } else {
    bytes.append(chr1);
  }
}


auto MTrk::setTick(midi_tick_t tick) -> void {
  assert(tick >= tick_);
  tick_ = tick;
}
auto MTrk::tick() -> midi_tick_t const {
  return tick_;
}

auto MTrk::meta(uint8 event, const vector<uint8_t> &data) -> void {
  varint(tick_);

  bytes.append(0xFF);
  bytes.append(event & 0x7F);

  varint(data.size());
  bytes.appends(data);
}

auto MTrk::noteOff(uint4 channel, uint7 note, uint7 velocity) -> void {
  varint(tick_);
  bytes.append(0x80 | channel);
  bytes.append(note);
  bytes.append(velocity);
}
auto MTrk::noteOn(uint4 channel, uint7 note, uint7 velocity) -> void {
  varint(tick_);
  bytes.append(0x90 | channel);
  bytes.append(note);
  bytes.append(velocity);
}
auto MTrk::keyPressure(uint4 channel, uint7 note, uint7 velocity) -> void {
  varint(tick_);
  bytes.append(0xA0 | channel);
  bytes.append(note);
  bytes.append(velocity);
}
auto MTrk::control(uint4 channel, uint7 control, uint7 value) -> void {
  varint(tick_);
  bytes.append(0xB0 | channel);
  bytes.append(control);
  bytes.append(value);
}
auto MTrk::program(uint4 channel, uint7 program) -> void {
  varint(tick_);
  bytes.append(0xC0 | channel);
  bytes.append(program);
}
auto MTrk::channelPressure(uint4 channel, uint7 velocity) -> void {
  varint(tick_);
  bytes.append(0xD0 | channel);
  bytes.append(velocity);
}
auto MTrk::pitchBend(uint4 channel, uint14 wheel) -> void {
  varint(tick_);
  bytes.append(0xE0 | channel);
  bytes.append(wheel & 0x7F);
  bytes.append(wheel >> 7);
}
