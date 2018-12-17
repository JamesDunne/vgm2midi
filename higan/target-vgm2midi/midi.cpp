
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

auto MTrk::meta(uint7 event, const vector<uint8_t> &data) -> void {
  writeTickDelta();

  bytes.append(0xFF);
  bytes.append(event);

  writeVarint(data.size());
  bytes.appends(data);
}

auto MTrk::noteOff(uint4 channel, uint7 note, uint7 velocity) -> void {
  if (!channels[channel].note) return;

  writeTickDelta();
  bytes.append(0x80 | channel);
  bytes.append(note);
  bytes.append(velocity);

  MIDIDevice::noteOff(channel, note, velocity);
}

auto MTrk::noteOn(uint4 channel, uint7 note, uint7 velocity) -> void {
  if (channels[channel].note && channels[channel].note() == note) return;

  writeTickDelta();
  bytes.append(0x90 | channel);
  bytes.append(note);
  bytes.append(velocity);

  MIDIDevice::noteOn(channel, note, velocity);
}

auto MTrk::keyPressureChange(uint4 channel, uint7 note, uint7 velocity) -> void {
  writeTickDelta();
  bytes.append(0xA0 | channel);
  bytes.append(note);
  bytes.append(velocity);

  MIDIDevice::keyPressureChange(channel, note, velocity);
}

auto MTrk::controlChange(uint4 channel, uint7 control, uint7 value) -> void {
  if (channels[channel].control[control] && channels[channel].control[control]() == value) return;

  writeTickDelta();
  bytes.append(0xB0 | channel);
  bytes.append(control);
  bytes.append(value);

  MIDIDevice::controlChange(channel, control, value);
}

auto MTrk::programChange(uint4 channel, uint7 program) -> void {
  if (channels[channel].program && channels[channel].program() == program) return;

  writeTickDelta();
  bytes.append(0xC0 | channel);
  bytes.append(program);

  MIDIDevice::programChange(channel, program);
}

auto MTrk::channelPressureChange(uint4 channel, uint7 velocity) -> void {
  writeTickDelta();
  bytes.append(0xD0 | channel);
  bytes.append(velocity);

  MIDIDevice::channelPressureChange(channel, velocity);
}

auto MTrk::pitchBendChange(uint4 channel, uint14 pitchBend) -> void {
  if (channels[channel].pitchBend && channels[channel].pitchBend() == pitchBend) return;

  writeTickDelta();
  bytes.append(0xE0 | channel);
  bytes.append(pitchBend & 0x7F);
  bytes.append(pitchBend >> 7);

  MIDIDevice::pitchBendChange(channel, pitchBend);
}
