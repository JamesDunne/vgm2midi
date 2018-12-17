
auto MIDIMelodic::midiNoteOff() -> void {
}

auto MIDIMelodic::midiNoteOn() -> void {
#if 0
  auto m = midiNote();
  auto new_midi_channel = midiChannel();
  auto new_channel_volume = midiChannelVolume();

  if (m != lastMidiNote) {
    midiNoteOff();

    if (new_channel_volume != last_midi_channel_volume[new_midi_channel]) {
      midi.control(new_midi_channel, 0x07, new_channel_volume);
      last_midi_channel_volume[new_midi_channel] = new_channel_volume;
    }

    note_on_period = period();
    if (midi_pitch_wheel_enabled()) {
      if (abs(period_cents[note_on_period]-0x2000) < wheel_threshold &&
        last_wheel_emit[new_midi_channel] != 0x2000)
      {
        // Reset pitch wheel to 0:
        midi_write_pitch_wheel(new_midi_channel, 0x2000);
      }
    }
    midi_write_note_on();
    last_midi_channel = new_midi_channel;
  } else {
    if (new_channel_volume != last_midi_channel_volume[last_midi_channel]) {
      // Update last channel played on's volume since we don't really support switching
      // duty cycle without restarting the note (i.e. playing it across multiple channels).
      midi_write_channel_volume(last_midi_channel);
      last_midi_channel_volume[last_midi_channel] = new_channel_volume;
    }
  }

  if (midi_pitch_wheel_enabled()) {
    // Period is changing too finely for MIDI note to change:
    int wheel = period_cents[period()];
    if (abs(wheel-0x2000) >= wheel_threshold ||
      abs(period_cents[note_on_period]-wheel) >= wheel_threshold)
    {
      if (last_wheel_emit[last_midi_channel] != wheel) {
        // Emit pitch wheel change:
        midi_write_pitch_wheel(last_midi_channel, wheel);
      }
    }
  }

  lastMidiNote = m;
#endif
}

auto MIDIRhythmic::midiNoteOff() -> void {
}

auto MIDIRhythmic::midiNoteOn() -> void {
}
