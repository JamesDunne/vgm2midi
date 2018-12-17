
// MIDIFile:

auto MIDIFile::createTrack() -> shared_pointer<MTrk> {
  auto mtrk = new MTrk;
  tracks.append(mtrk);
  return mtrk;
}

auto MIDIFile::setTick(midi_tick_t tick) -> void {
  for (auto track : tracks) {
    track->setTick(tick);
  }
}

auto MIDIFile::tick() -> midi_tick_t const {
  if (tracks.size() == 0) return 0;

  // All tracks should have same tick:
  return tracks[0]->tick();
}

auto MIDIFile::save(string path) -> void const {
  auto buf = file::open(path, file::mode::write);

  buf.write({"MThd", 4});
  // MThd length:
  buf.writem(6, 4);
  // format 1:
  buf.writem(1, 2);
  // track count:
  buf.writem(tracks.size(), 2);
  // division:
  buf.writem(480, 2);

  for (auto track : tracks) {
    buf.write({"MTrk", 4});
    // MTrk length:
    buf.writem(track->bytes.size(), 4);
    buf.write(track->bytes);
  }

  buf.close();
}


// MTrk:

auto MTrk::writeVarint(uint value) -> void {
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

auto MTrk::writeTickDelta() -> void {
  uint delta = tick_ - lastTick;
  writeVarint(delta);

  lastTick = tick_;
}


auto MTrk::setTick(midi_tick_t tick) -> void {
  assert(tick >= tick_);
  tick_ = tick;
}
auto MTrk::tick() -> midi_tick_t const {
  return tick_;
}

auto MTrk::meta(uint8 event, const vector<uint8_t> &data) -> void {
  writeTickDelta();

  bytes.append(0xFF);
  bytes.append(event & 0x7F);

  writeVarint(data.size());
  bytes.appends(data);
}

auto MTrk::noteOff(uint4 channel, uint7 note, uint7 velocity) -> void {
  writeTickDelta();
  bytes.append(0x80 | channel);
  bytes.append(note);
  bytes.append(velocity);
}

auto MTrk::noteOn(uint4 channel, uint7 note, uint7 velocity) -> void {
  writeTickDelta();
  bytes.append(0x90 | channel);
  bytes.append(note);
  bytes.append(velocity);
}

auto MTrk::keyPressure(uint4 channel, uint7 note, uint7 velocity) -> void {
  writeTickDelta();
  bytes.append(0xA0 | channel);
  bytes.append(note);
  bytes.append(velocity);
}

auto MTrk::control(uint4 channel, uint7 control, uint7 value) -> void {
  writeTickDelta();
  bytes.append(0xB0 | channel);
  bytes.append(control);
  bytes.append(value);
}

auto MTrk::program(uint4 channel, uint7 program) -> void {
  writeTickDelta();
  bytes.append(0xC0 | channel);
  bytes.append(program);
}

auto MTrk::channelPressure(uint4 channel, uint7 velocity) -> void {
  writeTickDelta();
  bytes.append(0xD0 | channel);
  bytes.append(velocity);
}

auto MTrk::pitchBend(uint4 channel, uint14 wheel) -> void {
  writeTickDelta();
  bytes.append(0xE0 | channel);
  bytes.append(wheel & 0x7F);
  bytes.append(wheel >> 7);
}
