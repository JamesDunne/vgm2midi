auto APU::Envelope::volume() const -> uint {
  return useSpeedAsVolume ? speed : decayVolume;
}

auto APU::Envelope::clock() -> void {
  if(reloadDecay) {
    midiTrigger = true;
    reloadDecay = false;
    decayVolume = 0x0f;
    decayCounter = speed + 1;
    return;
  }

  if(--decayCounter == 0) {
    decayCounter = speed + 1;
    if(decayVolume || loopMode) decayVolume--;
  }
}

auto APU::Envelope::power() -> void {
  speed = 0;
  useSpeedAsVolume = 0;
  loopMode = 0;
  reloadDecay = 0;
  decayCounter = 0;
  decayVolume = 0;
  midiTrigger = false;
}

auto APU::Envelope::midiVolume() -> uint7 {
  auto x = volume();

  if (x == 0) return 0;
  return 15 + (uint)((112.0/0.15)*95.88/((8128.0/x)+100.0));
}
