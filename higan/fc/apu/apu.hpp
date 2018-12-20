struct APU : Thread {
  shared_pointer<Emulator::Stream> stream;

  inline auto rate() const -> uint { return Region::PAL() ? 16 : 12; }

  //apu.cpp
  APU();

  static auto Enter() -> void;
  auto main() -> void;
  auto tick() -> void;
  auto setIRQ() -> void;
  auto setSample(int16 sample) -> void;

  auto power(bool reset) -> void;

  auto readIO(uint16 addr) -> uint8;
  auto writeIO(uint16 addr, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Envelope {
    auto volume() const -> uint;
    auto clock() -> void;

    auto power() -> void;

    auto serialize(serializer&) -> void;

    uint4 speed;
    bool useSpeedAsVolume;
    bool loopMode;

    bool reloadDecay;
    uint8 decayCounter;
    uint4 decayVolume;

    bool midiTrigger;
  };

  struct Sweep {
    auto checkPeriod() -> bool;
    auto clock(uint channel) -> void;

    auto power() -> void;

    auto serialize(serializer&) -> void;

    uint8 shift;
    bool decrement;
    uint3 period;
    uint8 counter;
    bool enable;
    bool reload;
    uint11 pulsePeriod;
  };

  struct Pulse : MIDIMelodic {
    auto clockLength() -> void;
    auto checkPeriod() -> bool;
    auto clock() -> uint8;

    auto power() -> void;
 
    auto serialize(serializer&) -> void;

    uint lengthCounter;

    Envelope envelope;
    Sweep sweep;

    uint2 duty;
    uint3 dutyCounter;

    uint11 period;
    uint periodCounter;

    uint  n; // which pulse (0, 1) is this
    uint  lastDuty;

    bool  midiTrigger;
    bool  midiTriggerMaybe;
    int   written;

    virtual auto midiProgram() -> uint7 override;
    virtual auto midiChannel() -> uint4 override;
    virtual auto midiChannelVolume() -> uint7 override;
    virtual auto midiNote() -> double override;
    virtual auto midiNoteVelocity() -> uint7 override;
  } pulse[2];

  struct Triangle : MIDIMelodic {
    auto clockLength() -> void;
    auto clockLinearLength() -> void;
    auto clock() -> uint8;

    auto power() -> void;

    auto serialize(serializer&) -> void;

    uint lengthCounter;

    uint8 linearLength;
    bool haltLengthCounter;

    uint11 period;
    uint periodCounter;

    uint5 stepCounter;
    uint8 linearLengthCounter;
    bool reloadLinear;

    bool midiTrigger;
    int written;

    virtual auto midiProgram() -> uint7 override;
    virtual auto midiChannel() -> uint4 override;
    virtual auto midiChannelVolume() -> uint7 override;
    virtual auto midiNote() -> double override;
    virtual auto midiNoteVelocity() -> uint7 override;
  } triangle;

  struct Noise : MIDIRhythmic {
    auto clockLength() -> void;
    auto clock() -> uint8;

    auto power() -> void;

    auto serialize(serializer&) -> void;

    uint lengthCounter;

    Envelope envelope;

    uint4 period;
    uint periodCounter;

    bool shortMode;
    uint15 lfsr;

    virtual auto midiNote() -> double override;
    virtual auto midiNoteVelocity() -> uint7 override;
    // virtual auto midiNoteOn() -> void override;

    map<uint, int> periodMidiNote;

    int written;
    int lastEnvelopeDirection;
    int lastEnvelopeVolume;
  } noise;

  struct DMC : MIDIRhythmic {
    auto start() -> void;
    auto stop() -> void;
    auto clock() -> uint8;

    auto power() -> void;

    auto serialize(serializer&) -> void;

    uint lengthCounter;
    bool irqPending;

    uint4 period;
    uint periodCounter;

    bool irqEnable;
    bool loopMode;

    uint8 dacLatch;
    uint8 addrLatch;
    uint8 lengthLatch;

    uint15 readAddr;
    uint dmaDelayCounter;

    uint3 bitCounter;
    bool dmaBufferValid;
    uint8 dmaBuffer;

    bool sampleValid;
    uint8 sample;

    virtual auto midiProgram() -> uint7 override;
    virtual auto midiChannel() -> uint4 override;
    virtual auto midiNote() -> double override;
    virtual auto midiNoteVelocity() -> uint7 override;

    struct targetMidi {
      uint4 midiChannel;
      uint7 midiProgram;
      int midiNote;

      // period to midiNote on midiChannel (else use midiNote):
      map<uint4, int> periodMidiNote;
    };

    map<uint8, targetMidi> sampleMidiMap;
  } dmc;

  struct FrameCounter {
    auto serialize(serializer&) -> void;

    enum : uint { NtscPeriod = 14915 };  //~(21.477MHz / 6 / 240hz)

    bool irqPending;

    uint2 mode;
    uint2 counter;
    int divider;
  };

  auto clockFrameCounter() -> void;
  auto clockFrameCounterDivider() -> void;

  FrameCounter frame;

  uint8 enabledChannels;
  double cartridgeSample;

  double pulseDAC[32];
  double dmcTriangleNoiseDAC[128][16][16];

  uint cyclesPerMidiTick;
  uint midiTickCycle;

  double sampleRate;

  double periodMidi[0x800];

  const double midiTempo = 120.0;
  const uint   midiTicksPerBeat = 480;

  auto loadMidiSupport(Markup::Node document) -> void;

  static const uint8 lengthCounterTable[32];
  static const uint16 dmcPeriodTableNTSC[16];
  static const uint16 dmcPeriodTablePAL[16];
  static const uint16 noisePeriodTableNTSC[16];
  static const uint16 noisePeriodTablePAL[16];
};

extern APU apu;
