
auto WaveFile::writeHeader() -> void {
	// Write WAVE headers:
	const int headerSize = 0x2C;

	long ds = samples * sizeof (float);
	long rs = headerSize - 8 + ds;
	int frameSize = chanCount * sizeof (float);
	long bps = rate * frameSize;

	unsigned char header[headerSize] =
	{
		'R','I','F','F',
		rs,rs>>8,              // length of rest of file
		rs>>16,rs>>24,

		'W','A','V','E',
		'f','m','t',' ',
		0x10,0,0,0,            // size of fmt chunk

		// 1,0,                // PCM format
		3,0,				   // float format

		chanCount,0,           // channel count
		rate,rate >> 8,        // sample rate
		rate>>16,rate>>24,
		bps,bps>>8,            // bytes per second
		bps>>16,bps>>24,
		frameSize,0,           // bytes per sample frame
		32,0,                  // bits per sample

		'd','a','t','a',
		ds,ds>>8,ds>>16,ds>>24 // size of sample data
		// ...                 // sample data
	};

	file.write({header, headerSize});
}

auto WaveFile::updateHeader() -> void {
	auto offs = file.offset();

	file.seek(0);
	writeHeader();

	file.seek(offs);
}

auto WaveFile::writeSamples(const double *s) -> void {
	for (auto chan: range(chanCount)) {
		// Write 32-bit sample:
		auto x = (float)s[chan];
		file.write({&x, 4});
	}
	samples++;
}
