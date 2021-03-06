auto System::serialize() -> serializer {
  serializer s(_serializeSize);

  uint signature = 0x31545342;
  char version[16] = {0};
  char description[512] = {0};
  memory::copy(&version, (const char*)Emulator::SerializerVersion, Emulator::SerializerVersion.size());

  s.integer(signature);
  s.array(version);
  s.array(description);

  serializeAll(s);
  return s;
}

auto System::unserialize(serializer& s) -> bool {
  uint signature;
  char version[16];
  char description[512];

  s.integer(signature);
  s.array(version);
  s.array(description);

  if(signature != 0x31545342) return false;
  if(string{version} != Emulator::SerializerVersion) return false;

  power(/* reset = */ false);
  serializeAll(s);
  return true;
}

auto System::serialize(serializer& s) -> void {
}

auto System::serializeAll(serializer& s) -> void {
  system.serialize(s);
  cartridge.serialize(s);
  cpu.serialize(s);
  apu.serialize(s);
  ppu.serialize(s);
  controllerPort1.serialize(s);
  controllerPort2.serialize(s);
}

auto System::serializeInit() -> void {
  serializer s;

  uint signature = 0;
  char version[16];
  char description[512];

  s.integer(signature);
  s.array(version);
  s.array(description);

  serializeAll(s);
  _serializeSize = s.size();
}
