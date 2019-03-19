
auto MIDIMelodic::midiNoteOff() -> void {
  if (!lastMidiNote || !lastMidiChannel) return;

  midi->noteOff(lastMidiChannel(), round(lastMidiNote()), 0);

  lastMidiNote = nothing;
}

auto MIDIMelodic::midiNoteOn() -> void {
  auto n = midiNote();
  auto m = round(n);
  auto newMidiChannel = midiChannel();

  // Rate limit note on of the same note:
  if (lastMidiNote && lastMidiChannel
    && (lastMidiChannel() == newMidiChannel)
    && ((round(lastMidiNote()) == m) && (midi->tick() - lastMidiNoteTick() < 0x30))
  ) {
    return;
  }

  if (lastMidiNote) {
    midiNoteOff();
  }

  if (m < 0) return;

  auto newChannelVolume = midiChannelVolume();

  // Update channel volume:
  midi->controlChange(newMidiChannel, 0x07, newChannelVolume);

  if (newMidiChannel != 9) {
    // Reset pitch bend if within a tolerance of a concert pitch:
    auto pitchDiff = abs(n - m);
    auto newPitchBend = midiPitchBend(n, m);
    if (pitchDiff < 0.0625) {
      // Reset pitch bend to 0:
      newPitchBend = 0x2000;
      lastPitch = m;
    } else {
      lastPitch = n;
    }
    midi->pitchBendChange(newMidiChannel, newPitchBend);
  }

  // Change program:
  midi->programChange(newMidiChannel, midiProgram());

  // Note ON:
  midi->noteOn(newMidiChannel, m, midiNoteVelocity());

  lastMidiNoteTick = midi->tick();
  lastMidiChannel = newMidiChannel;
  lastMidiNote = n;
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
    auto m = round(lastMidiNote());
    auto wheel = abs(n - m);
    auto wheelDiff = abs(lastPitch() - n);

    // Start a new note if pitch went too far out of range:
    if (wheel >= 2.0 || wheelDiff > 1.0) {
      midiNoteOn();
    } else if (wheelDiff >= 0.0625) {
      // Adjust pitch wheel:
      auto newPitchBend = midiPitchBend(n, m);
      midi->pitchBendChange(lastMidiChannel(), newPitchBend);
      lastPitch = n;
    }
  }
}
