struct WaveFile {
	WaveFile(file_buffer& file_, long rate_, long chanCount_, long bitsPerSample_ = 16)
	  : file(file_), chanCount(chanCount_), rate(rate_), bitsPerSample(bitsPerSample_)
	{
		samples = 0;
	}

	file_buffer& file;
	const long chanCount;
	const long rate;
	const long bitsPerSample;
	long samples;

	auto writeHeader() -> void;
	auto updateHeader() -> void;

	auto writeSamples(const double *s) -> void;
};
