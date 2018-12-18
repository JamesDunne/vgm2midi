
auto MIDIMelodic::midiNoteOff() -> void {
  if (!lastMidiNote || !lastMidiChannel) return;

  midi->noteOff(lastMidiChannel(), lastMidiNote(), 0);
}

auto MIDIMelodic::midiNoteOn() -> void {
  const int wheel_threshold = 384;

  auto n = midiNote();
  auto m = round(n);
  auto newMidiChannel = midiChannel();
  auto newChannelVolume = midiChannelVolume();

  if (!lastMidiNote || m != round(lastMidiNote())) {
    midiNoteOff();

    // Update channel volume:
    midi->controlChange(newMidiChannel, 0x07, newChannelVolume);

    if (midiPitchBendEnabled()) {
      // Reset pitch bend if within a tolerance of a concert pitch:
      if (abs(n - m) < 0.0625) {
        // Reset pitch bend to 0:
        midi->pitchBendChange(newMidiChannel, 0x2000);
      }
    }

    // Note ON:
    midi->noteOn(newMidiChannel, m, midiNoteVelocity());
    lastMidiChannel = newMidiChannel;
    lastMidiNote = m;
  } else if (lastMidiChannel) {
    // Update last channel played on's volume since we don't really support switching
    // duty cycle without restarting the note (i.e. playing it across multiple channels).
    midi->controlChange(lastMidiChannel(), 0x07, newChannelVolume);
  }

  if (midiPitchBendEnabled() && lastMidiNote) {
    // Period is changing too finely for MIDI note to change:
    if (abs(n - lastMidiNote()) >= 0.0625) {
      // Emit pitch wheel change:
      auto newPitchBend = midiPitchBend(n);
      midi->pitchBendChange(lastMidiChannel(), newPitchBend);
    }
  }
}
