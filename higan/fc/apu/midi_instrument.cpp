
auto MIDIMelodic::midiNoteOff() -> void {
  if (!lastMidiNote || !lastMidiChannel) return;

  midi->noteOff(lastMidiChannel(), lastMidiNote(), 0);

  lastMidiNote = nothing;
}

auto MIDIMelodic::midiNoteOn() -> void {
  auto n = midiNote();
  auto m = round(n);

  // Rate limit note on of the same note:
  if (lastMidiNote && ((lastMidiNote() == m) && (midi->tick() - lastMidiNoteTick() < 0x30))) {
    return;
  }

  if (lastMidiNote) {
    midiNoteOff();
  }

  if (m < 0) return;

  auto newMidiChannel = midiChannel();
  auto newChannelVolume = midiChannelVolume();

  // Update channel volume:
  midi->controlChange(newMidiChannel, 0x07, newChannelVolume);

  if (newMidiChannel != 9) {
    // Reset pitch bend if within a tolerance of a concert pitch:
    auto pitchDiff = abs(n - m);
    if (pitchDiff < 0.0625) {
      // Reset pitch bend to 0:
      midi->pitchBendChange(newMidiChannel, 0x2000);
    } else {
      auto newPitchBend = midiPitchBend(n, m);
      midi->pitchBendChange(newMidiChannel, newPitchBend);
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

auto MIDIMelodic::midiNoteContinue() -> void {
  if (!lastMidiChannel || !lastMidiNote) return;

  // Update last channel played on's volume since we don't really support switching
  // duty cycle without restarting the note (i.e. playing it across multiple channels).
  auto newChannelVolume = midiChannelVolume();
  midi->controlChange(lastMidiChannel(), 0x07, newChannelVolume);

  // Update pitch wheel:
  if (lastMidiChannel() != 9) {
    auto n = midiNote();
    auto m = lastMidiNote();

    // Period is changing too finely for MIDI note to change:
    auto pitchDiff = abs(n - m);
    if (pitchDiff >= 0.0625 && pitchDiff < 0.925) {
      // Emit pitch wheel change:
      auto newPitchBend = midiPitchBend(n, m);
      midi->pitchBendChange(lastMidiChannel(), newPitchBend);
    } else if (pitchDiff >= 0.925) {
      // Too far outside bend range so start a new note:
      midiNoteOn();
    }
  }
}
