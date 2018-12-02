#include "vgm2midi.hpp"
#include <fc/fc.hpp>

#include "nsfplayer.cpp"

// Main:
#include <nall/main.hpp>
auto nall::main(Arguments arguments) -> void {
	// NSF filename:
	auto filename = arguments.take();
	if (!filename) {
		print("Missing filename\n");
		return;
	}

	if (filename.downcase().endsWith(".nsf")) {
		auto nsfplayer = new NSFPlayer;
		platform = nsfplayer;
		nsfplayer->run(filename, arguments);
	} else {
		print("Unrecognized file extension\n");
		return;
	}
}
