
#include <sfc/sfc.hpp>

struct SPCPlayer : Emulator::Platform {
	auto run(string filename, Arguments arguments) -> void;

	// Generated manfiest and supporting data for SPC file:
	string manifest;
	vector<uint8_t> spcregs;
	vector<uint8_t> dspram;
	vector<uint8_t> dspregs;
	vector<uint8_t> iplrom;

	// WAVE file writing out:
	file_buffer wave;
	long samples;

	SuperFamicom::Interface* snes;

	SuperFamicom::CPU* cpu;
	SuperFamicom::PPU* ppu;
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
		if (name == "boards.bml" && mode == vfs::file::mode::read) {
			return vfs::memory::file::open(Resource::System::Boards, sizeof(Resource::System::Boards));
		}

		if (name == "ipl.rom" && mode == vfs::file::mode::read) {
			return vfs::memory::file::open(Resource::System::IPLROM, sizeof(Resource::System::IPLROM));
		}
	}

	if (id == 1) {  //Super Famicom
		// print("platform::open  id = Super Famicom\n");
		if (name == "manifest.bml" && mode == vfs::file::mode::read) {
			// Load the manifest generated from the SPC file:
			// print("platform::open  manifest.bml from memory\n");
			return vfs::memory::file::open(manifest.data<uint8_t>(), manifest.size());
		} else if (name == "program.rom" && mode == vfs::file::mode::read) {
			// print("platform::open  program.rom from memory\n");
			return vfs::memory::file::open(0, 0);
		} else {
			print("platform::open null response\n");
			return {};
		}
	}

	if (required) {
		print("platform::open  Missing required file {0}\n", string_format{name});
	}

	return {};
}
auto SPCPlayer::load(uint id, string name, string type, vector<string> options) -> Emulator::Platform::Load {
	// print("load({0}, {1})\n", string_format{id, name});
	return {id, "NTSC"};
}
auto SPCPlayer::videoRefresh(uint display, const uint32* data, uint pitch, uint width, uint height) -> void {
	// print("videoRefresh\n");
}
auto SPCPlayer::audioSample(const double* samples, uint channels) -> void {
	// print("audioSample channels={0}\n", string_format{channels});

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
	if (buf.reads(33+2) != "SNES-SPC700 Sound File Data v0.30\x1A\x1A") {
		print("Missing header for SPC!\n");
		return;
	}
	auto hasID666 = buf.read() == 26;
	auto spcVersion = buf.read();
	if (spcVersion != 30) {
		print("SPC version not 30\n");
		return;
	}

	assert(buf.offset() == 0x25);
	// Read SPC700 CPU registers:
	spcregs.resize(0x2C - 0x25);
	buf.read(spcregs);

	// Jump to real data:
	buf.seek(0x100);

	// An SPC dump is just a dump of SPC ram at the time of song init:
	dspram.resize(0x10000);
	dspram.fill(0x00);
	buf.read(dspram);

	// And the SPC700 DSP registers:
	dspregs.resize(0x80);
	dspregs.fill(0x00);
	buf.read(dspregs);

	buf.seek(64, file_buffer::index::relative);
	iplrom.resize(64);
	buf.read(iplrom);

	auto song_name = "";

	// for (auto b: prgrom) {
	// 	print("{0} ", string_format{hex(b, 2)});
	// }
	// print("\n");

	buf.close();

	// Build a temporary manifest for cartridge to load:
	manifest = "";
	manifest.append("game\n");
	manifest.append("  sha256: ", Hash::SHA256(dspram).digest(), "\n");
	manifest.append("  label:  ", song_name, "\n");
	manifest.append("  name:   ", filename, "\n");

	// manifest.append("  board:  SPC\n");
	// manifest.append("    mirror mode=", "horizontal", "\n");

	// manifest.append("    memory\n");
	// manifest.append("      type: ", "ROM", "\n");
	// manifest.append("      size: 0x", hex(0), "\n");
	// manifest.append("      content: ", "Program", "\n");

	// print(manifest, "\n");

	// print("Song:      ", song_name, "\n");
	// print("Artist:    ", artist_name, "\n");
	// print("Copyright: ", copyright_name, "\n");
	// print("song count: {0}, start: {1}\n", string_format{song_count, start_song});

	Emulator::audio.setFrequency(48000);
	Emulator::audio.setVolume(1.0);
	Emulator::audio.setBalance(0.0);

	// Grab a hold of the instantied SNES components:
	cpu = &SuperFamicom::cpu;
	ppu = &SuperFamicom::ppu;
	smp = &SuperFamicom::smp;
	dsp = &SuperFamicom::dsp;
	system = &SuperFamicom::system;
	scheduler = &SuperFamicom::scheduler;

	// We don't actually need the CPU to do anything for SPC playing:
	cpu->disabled = true;
	ppu->disabled = true;

	// Load and power up the system:
	snes = new SuperFamicom::Interface;
	// print("snes->load()\n");
	if (!snes->load()) {
		print("SNES failed load()\n");
		return;
	}
	// print("snes->power()\n");
	snes->power();

	// Load SPC regs:
	smp->r.pc.byte.l = spcregs[0];
	smp->r.pc.byte.h = spcregs[1];
	smp->r.ya.byte.l = spcregs[2];
	smp->r.x = spcregs[3];
	smp->r.ya.byte.h = spcregs[4];
	smp->r.p = spcregs[5];
	smp->r.s = spcregs[6];

	// Load DSP RAM and regs:
	for (auto n : range(dspram.size())) {
		const uint16 address = n;
		const uint8 data = dspram[n];

  		dsp->apuram[address] = data;
  		// if ((address & 0xfff0) == 0x00f0) smp->writeIO(address, data);
	}

	for (auto n : range(dspregs.size())) {
		dsp->write(n, dspregs[n]);
	}

	for (auto n : range(64)) {
		smp->iplrom[n] = iplrom[n];
	}

	print("SPC state loaded\n");

	const int header_size = 0x2C;

	wave = file::open("out.wav", file::mode::write);
	wave.truncate(header_size);
	wave.seek(header_size);
	samples = 0;

	// const long play_seconds = 3 * 60 + 15;
	const long play_seconds = 1;

	const long totalCycles = (long)system->apuFrequency() / 12;
	// const long totalCycles = 1000 * 1024;
	print("apu: {0}\n", string_format{totalCycles});

	int plays = 0;
	do
	{
		// smp->step(system->apuFrequency());
		for (long cycles = 0; cycles < totalCycles; cycles++) {
			#if 0
			print("pc={0} x={1} y={2} a={3} s={4}\n", string_format{
				hex(smp->r.pc.w,4),
				hex(smp->r.x,2),
				hex(smp->r.ya.byte.h,2),
				hex(smp->r.ya.byte.l,2),
				hex(smp->r.s,2)
			});
			#endif
			scheduler->enter(Emulator::Scheduler::Mode::SynchronizeMaster);
		}

		plays++;
	} while (plays <= play_seconds);

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