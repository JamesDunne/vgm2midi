#pragma once

namespace Emulator {

struct Platform {
  struct Load {
    Load() = default;
    Load(uint pathID, string option = "") : valid(true), pathID(pathID), option(option) {}
    explicit operator bool() const { return valid; }

    bool valid = false;
    uint pathID = 0;
    string option;
  };

  virtual auto path(uint id) -> string { return ""; }
  virtual auto open(uint id, string name, vfs::file::mode mode, bool required = false) -> vfs::shared::file { return {}; }
  virtual auto load(uint id, string name, string type, vector<string> options = {}) -> Load { return {}; }
  virtual auto videoRefresh(uint display, const uint32* data, uint pitch, uint width, uint height) -> void {}
  virtual auto audioSample(const double* samples, uint channels) -> void {}
  virtual auto inputPoll(uint port, uint device, uint input) -> int16 { return 0; }
  virtual auto inputRumble(uint port, uint device, uint input, bool enable) -> void {}
  virtual auto dipSettings(Markup::Node node) -> uint { return 0; }
  virtual auto notify(string text) -> void {}

  virtual auto createMIDITrack() -> shared_pointer<MIDITrack> { return new MIDITrack::Null; }
};

extern Platform* platform;

}
