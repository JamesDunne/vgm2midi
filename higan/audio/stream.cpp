auto Stream::reset(uint channels_, double inputFrequency, double outputFrequency) -> void {
  channels.reset();
  channels.resize(channels_);

  for(auto& channel : channels) {
    channel.filters.reset();
    channel.filterDCOffset = {false,0,0};
  }

  setFrequency(inputFrequency, outputFrequency);
}

auto Stream::setFrequency(double inputFrequency, maybe<double> outputFrequency) -> void {
  this->inputFrequency = inputFrequency;
  if(outputFrequency) this->outputFrequency = outputFrequency();

  for(auto& channel : channels) {
    channel.nyquist.reset();
    channel.resampler.reset(this->inputFrequency, this->outputFrequency);
  }

  if(this->inputFrequency >= this->outputFrequency * 2) {
    //add a low-pass filter to prevent aliasing during resampling
    double cutoffFrequency = min(25000.0, this->outputFrequency / 2.0 - 2000.0);
    for(auto& channel : channels) {
      uint passes = 3;
      for(uint pass : range(passes)) {
        DSP::IIR::Biquad filter;
        double q = DSP::IIR::Biquad::butterworth(passes * 2, pass);
        filter.reset(DSP::IIR::Biquad::Type::LowPass, cutoffFrequency, this->inputFrequency, q);
        channel.nyquist.append(filter);
      }
    }
  }
}

auto Stream::addFilter(Filter::Order order, Filter::Type type, double cutoffFrequency, uint passes) -> void {
  for(auto& channel : channels) {
    for(uint pass : range(passes)) {
      Filter filter{order};

      if(order == Filter::Order::First) {
        DSP::IIR::OnePole::Type _type;
        if(type == Filter::Type::LowPass) _type = DSP::IIR::OnePole::Type::LowPass;
        if(type == Filter::Type::HighPass) _type = DSP::IIR::OnePole::Type::HighPass;
        filter.onePole.reset(_type, cutoffFrequency, inputFrequency);
      }

      if(order == Filter::Order::Second) {
        DSP::IIR::Biquad::Type _type;
        if(type == Filter::Type::LowPass) _type = DSP::IIR::Biquad::Type::LowPass;
        if(type == Filter::Type::HighPass) _type = DSP::IIR::Biquad::Type::HighPass;
        double q = DSP::IIR::Biquad::butterworth(passes * 2, pass);
        filter.biquad.reset(_type, cutoffFrequency, inputFrequency, q);
      }

      channel.filters.append(filter);
    }
  }
}

auto FilterDCOffset::filter(double sample) -> double {
  if (!enabled) return sample;

  // Remove DC offset:
  otm = 0.999 * otm + sample - itm;
  itm = sample;
  return otm;
}

auto Stream::enableFilterDCOffset(bool enabled) -> void {
  for(auto& channel : channels) {
    channel.filterDCOffset = {enabled,0,0};
  }
}

auto Stream::pending() const -> bool {
  return channels && channels[0].resampler.pending();
}

auto Stream::read(double samples[]) -> uint {
  for(uint c : range(channels.size())) {
    double sample = channels[c].resampler.read();
    samples[c] = channels[c].filterDCOffset.filter(sample);
  }
  return channels.size();
}

auto Stream::write(const double samples[]) -> void {
  // print("Stream::write channels={0}\n", string_format{channels.size()});
  for(auto c : range(channels.size())) {
    double sample = samples[c] + 1e-25;  //constant offset used to suppress denormals
    for(auto& filter : channels[c].filters) {
      switch(filter.order) {
      case Filter::Order::First: sample = filter.onePole.process(sample); break;
      case Filter::Order::Second: sample = filter.biquad.process(sample); break;
      }
    }
    for(auto& filter : channels[c].nyquist) {
      sample = filter.process(sample);
    }
    // print("Stream::write channels[{0}].resampler.write({1})\n", string_format{c, sample});
    channels[c].resampler.write(sample);
  }

  audio.process();
}
