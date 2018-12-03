#include "vgm2midi.hpp"
#include <fc/fc.hpp>

::Emulator::Interface* emulator = nullptr;

struct Program : Emulator::Platform {
  Program(Arguments arguments);
  auto main() -> void;

  Arguments arguments;
  bool initializing;

  // Hard-coded manifest.bml for Famicom:
  string nes_sys_manifest = "system name:Famicom";

  Famicom::Interface* nes;

  // Generated manfiest and supporting data for NSF file:
  string manifest;
  vector<uint8_t> prgrom;

  // WAVE file writing out:
  file_buffer wave;
  long samples;

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

Program::Program(Arguments arguments) : arguments(arguments) {
	platform = this;
}

auto Program::path(uint id) -> string {
	return "";
}
auto Program::open(uint id, string name, vfs::file::mode mode, bool required) -> vfs::shared::file {
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
auto Program::load(uint id, string name, string type, vector<string> options) -> Emulator::Platform::Load {
	// print("load({0}, {1})\n", string_format{id, name});
	return {id, "NTSC-U"};
}
auto Program::videoRefresh(uint display, const uint32* data, uint pitch, uint width, uint height) -> void {
	// print("videoRefresh\n");
}
auto Program::audioSample(const double* samples, uint channels) -> void {
	if (initializing) return;

	// For NSF:
	assert(channels == 1);

	// Write 32-bit sample:
	auto x = (float)samples[0];
	wave.write({&x, 4});
	this->samples++;
}
auto Program::inputPoll(uint port, uint device, uint input) -> int16 {
	print("inputPoll\n");
	return 0;
}
auto Program::inputRumble(uint port, uint device, uint input, bool enable) -> void {}
auto Program::dipSettings(Markup::Node node) -> uint {
	return 0;
}
auto Program::notify(string text) -> void {
	print("notify(\"{0}\")\n", string_format{text});
}

// Main:
auto Program::main() -> void {
	auto filename = arguments[0];
	if (!filename) {
		print("Missing filename\n");
		return;
	}
	auto buf = file::open(filename, file::mode::read);
	if (buf.reads(5) != "NESM\x1A") {
		print("Missing NESM header for NSF!\n");
		return;
	}
	if (buf.readl(1) != 0x01) {
		print("Bad NESM version!\n");
		return;
	}
	auto song_count = buf.readl(1);
	auto start_song = buf.readl(1);
	uint16_t addr_load = buf.readl(2);
	uint16_t addr_init = buf.readl(2);
	uint16_t addr_play = buf.readl(2);
	auto song_name = buf.reads(32);
	auto artist_name = buf.reads(32);
	auto copyright_name = buf.reads(32);
	auto ntsc_play_speed = buf.readl(2);
	vector<uint8_t> bankswitch_init;
	bankswitch_init.resize(8);
	buf.read(bankswitch_init);
	auto pal_play_speed = buf.readl(2);
	auto pal_ntsc = buf.readl(1);
	auto extra_sound_support = buf.readl(1);
	buf.reads(4);

	// We've read the entire header now:
	assert(buf.offset() == 0x80);

	// Build a PRGROM vector:
	prgrom.resize(0x8000);
	prgrom.fill(0xFF);

	// Determine if bank switching is used:
	auto bankswitch_enabled = false;
	for (auto bs : bankswitch_init) {
		if (bs != 0x00) {
			bankswitch_enabled = true;
			break;
		}
	}

	if (!bankswitch_enabled) {
		// No bank switching, just load data straight into PRGROM at addr_load:
		// print("addr_load: {0}\n", string_format{hex(addr_load - 0x8000, 4)});
		buf.read({prgrom.data<uint8_t>() + (addr_load - 0x8000), buf.size() - buf.offset()});
		// for (auto b: prgrom) {
		// 	print("{0} ", string_format{hex(b, 2)});
		// }
		// print("\n");
	} else {
		// TODO: bank switching!
		print("TODO: bank switching!\n");
	}

	buf.close();

	// Build a temporary manifest for cartridge to load:
	manifest = "";
	manifest.append("game\n");
	manifest.append("  sha256: ", Hash::SHA256(prgrom).digest(), "\n");
	manifest.append("  label:  ", song_name, "\n");
	manifest.append("  name:   ", filename, "\n");

	manifest.append("  board:  NSF\n");
	manifest.append("    mirror mode=", "horizontal", "\n");
	manifest.append("    nsf\n");
	manifest.append("      init: 0x", hex(addr_init,4), "\n");
	manifest.append("      play: 0x", hex(addr_play,4), "\n");

	manifest.append("    memory\n");
	manifest.append("      type: ", "ROM", "\n");
	manifest.append("      size: 0x", hex(prgrom.size()), "\n");
	manifest.append("      content: ", "Program", "\n");
	// if(_manufacturer)
	// 	output.append("      manufacturer: ", _manufacturer, "\n");
	// if(_architecture)
	// 	output.append("      architecture: ", _architecture, "\n");
	// if(_identifier)
	// 	output.append("      identifier: ", _identifier, "\n");
	// if(_volatile)
	// 	output.append("      volatile\n");

	print(manifest, "\n");

	print(song_name, "\n");
	print(artist_name, "\n");
	print(copyright_name, "\n");
	print("count: {0}, start: {1}\n", string_format{song_count, start_song});
	print("bank switching: {0}\n", string_format{bankswitch_enabled});

	Emulator::audio.setFrequency(48000);
	Emulator::audio.setVolume(1.0);
	Emulator::audio.setBalance(0.0);

	nes = new Famicom::Interface;
	// print("nes->load()\n");
	if (!nes->load()) {
		print("NES failed load()\n");
		return;
	}
	// print("nes->power()\n");
	nes->power();

	auto& cpu = Famicom::cpu;
	auto& apu = Famicom::apu;
	auto& ppu = Famicom::ppu;
	auto& system = Famicom::system;
	auto& scheduler = Famicom::scheduler;

	// Disable PPU rendering to save performance:
	ppu.disabled = true;

	// print("Region: {0}\n", string_format{(int)system.region()});

	const int header_size = 0x2C;

	wave = file::open("out.wav", file::mode::write);
	wave.truncate(header_size);
	wave.seek(header_size);
	samples = 0;

	initializing = true;

	// Clear RAM to 00:
	for (auto& data : cpu.ram) data = 0x00;

#if 0
	// Push a return address on stack that points to a HLT instruction:
	const int badop_addr = 0x8000;
	cpu.ram[0x1FF] = (badop_addr - 1) >> 8;
	cpu.ram[0x1FE] = (badop_addr - 1) & 0xFF;
	cpu.r.s = 0xFD;
	// Start at INIT routine:
	cpu.r.pc = addr_init;
	// A = song index
	cpu.r.a = 5;
	// X = PAL or NTSC
	cpu.r.x = 0;
#endif

	// Reset APU:
	for (auto addr : range(0x4000, 0x4014)) {
		apu.writeIO(addr, 0x00);
	}
	// clear channels:
	apu.writeIO(0x4015, 0x00);
	apu.writeIO(0x4015, 0x0F);
	// 4-step mode:
	apu.writeIO(0x4017, 0x40);

	const long cpu_rate = cpu.rate();
	const long ppu_step = (ppu.vlines() * 341L * ppu.rate() - 2) / (cpu_rate * 2);

#if 0
	do
	{
		scheduler.enter(Famicom::Scheduler::Mode::SynchronizeMaster);
	} while (cpu.r.pc != badop_addr);

	// Start the PLAY routine:
	cpu.ram[0x1FF] = (badop_addr - 1) >> 8;
	cpu.ram[0x1FE] = (badop_addr - 1) & 0xFF;
	cpu.r.s = 0xFD;
	cpu.r.pc = addr_play;
#endif

	int cycles = 0;
	int plays = 0;

	// const long play_sec = (60 * 3 + 10);
	const long play_sec = 10;

	print("initialized\n");
	initializing = false;

	for (int i = 0; i < 10; i++)
	{
		ppu.step(ppu_step);

		// if (cpu.r.pc == badop_addr)
		// {
		// 	plays++;
		// 	cpu.r.pc = addr_play;
		// 	cpu.ram[0x100 + cpu.r.s--] = (badop_addr - 1) >> 8;
		// 	cpu.ram[0x100 + cpu.r.s--] = (badop_addr - 1) & 0xFF;
		// }
	} // while (plays < (1'000'000.0 / ntsc_play_speed) /*Hz*/ * play_sec /*sec*/);

	// Write WAVE headers:
	long chan_count = 1;
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

#include <nall/main.hpp>
auto nall::main(Arguments arguments) -> void {
	auto program = new Program(arguments);
	program->main();
}
