
// MIDIFile:

auto MIDIFile::createTrack() -> shared_pointer<MTrk> {
  auto mtrk = new MTrk(*this);
  tracks.append(mtrk);
  return mtrk;
}

auto MIDIFile::advanceTicks(midi_tick_t ticks) -> void {
  for (auto track: tracks) {
    track->advanceTicks(ticks);
  }
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

auto MTrk::advanceTicks(midi_tick_t ticks) -> void {
  tick_ += ticks;
}

auto MTrk::writeVarint(uint value) -> void {
  uint8_t chr1 = (uint8_t)(value & 0x7F);
  value >>= 7;
  if (value > 0) {
    uint8_t chr2 = (uint8_t)((value & 0x7F) | 0x80);
    value >>= 7;
    if (value > 0) {
      uint8_t chr3 = (uint8_t)((value & 0x7F) | 0x80);
      value >>= 7;
      if (value > 0) {
        uint8_t chr4 = (uint8_t)((value & 0x7F) | 0x80);

        assert((chr4 & 0x80) == 0x80);
        bytes.append(chr4);
        assert((chr3 & 0x80) == 0x80);
        bytes.append(chr3);
        assert((chr2 & 0x80) == 0x80);
        bytes.append(chr2);
        assert((chr1 & 0x80) == 0x00);
        bytes.append(chr1);
      } else {
        assert((chr3 & 0x80) == 0x80);
        bytes.append(chr3);
        assert((chr2 & 0x80) == 0x80);
        bytes.append(chr2);
        assert((chr1 & 0x80) == 0x00);
        bytes.append(chr1);
      }
    } else {
      assert((chr2 & 0x80) == 0x80);
      bytes.append(chr2);
      assert((chr1 & 0x80) == 0x00);
      bytes.append(chr1);
    }
  } else {
    assert((chr1 & 0x80) == 0x00);
    bytes.append(chr1);
  }
}

auto MTrk::writeTickDelta() -> void {
  writeVarint(tick_);

  tick_ = 0;
}

auto MTrk::meta(uint7 event, const array_view<uint8_t> data) -> void {
  writeTickDelta();

  bytes.append(0xFF);
  bytes.append(event);

  writeVarint(data.size());
  bytes.appends(data);
}

auto MTrk::note(uint4 channel) -> maybe<uint7> {
  return nothing;
  // return file.channels[channel].note;
}
auto MTrk::control(uint4 channel, uint7 control) -> maybe<uint7> {
  return file.channels[channel].control[control];
}
auto MTrk::program(uint4 channel) -> maybe<uint7> {
  return file.channels[channel].program;
}
auto MTrk::pitchBend(uint4 channel) -> maybe<uint14> {
  return file.channels[channel].pitchBend;
}

auto MTrk::noteOff(uint4 channel, uint7 note, uint7 velocity) -> void {
  // if (!file.channels[channel].note) return;

  writeTickDelta();
  bytes.append(0x80 | channel);
  bytes.append(note);
  bytes.append(velocity);

  // file.channels[channel].note = nothing;
}

auto MTrk::noteOn(uint4 channel, uint7 note, uint7 velocity) -> void {
  // if (file.channels[channel].note && file.channels[channel].note() == note) return;

  writeTickDelta();
  bytes.append(0x90 | channel);
  bytes.append(note);
  bytes.append(velocity);

  // file.channels[channel].note = note;
}

auto MTrk::keyPressureChange(uint4 channel, uint7 note, uint7 velocity) -> void {
  writeTickDelta();
  bytes.append(0xA0 | channel);
  bytes.append(note);
  bytes.append(velocity);
}

auto MTrk::controlChange(uint4 channel, uint7 control, uint7 value) -> void {
  if (file.channels[channel].control[control]
    && file.channels[channel].control[control]() == value) return;

  writeTickDelta();
  bytes.append(0xB0 | channel);
  bytes.append(control);
  bytes.append(value);

  file.channels[channel].control[control] = value;
}

auto MTrk::programChange(uint4 channel, uint7 program) -> void {
  if (file.channels[channel].program
    && file.channels[channel].program() == program) return;

  writeTickDelta();
  bytes.append(0xC0 | channel);
  bytes.append(program);

  file.channels[channel].program = program;
}

auto MTrk::channelPressureChange(uint4 channel, uint7 velocity) -> void {
  writeTickDelta();
  bytes.append(0xD0 | channel);
  bytes.append(velocity);
}

auto MTrk::pitchBendChange(uint4 channel, uint14 pitchBend) -> void {
  if (file.channels[channel].pitchBend
    && file.channels[channel].pitchBend() == pitchBend) return;

  writeTickDelta();
  bytes.append(0xE0 | channel);
  bytes.append(pitchBend & 0x7F);
  bytes.append((pitchBend >> 7) & 0x7F);

  file.channels[channel].pitchBend = pitchBend;
}
