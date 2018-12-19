
auto MIDIMelodic::midiNoteOff() -> void {
  if (!lastMidiNote || !lastMidiChannel) return;

  midi->noteOff(lastMidiChannel(), lastMidiNote(), 0);

  lastMidiNote = nothing;
}

auto MIDIMelodic::midiNoteOn() -> void {
  auto n = midiNote();
  auto m = round(n);
  auto newMidiChannel = midiChannel();
  auto newChannelVolume = midiChannelVolume();

  if (!lastMidiNote || m != lastMidiNote()) {
    midiNoteOff();

    if (m >= 0) {
      // Update channel volume:
      midi->controlChange(newMidiChannel, 0x07, newChannelVolume);

      if (newMidiChannel != 9) {
        // Reset pitch bend if within a tolerance of a concert pitch:
        if (abs(n - m) < 0.0625) {
          // Reset pitch bend to 0:
          midi->pitchBendChange(newMidiChannel, 0x2000);
        }
      }

      // Change program:
      midi->programChange(newMidiChannel, midiProgram());

      // Note ON:
      midi->noteOn(newMidiChannel, m, midiNoteVelocity());

      lastMidiNoteTick = midi->tick();
      lastMidiChannel = newMidiChannel;
      lastMidiNote = m;
    }
  } else if (lastMidiChannel) {
    // Update last channel played on's volume since we don't really support switching
    // duty cycle without restarting the note (i.e. playing it across multiple channels).
    midi->controlChange(lastMidiChannel(), 0x07, newChannelVolume);
  }

  if (lastMidiChannel() != 9 && lastMidiNote) {
    // Period is changing too finely for MIDI note to change:
    if (abs(n - lastMidiNote()) >= 0.0625) {
      // Emit pitch wheel change:
      auto newPitchBend = midiPitchBend(n);
      midi->pitchBendChange(lastMidiChannel(), newPitchBend);
    }
  }
}
