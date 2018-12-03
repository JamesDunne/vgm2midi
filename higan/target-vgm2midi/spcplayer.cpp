
#include <sfc/sfc.hpp>

struct SPCPlayer : Emulator::Platform {
	auto run(string filename, Arguments arguments) -> void;

	// Hard-coded manifest.bml for Famicom:
	string nes_sys_manifest = "system name:Famicom";

	// Generated manfiest and supporting data for NSF file:
	string manifest;
	vector<uint8_t> prgrom;

	// WAVE file writing out:
	file_buffer wave;
	long samples;

	SuperFamicom::Interface* snes;

	SuperFamicom::CPU* cpu;
	SuperFamicom::SMP* smp;
	SuperFamicom::DSP* dsp;
	SuperFamicom::System* system;
	SuperFamicom::Scheduler* scheduler;

	// Emulator::Platform
	auto path(uint id) -> string override;
	auto open(uint id, string name, vfs::file::mode mode, bool required) -> vfs::shared::file override;
	auto load(uint id, string name, string type, vector<string> options = {}) -> Emulator::Platform::Load override;
	auto videoRefresh(uint display, const uint32* data, uint pitch, uint width, uint height) -> void override;
	auto audioSample(const double* samples, uint channels) -> void override;
	auto inputPoll(uint port, uint device, uint input) -> int16 override;
	auto inputRumble(uint port, uint device, uint input, bool enable) -> void override;
	auto dipSettings(Markup::Node node) -> uint override;
	auto notify(string text) -> void override;
};

auto SPCPlayer::path(uint id) -> string {
	return "";
}
auto SPCPlayer::open(uint id, string name, vfs::file::mode mode, bool required) -> vfs::shared::file {
	// print("platform::open({0}, {1})\n", string_format{id, name});

	if (id == 0) {  //System
		// print("platform::open  id = System\n");
		if (name == "manifest.bml" && mode == vfs::file::mode::read) {
			// print("platform::open  manifest.bml from memory\n");
			return vfs::memory::file::open(nes_sys_manifest.data(), nes_sys_manifest.size());
		}
	}

	if (id == 1) {  //Famicom
		// print("platform::open  id = Famicom\n");
		if(name == "manifest.bml" && mode == vfs::file::mode::read) {
			// Load the manifest generated from the NSF file:
			// print("platform::open  manifest.bml from memory\n");
			return vfs::memory::file::open(manifest.data<uint8_t>(), manifest.size());
		} else if (name == "program.rom" && mode == vfs::file::mode::read) {
			// print("platform::open  program.rom from memory\n");
			return vfs::memory::file::open(prgrom.data<uint8_t>(), prgrom.size());
		}
	}

	if (required) {
		print("platform::open  Missing required file {0}\n", string_format{name});
	}

	return {};
}
auto SPCPlayer::load(uint id, string name, string type, vector<string> options) -> Emulator::Platform::Load {
	// print("load({0}, {1})\n", string_format{id, name});
	return {id, "NTSC-U"};
}
auto SPCPlayer::videoRefresh(uint display, const uint32* data, uint pitch, uint width, uint height) -> void {
	// print("videoRefresh\n");
}
auto SPCPlayer::audioSample(const double* samples, uint channels) -> void {
	// if (!nsf->playing) return;

	// For SPC:
	assert(channels == 2);

	// Write 32-bit sample:
	auto x = (float)samples[0];
	auto y = (float)samples[1];
	wave.write({&x, 4});
	wave.write({&y, 4});
	this->samples++;
}
auto SPCPlayer::inputPoll(uint port, uint device, uint input) -> int16 {
	print("inputPoll\n");
	return 0;
}
auto SPCPlayer::inputRumble(uint port, uint device, uint input, bool enable) -> void {}
auto SPCPlayer::dipSettings(Markup::Node node) -> uint {
	return 0;
}
auto SPCPlayer::notify(string text) -> void {
	print("notify(\"{0}\")\n", string_format{text});
}

auto SPCPlayer::run(string filename, Arguments arguments) -> void {
	auto buf = file::open(filename, file::mode::read);
	if (buf.reads(5) != "NESM\x1A") {
		print("Missing NESM header for SPC!\n");
		return;
	}

	auto song_name = "";
	vector<uint8_t> prgrom;

	// for (auto b: prgrom) {
	// 	print("{0} ", string_format{hex(b, 2)});
	// }
	// print("\n");

	buf.close();

	// Build a temporary manifest for cartridge to load:
	manifest = "";
	manifest.append("game\n");
	manifest.append("  sha256: ", Hash::SHA256(prgrom).digest(), "\n");
	manifest.append("  label:  ", song_name, "\n");
	manifest.append("  name:   ", filename, "\n");

	manifest.append("  board:  NSF\n");
	manifest.append("    mirror mode=", "horizontal", "\n");

	manifest.append("    memory\n");
	manifest.append("      type: ", "ROM", "\n");
	manifest.append("      size: 0x", hex(prgrom.size()), "\n");
	manifest.append("      content: ", "Program", "\n");

	// print(manifest, "\n");

	// print("Song:      ", song_name, "\n");
	// print("Artist:    ", artist_name, "\n");
	// print("Copyright: ", copyright_name, "\n");
	// print("song count: {0}, start: {1}\n", string_format{song_count, start_song});

	Emulator::audio.setFrequency(48000);
	Emulator::audio.setVolume(1.0);
	Emulator::audio.setBalance(0.0);

	snes = new SuperFamicom::Interface;
	// print("snes->load()\n");
	if (!snes->load()) {
		print("SNES failed load()\n");
		return;
	}
	// print("snes->power()\n");
	snes->power();

	cpu = &SuperFamicom::cpu;
	smp = &SuperFamicom::smp;
	dsp = &SuperFamicom::dsp;
	system = &SuperFamicom::system;
	scheduler = &SuperFamicom::scheduler;

	// Disable PPU rendering to save performance:
	// ppu->disabled = true;

	// print("Region: {0}\n", string_format{(int)system.region()});

	const int header_size = 0x2C;

	wave = file::open("out.wav", file::mode::write);
	wave.truncate(header_size);
	wave.seek(header_size);
	samples = 0;

	const long play_seconds = 3 * 60 + 15;

	int plays = 0;
	do
	{
#if DEBUG_NSF
		print("pc = {0}, a = {1}, x = {2}, y = {3}, s = {4}\n", string_format{
		     hex(cpu->r.pc, 4),
		     hex(cpu->r.a, 2),
		     hex(cpu->r.x, 2),
		     hex(cpu->r.y, 2),
		     hex(cpu->r.s, 2)
		});
#endif
		if (scheduler->enter(Emulator::Scheduler::Mode::SynchronizeMaster) == Emulator::Scheduler::Event::Frame) {
			plays++;
		}
	} while (plays <= 60 * play_seconds);

	// Write WAVE headers:
	long chan_count = 2;
	long rate = 48000;
	long ds = samples * sizeof (float);
	long rs = header_size - 8 + ds;
	int frame_size = chan_count * sizeof (float);
	long bps = rate * frame_size;

	unsigned char header [header_size] =
	{
		'R','I','F','F',
		rs,rs>>8,           // length of rest of file
		rs>>16,rs>>24,
		'W','A','V','E',
		'f','m','t',' ',
		0x10,0,0,0,         // size of fmt chunk
		// 1,0,                // PCM format
		3,0,				// float format
		chan_count,0,       // channel count
		rate,rate >> 8,     // sample rate
		rate>>16,rate>>24,
		bps,bps>>8,         // bytes per second
		bps>>16,bps>>24,
		frame_size,0,       // bytes per sample frame
		32,0,               // bits per sample
		'd','a','t','a',
		ds,ds>>8,ds>>16,ds>>24// size of sample data
		// ...              // sample data
	};

	wave.seek(0);
	wave.write({header, header_size});

	wave.close();
}