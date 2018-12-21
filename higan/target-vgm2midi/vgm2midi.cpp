#include "vgm2midi.hpp"

#include "midi.cpp"
#include "wave.cpp"

#include "nsfplayer.cpp"
#include "spcplayer.cpp"

// Main:
#include <nall/main.hpp>
auto nall::main(Arguments arguments) -> void {
	// NSF filename:
	auto filename = arguments.take();
	if (!filename) {
		print("Missing filename\n");
		return;
	}

	auto df = filename.downcase();

	if (df.endsWith(".nsf")) {
		auto nsfplayer = new NSFPlayer;
		platform = nsfplayer;
		nsfplayer->run(filename, arguments);
	} else if (df.endsWith(".spc")) {
		auto spcplayer = new SPCPlayer;
		platform = spcplayer;
		spcplayer->run(filename, arguments);
	} else {
		print("Unrecognized file extension\n");
		return;
	}
}
