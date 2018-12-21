
auto WaveFile::writeHeader() -> void {
	// Write WAVE headers:
	const int headerSize = 0x2C;

	long ds = samples * (bitsPerSample / 8);
	long rs = headerSize - 8 + ds;
	int frameSize = chanCount * (bitsPerSample / 8);
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
		// 3,0,                // float format
		bitsPerSample == 32 ? 3 : 1, 0,

		chanCount,0,           // channel count
		rate,rate >> 8,        // sample rate
		rate>>16,rate>>24,
		bps,bps>>8,            // bytes per second
		bps>>16,bps>>24,
		frameSize,0,           // bytes per sample frame
		bitsPerSample,0,       // bits per sample

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
		if (bitsPerSample == 32) {
			// Write 32-bit sample:
			auto x = (float)s[chan];
			file.write({&x, 4});
		} else if (bitsPerSample == 24) {
			// Write 24-bit sample:
			auto x = (int32_t)(s[chan] * 16777215.0);
			file.write({&x, 3});
		} else if (bitsPerSample == 16) {
			// Write 16-bit sample:
			auto x = (int16_t)(s[chan] * 32767.0);
			file.write({&x, 2});
		} else if (bitsPerSample == 8) {
			// Write  8-bit sample:
			auto x = (int8_t)(s[chan] * 127.0);
			file.write({&x, 1});
		}
	}
	samples++;
}
