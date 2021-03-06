namespace Heuristics {

struct MegaDrive {
  MegaDrive(vector<uint8_t>& data, string location);
  explicit operator bool() const;
  auto manifest() const -> string;

private:
  vector<uint8_t>& data;
  string location;
};

MegaDrive::MegaDrive(vector<uint8_t>& data, string location) : data(data), location(location) {
}

MegaDrive::operator bool() const {
  return data.size() >= 0x200;
}

auto MegaDrive::manifest() const -> string {
  if(!operator bool()) return {};

  string ramMode = "none";

  uint32_t ramFrom = 0;
  ramFrom |= data[0x01b4] << 24;
  ramFrom |= data[0x01b5] << 16;
  ramFrom |= data[0x01b6] <<  8;
  ramFrom |= data[0x01b7] <<  0;

  uint32_t ramTo = 0;
  ramTo |= data[0x01b8] << 24;
  ramTo |= data[0x01b9] << 16;
  ramTo |= data[0x01ba] <<  8;
  ramTo |= data[0x01bb] <<  0;

  if(!(ramFrom & 1) && !(ramTo & 1)) ramMode = "hi";
  if( (ramFrom & 1) &&  (ramTo & 1)) ramMode = "lo";
  if(!(ramFrom & 1) &&  (ramTo & 1)) ramMode = "word";
  if(data[0x01b0] != 'R' || data[0x01b1] != 'A') ramMode = "none";

  uint32_t ramSize = ramTo - ramFrom + 1;
  if(ramMode == "hi") ramSize = (ramTo >> 1) - (ramFrom >> 1) + 1;
  if(ramMode == "lo") ramSize = (ramTo >> 1) - (ramFrom >> 1) + 1;
  if(ramMode == "word") ramSize = ramTo - ramFrom + 1;
  if(ramMode != "none") ramSize = bit::round(min(0x20000, ramSize));
  if(ramMode == "none") ramSize = 0;

  vector<string> regions;
  string region = slice((const char*)&data[0x1f0], 0, 16).trimRight(" ");
  if(!regions) {
    if(region == "JAPAN" ) regions.append("NTSC-J");
    if(region == "EUROPE") regions.append("PAL");
  }
  if(!regions) {
    if(region.find("J")) regions.append("NTSC-J");
    if(region.find("U")) regions.append("NTSC-U");
    if(region.find("E")) regions.append("PAL");
    if(region.find("W")) regions.append("NTSC-J", "NTSC-U", "PAL");
  }
  if(!regions && region.size() == 1) {
    uint8_t field = region.hex();
    if(field & 0x01) regions.append("NTSC-J");
    if(field & 0x04) regions.append("NTSC-U");
    if(field & 0x08) regions.append("PAL");
  }
  if(!regions) {
    regions.append("NTSC-J");
  }

  string domesticName;
  domesticName.resize(48);
  memory::copy(domesticName.get(), &data[0x0120], domesticName.size());
  for(auto& c : domesticName) if(c < 0x20 || c > 0x7e) c = ' ';
  while(domesticName.find("  ")) domesticName.replace("  ", " ");
  domesticName.strip();

  string internationalName;
  internationalName.resize(48);
  memory::copy(internationalName.get(), &data[0x0150], internationalName.size());
  for(auto& c : internationalName) if(c < 0x20 || c > 0x7e) c = ' ';
  while(internationalName.find("  ")) internationalName.replace("  ", " ");
  internationalName.strip();

  string output;
  output.append("game\n");
  output.append("  sha256: ", Hash::SHA256(data).digest(), "\n");
  output.append("  label:  ", Location::prefix(location), "\n");
  output.append("  name:   ", Location::prefix(location), "\n");
  output.append("  title:  ", domesticName, "\n");
  output.append("  region: ", regions.left(), "\n");
  output.append("  board\n");
  if(domesticName == "Game Genie") {
    output.append(Memory{}.type("ROM").size(data.size()).content("Program").text());
    output.append(Slot{}.type("MegaDrive").text());
  } else if(domesticName == "SONIC & KNUCKLES") {
    output.append(Memory{}.type("ROM").size(0x200000).content("Program").text());
    output.append(Memory{}.type("ROM").size( 0x40000).content("Patch").text());
    output.append(Slot{}.type("MegaDrive").text());
  } else {
    output.append(Memory{}.type("ROM").size(data.size()).content("Program").text());
  }
  if(ramSize && ramMode != "none") {
    output.append(Memory{}.type("RAM").size(ramSize).content("Save").text());
    output.append("      mode: ", ramMode, "\n");
    output.append("      offset: 0x", hex(ramFrom), "\n");
  }
  return output;
}

}
