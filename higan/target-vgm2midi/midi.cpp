
// MIDIFile:

auto MIDIFile::createTrack() -> shared_pointer<MTrk> {
  if (midiFormat == 1) {
    auto mtrk = new MTrk(*this);
    tracks.append(mtrk);
    return mtrk;
  } else {
    // Format 0 has only one track:
    return tracks[0];
  }
}

auto MIDIFile::advanceTicks(midi_tick_t ticks) -> void {
  tick_abs += ticks;
  for (auto track: tracks) {
    track->advanceTicks(ticks);
  }
}

auto MIDIFile::tick() -> midi_tick_t const {
  return tick_abs;
}

auto MIDIFile::writeHeader() -> void const {
  file.write({"MThd", 4});
  // MThd length:
  file.writem(6, 4);
  // format 1:
  file.writem(midiFormat, 2);
  // track count:
  file.writem(tracks.size(), 2);
  // division:
  file.writem(480, 2);

  if (midiFormat == 0) {
    file.write({"MTrk", 4});

    // MTrk length:
    mtrk0LengthOffset = file.offset();
    file.writem(tracks[0]->bytes.size(), 4);
  }
}

auto MIDIFile::updateHeader() -> void const {
  if (midiFormat == 0) {
    // Go back to MTrk 0 and write current size:
    auto offs = file.offset();
    file.seek(mtrk0LengthOffset);
    file.writem(offs - (mtrk0LengthOffset + 4), 4);
    file.seek(offs);
  }
}

auto MIDIFile::save() -> void const {
  if (midiFormat == 1) {
    for (auto track : tracks) {
      file.write({"MTrk", 4});
      // MTrk length:
      file.writem(track->bytes.size(), 4);
      file.write(track->bytes);
    }
  } else {
    updateHeader();
  }

  file.close();
}


// MTrk:

auto MTrk::write(uint8_t data) -> void {
  if (file.midiFormat == 1) {
    bytes.append(data);
  } else {
    file.file.write(data);
  }
}

auto MTrk::write(array_view<uint8_t> memory) -> void {
  if (file.midiFormat == 1) {
    bytes.appends(memory);
  } else {
    file.file.write(memory);
  }
}

auto MTrk::flush() -> void {
  if (file.midiFormat == 0) {
    if (++flushCount == 100) {
      file.updateHeader();
      flushCount = 0;
    }
  }
}


auto MTrk::advanceTicks(midi_tick_t ticks) -> void {
  tick_ += ticks;
  tick_abs += ticks;
}

auto MTrk::tick() -> midi_tick_t const {
  return tick_abs;
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
        write(chr4);
        assert((chr3 & 0x80) == 0x80);
        write(chr3);
        assert((chr2 & 0x80) == 0x80);
        write(chr2);
        assert((chr1 & 0x80) == 0x00);
        write(chr1);
      } else {
        assert((chr3 & 0x80) == 0x80);
        write(chr3);
        assert((chr2 & 0x80) == 0x80);
        write(chr2);
        assert((chr1 & 0x80) == 0x00);
        write(chr1);
      }
    } else {
      assert((chr2 & 0x80) == 0x80);
      write(chr2);
      assert((chr1 & 0x80) == 0x00);
      write(chr1);
    }
  } else {
    assert((chr1 & 0x80) == 0x00);
    write(chr1);
  }
}

auto MTrk::writeTickDelta() -> void {
  writeVarint(tick_);

  tick_ = 0;
}

auto MTrk::meta(uint7 event, const array_view<uint8_t> data) -> void {
  writeTickDelta();

  write(0xFF);
  write(event);

  writeVarint(data.size());
  write(data);

  flush();
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
  writeTickDelta();
  write(0x80 | channel);
  write(note);
  write(velocity);

  flush();
}

auto MTrk::noteOn(uint4 channel, uint7 note, uint7 velocity) -> void {
  writeTickDelta();
  write(0x90 | channel);
  write(note);
  write(velocity);

  flush();
}

auto MTrk::keyPressureChange(uint4 channel, uint7 note, uint7 velocity) -> void {
  writeTickDelta();
  write(0xA0 | channel);
  write(note);
  write(velocity);

  flush();
}

auto MTrk::controlChange(uint4 channel, uint7 control, uint7 value) -> void {
  if (file.channels[channel].control[control]
    && file.channels[channel].control[control]() == value) return;

  writeTickDelta();
  write(0xB0 | channel);
  write(control);
  write(value);

  flush();

  file.channels[channel].control[control] = value;
}

auto MTrk::programChange(uint4 channel, uint7 program) -> void {
  if (file.channels[channel].program
    && file.channels[channel].program() == program) return;

  writeTickDelta();
  write(0xC0 | channel);
  write(program);

  flush();

  file.channels[channel].program = program;
}

auto MTrk::channelPressureChange(uint4 channel, uint7 velocity) -> void {
  writeTickDelta();
  write(0xD0 | channel);
  write(velocity);

  flush();
}

auto MTrk::pitchBendChange(uint4 channel, uint14 pitchBend) -> void {
  if (file.channels[channel].pitchBend
    && file.channels[channel].pitchBend() == pitchBend) return;

  writeTickDelta();
  write(0xE0 | channel);
  write(pitchBend & 0x7F);
  write((pitchBend >> 7) & 0x7F);

  flush();

  file.channels[channel].pitchBend = pitchBend;
}
