#include <fc/fc.hpp>

namespace Famicom {

#include "midi_instrument.cpp"
#include "envelope.cpp"
#include "sweep.cpp"
#include "pulse.cpp"
#include "triangle.cpp"
#include "noise.cpp"
#include "dmc.cpp"
#include "serialization.cpp"
APU apu;

APU::APU() {
  for(uint amp : range(32)) {
    if(amp == 0) {
      pulseDAC[amp] = 0.0;
    } else {
      pulseDAC[amp] = 95.88 / ((8128.0 / amp) + 100.0);
    }

    pulseMIDI[amp] = (uint7)(127 * log2(1 + pow(4.2 * pulseDAC[amp], 0.75)));
  }

  for(uint dmc_amp : range(128)) {
    for(uint triangle_amp : range(16)) {
      for(uint noise_amp : range(16)) {
        if(dmc_amp == 0 && triangle_amp == 0 && noise_amp == 0) {
          dmcTriangleNoiseDAC[dmc_amp][triangle_amp][noise_amp] = 0;
        } else {
          dmcTriangleNoiseDAC[dmc_amp][triangle_amp][noise_amp]
          = 159.79 / (100.0 + 1.0 / (triangle_amp / 8227.0 + noise_amp / 12241.0 + dmc_amp / 22638.0));
        }
      }
    }
  }

  for (uint noise_amp : range(16)) {
    noiseMIDI[noise_amp] = (uint7)(127 * log2(1 + pow(4.0 * dmcTriangleNoiseDAC[0][0][noise_amp], 0.75)));
  }

  triangleMIDI = (uint7)(127 * log2(1 + pow(4.0 * dmcTriangleNoiseDAC[0][3][0], 0.75)));
  dmcMIDI = (uint7)(127 * log2(1 + pow(4.0 * dmcTriangleNoiseDAC[12][0][0], 0.75)));

  emitEvents.pulse[0] = false;
  emitEvents.pulse[1] = false;
  emitEvents.triangle = false;
  emitEvents.noise = false;
  emitEvents.dmc = false;
  emitEvents.control = false;

  pulse[0].n = 0;
  pulse[1].n = 1;
}

auto APU::Enter() -> void {
  while(true) scheduler.synchronize(), apu.main();
}

auto APU::main() -> void {
  uint pulse_output, triangle_output, noise_output, dmc_output;

  pulse_output  = pulse[0].clock();
  pulse_output += pulse[1].clock();
  triangle_output = triangle.clock();
  noise_output = noise.clock();
  dmc_output = dmc.clock();

  clockFrameCounterDivider();

  double output = 0.0;
  output += pulseDAC[pulse_output];
  output += dmcTriangleNoiseDAC[dmc_output][triangle_output][noise_output];
  output += cartridgeSample;
  stream->sample(output);

  // Advance MIDI tick when enough APU cycles have elapsed:
  if (++midiTickCycle >= cyclesPerMidiTick) {
    midiTickCycle = 0;
    platform->advanceMIDITicks(1);
  }

  tick();
}

auto APU::tick() -> void {
  Thread::step(rate());
  synchronize(cpu);
}

auto APU::setIRQ() -> void {
  cpu.apuLine(frame.irqPending || dmc.irqPending);
}

auto APU::setSample(int16 sample) -> void {
  cartridgeSample = sample / 32768.0;
}

auto APU::power(bool reset) -> void {
  create(APU::Enter, system.frequency());
  stream = Emulator::audio.createStream(1, frequency() / rate());
  stream->addFilter(Emulator::Filter::Order::First, Emulator::Filter::Type::HighPass, 90.0);
  stream->addFilter(Emulator::Filter::Order::First, Emulator::Filter::Type::HighPass, 440.0);
  stream->addFilter(Emulator::Filter::Order::First, Emulator::Filter::Type::LowPass, 14000.0);
  stream->enableFilterDCOffset();

  sampleRate = (frequency() / rate());

  pulse[0].power();
  pulse[1].power();
  triangle.power();
  noise.power();
  dmc.power();

  // cycles |  60 seconds |   1 minutes |   1 beats
  // -------+-------------+-------------+----------
  // second |   1 minutes | 120 beats   | 480 ticks  
  cyclesPerMidiTick = sampleRate * 60.0 / (midiTempo * midiTicksPerBeat);
  // print(cyclesPerMidiTick, "\n");
  // cyclesPerMidiTick = 1864 (NTSC)

  midiTickCycle = 0;

  for (auto p : range(0x800)) {
    double f = sampleRate / (16 * (p + 1));
    double n = (log(f / 54.99090178) / log(2)) * 12;

    periodMidi[p] = n;
  }

  frame.irqPending = 0;

  frame.mode = 0;
  frame.counter = 0;
  frame.divider = 1;

  enabledChannels = 0;
  cartridgeSample = 0;

  setIRQ();
}

auto APU::readIO(uint16 addr) -> uint8 {
  switch(addr) {

  case 0x4015: {
    uint8 result = 0x00;
    result |= pulse[0].lengthCounter ? 0x01 : 0;
    result |= pulse[1].lengthCounter ? 0x02 : 0;
    result |= triangle.lengthCounter ? 0x04 : 0;
    result |=    noise.lengthCounter ? 0x08 : 0;
    result |=      dmc.lengthCounter ? 0x10 : 0;
    result |=       frame.irqPending ? 0x40 : 0;
    result |=         dmc.irqPending ? 0x80 : 0;

    frame.irqPending = false;
    setIRQ();

    return result;
  }

  }

  return cpu.mdr();
}

auto APU::writeIO(uint16 addr, uint8 data) -> void {
  const auto metaEventKind = 0x04;

  const uint n = (addr >> 2) & 1;  //pulse#

  switch(addr) {

  case 0x4000: case 0x4004: {
    pulse[n].duty = data >> 6;
    pulse[n].envelope.loopMode = data & 0x20;
    pulse[n].envelope.useSpeedAsVolume = data & 0x10;
    pulse[n].envelope.speed = data & 0x0f;
    pulse[n].written = cyclesPerMidiTick;
    if (emitEvents.pulse[n]) {
      pulse[n].midi->channelPrefixMeta(
        pulse[n].midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x4001: case 0x4005: {
    pulse[n].sweep.enable = data & 0x80;
    pulse[n].sweep.period = (data & 0x70) >> 4;
    pulse[n].sweep.decrement = data & 0x08;
    pulse[n].sweep.shift = data & 0x07;
    pulse[n].sweep.reload = true;
    pulse[n].written = cyclesPerMidiTick;
    if (emitEvents.pulse[n]) {
      pulse[n].midi->channelPrefixMeta(
        pulse[n].midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x4002: case 0x4006: {
    pulse[n].period = (pulse[n].period & 0x0700) | (data << 0);
    pulse[n].sweep.pulsePeriod = (pulse[n].sweep.pulsePeriod & 0x0700) | (data << 0);
    pulse[n].midiTriggerMaybe = true;
    pulse[n].written = cyclesPerMidiTick;
    if (emitEvents.pulse[n]) {
      pulse[n].midi->channelPrefixMeta(
        pulse[n].midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x4003: case 0x4007: {
    pulse[n].period = (pulse[n].period & 0x00ff) | (data << 8);
    pulse[n].sweep.pulsePeriod = (pulse[n].sweep.pulsePeriod & 0x00ff) | (data << 8);

    pulse[n].dutyCounter = 0;
    // pulse[n].midiTrigger = true;
    pulse[n].envelope.reloadDecay = true;
    pulse[n].written = cyclesPerMidiTick;

    if(enabledChannels & (1 << n)) {
      pulse[n].lengthCounter = lengthCounterTable[(data >> 3) & 0x1f];
    }
    if (emitEvents.pulse[n]) {
      pulse[n].midi->channelPrefixMeta(
        pulse[n].midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x4008: {
    triangle.haltLengthCounter = data & 0x80;
    triangle.linearLength = data & 0x7f;
    triangle.written = cyclesPerMidiTick;
    if (emitEvents.triangle) {
      triangle.midi->channelPrefixMeta(
        triangle.midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x400a: {
    triangle.period = (triangle.period & 0x0700) | (data << 0);
    triangle.written = cyclesPerMidiTick;
    if (emitEvents.triangle) {
      triangle.midi->channelPrefixMeta(
        triangle.midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x400b: {
    triangle.period = (triangle.period & 0x00ff) | (data << 8);

    triangle.reloadLinear = true;
    triangle.written = cyclesPerMidiTick;

    if(enabledChannels & (1 << 2)) {
      triangle.lengthCounter = lengthCounterTable[(data >> 3) & 0x1f];
      triangle.midiTrigger = true;
    }

    if (emitEvents.triangle) {
      triangle.midi->channelPrefixMeta(
        triangle.midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x400c: {
    noise.envelope.loopMode = data & 0x20;
    noise.envelope.useSpeedAsVolume = data & 0x10;
    noise.envelope.speed = data & 0x0f;
    noise.written = cyclesPerMidiTick;
    if (emitEvents.noise) {
      noise.midi->channelPrefixMeta(
        noise.midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x400e: {
    noise.shortMode = data & 0x80;
    noise.period = data & 0x0f;
    noise.written = cyclesPerMidiTick;
    if (emitEvents.noise) {
      noise.midi->channelPrefixMeta(
        noise.midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x400f: {
    noise.envelope.reloadDecay = true;
    noise.written = cyclesPerMidiTick;

    if(enabledChannels & (1 << 3)) {
      noise.lengthCounter = lengthCounterTable[(data >> 3) & 0x1f];
    }

    if (emitEvents.noise) {
      noise.midi->channelPrefixMeta(
        noise.midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x4010: {
    dmc.irqEnable = data & 0x80;
    dmc.loopMode = data & 0x40;
    dmc.period = data & 0x0f;

    dmc.irqPending = dmc.irqPending && dmc.irqEnable && !dmc.loopMode;

    if (emitEvents.dmc) {
      dmc.midi->channelPrefixMeta(
        dmc.midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }

    setIRQ();
    return;
  }

  case 0x4011: {
    dmc.dacLatch = data & 0x7f;
    if (emitEvents.dmc) {
      dmc.midi->channelPrefixMeta(
        dmc.midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x4012: {
    dmc.addrLatch = data;
    if (emitEvents.dmc) {
      dmc.midi->channelPrefixMeta(
        dmc.midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x4013: {
    dmc.lengthLatch = data;
    if (emitEvents.dmc) {
      dmc.midi->channelPrefixMeta(
        dmc.midiChannel(),
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x4015: {
    if((data & 0x01) == 0) { pulse[0].lengthCounter = 0; pulse[0].written = cyclesPerMidiTick; }
    if((data & 0x02) == 0) { pulse[1].lengthCounter = 0; pulse[1].written = cyclesPerMidiTick; }
    if((data & 0x04) == 0) { triangle.lengthCounter = 0; triangle.written = cyclesPerMidiTick; triangle.midiNoteOff(); }
    if((data & 0x08) == 0) {    noise.lengthCounter = 0; noise.written = cyclesPerMidiTick; }

    (data & 0x10) ? dmc.start() : dmc.stop();
    dmc.irqPending = false;

    setIRQ();
    enabledChannels = data & 0x1f;

    if (emitEvents.control) {
      dmc.midi->meta(
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  case 0x4017: {
    frame.mode = data >> 6;

    frame.counter = 0;
    if(frame.mode & 2) clockFrameCounter();
    if(frame.mode & 1) {
      frame.irqPending = false;
      setIRQ();
    }
    frame.divider = FrameCounter::NtscPeriod;

    if (emitEvents.control) {
      dmc.midi->meta(
        metaEventKind,
        string("[{0}]={1}").format(string_format{hex(addr&0x1f,2),hex(data,2)})
      );
    }
    return;
  }

  }
}

auto APU::clockFrameCounter() -> void {
  frame.counter++;

  if(frame.counter & 1) {
    pulse[0].clockLength();
    pulse[0].sweep.clock(0);
    pulse[1].clockLength();
    pulse[1].sweep.clock(1);
    triangle.clockLength();
    noise.clockLength();
  }

  pulse[0].envelope.clock();
  pulse[1].envelope.clock();
  triangle.clockLinearLength();
  noise.envelope.clock();

  if(frame.counter == 0) {
    if(frame.mode & 2) frame.divider += FrameCounter::NtscPeriod;
    if(frame.mode == 0) {
      frame.irqPending = true;
      setIRQ();
    }
  }
}

auto APU::clockFrameCounterDivider() -> void {
  frame.divider -= 2;
  if(frame.divider <= 0) {
    clockFrameCounter();
    frame.divider += FrameCounter::NtscPeriod;
  }
}

const uint8 APU::lengthCounterTable[32] = {
  0x0a, 0xfe, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xa0, 0x08, 0x3c, 0x0a, 0x0e, 0x0c, 0x1a, 0x0e,
  0x0c, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16, 0xc0, 0x18, 0x48, 0x1a, 0x10, 0x1c, 0x20, 0x1e,
};

const uint16 APU::noisePeriodTableNTSC[16] = {
  4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
};

const uint16 APU::noisePeriodTablePAL[16] = {
  4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778,
};

const uint16 APU::dmcPeriodTableNTSC[16] = {
  428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54,
};

const uint16 APU::dmcPeriodTablePAL[16] = {
  398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118,  98, 78, 66, 50,
};

auto APU::loadMidiSupport(Markup::Node document) -> void {
  for (auto nr : document["noise"]) {
    auto period = nr["period"].integer();
    auto midiNote = nr["midiNote"].integer();
    auto disable = nr["disable"];
    if (disable) {
      midiNote = -1;
    }

    // print("noise map period={0} midiNote={1}\n", string_format{period, midiNote});
    noise.periodMidiNote.insert(
      period,
      midiNote
    );
  }

  for (auto nr : document["dmc"]) {
    auto sample = nr["sample"].natural();
    auto midiChannel = nr["midiChannel"].natural() - 1;
    auto midiProgram = nr["midiProgram"].natural();
    auto midiNote = nr["midiNote"].integer();
    auto disable = nr["disable"];
    if (disable) {
      midiNote = -1;
    }

    // print("dmc map sample={0} midiChannel={1}\n", string_format{sample, midiChannel});

    auto tm = dmc.sampleMidiMap.find(sample);
    if (!tm) {
      dmc.sampleMidiMap.insert(
        sample,
        {
          midiChannel: midiChannel,
          midiProgram: midiProgram,
          // TODO: initialize midiNote when period is set?
          midiNote: midiNote
        }
      );

      tm = dmc.sampleMidiMap.find(sample);
    }

    if (nr["period"]) {
      tm().periodMidiNote.insert(nr["period"].natural(), midiNote);
    }
  }

  // Let the user control the emitting of meta-events to MIDI:
  if (auto eventsNode = document["events"]) {
    emitEvents.pulse[0] = (bool)eventsNode["pulse0"];
    emitEvents.pulse[1] = (bool)eventsNode["pulse1"];
    emitEvents.triangle = (bool)eventsNode["triangle"];
    emitEvents.noise = (bool)eventsNode["noise"];
    emitEvents.dmc = (bool)eventsNode["dmc"];
    emitEvents.control = (bool)eventsNode["control"];
  }
}

}
