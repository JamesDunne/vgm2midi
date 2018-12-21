struct WaveFile {
	WaveFile(file_buffer& file_, long chanCount_, long rate_)
	  : file(file_), chanCount(chanCount_), rate(rate_)
	{
		samples = 0;
	}

	file_buffer& file;
	const long chanCount;
	const long rate;
	long samples;

	auto writeHeader() -> void;
	auto updateHeader() -> void;

	auto writeSamples(const double *s) -> void;
};
