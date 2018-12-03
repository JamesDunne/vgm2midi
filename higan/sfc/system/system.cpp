#include <sfc/sfc.hpp>

namespace SuperFamicom {

System system;
Scheduler scheduler;
Random random;
Cheat cheat;
#include "serialization.cpp"

auto System::run() -> void {
  if(scheduler.enter() == Scheduler::Event::Frame) if (!ppu.disabled) ppu.refresh();
}

auto System::runToSave() -> void {
  if (!cpu.disabled) scheduler.synchronize(cpu);
  scheduler.synchronize(smp);
  if (!ppu.disabled) scheduler.synchronize(ppu);
  scheduler.synchronize(dsp);
  if (!cpu.disabled) {
    for(auto coprocessor : cpu.coprocessors) scheduler.synchronize(*coprocessor);
    for(auto peripheral : cpu.peripherals) scheduler.synchronize(*peripheral);
  }
}

auto System::load(Emulator::Interface* interface) -> bool {
  information = {};
  hacks.fastPPU = configuration.hacks.ppuFast.enable;
  hacks.fastDSP = configuration.hacks.dspFast.enable;

  bus.reset();
  if (!cpu.disabled) if(!cpu.load()) return false;
  if(!smp.load()) return false;
  if (!ppu.disabled) if(!ppu.load()) return false;
  if(!dsp.load()) return false;
  if(!cartridge.load()) return false;

  if(cartridge.region() == "NTSC") {
    information.region = Region::NTSC;
    information.cpuFrequency = Emulator::Constants::Colorburst::NTSC * 6.0;
  }
  if(cartridge.region() == "PAL") {
    information.region = Region::PAL;
    information.cpuFrequency = Emulator::Constants::Colorburst::PAL * 4.8;
  }

  if(cartridge.has.ICD) icd.load();
  if(cartridge.has.BSMemorySlot) bsmemory.load();

  serializeInit();
  this->interface = interface;
  return information.loaded = true;
}

auto System::save() -> void {
  if(!loaded()) return;

  cartridge.save();
}

auto System::unload() -> void {
  if(!loaded()) return;

  if (!cpu.disabled) cpu.peripherals.reset();
  controllerPort1.unload();
  controllerPort2.unload();
  expansionPort.unload();

  if(cartridge.has.ICD) icd.unload();
  if(cartridge.has.MCC) mcc.unload();
  if(cartridge.has.Event) event.unload();
  if(cartridge.has.SA1) sa1.unload();
  if(cartridge.has.SuperFX) superfx.unload();
  if(cartridge.has.HitachiDSP) hitachidsp.unload();
  if(cartridge.has.SPC7110) spc7110.unload();
  if(cartridge.has.SDD1) sdd1.unload();
  if(cartridge.has.OBC1) obc1.unload();
  if(cartridge.has.MSU1) msu1.unload();
  if(cartridge.has.BSMemorySlot) bsmemory.unload();
  if(cartridge.has.SufamiTurboSlotA) sufamiturboA.unload();
  if(cartridge.has.SufamiTurboSlotB) sufamiturboB.unload();

  cartridge.unload();
  information.loaded = false;
}

auto System::power(bool reset) -> void {
  Emulator::video.reset(interface);
  Emulator::video.setPalette();

  Emulator::audio.reset(interface);

  random.entropy(Random::Entropy::Low);

  scheduler.reset();
  if (!cpu.disabled) cpu.power(reset);
  smp.power(reset);
  dsp.power(reset);
  if (!ppu.disabled) ppu.power(reset);

  if(cartridge.has.ICD) icd.power();
  if(cartridge.has.MCC) mcc.power();
  if(cartridge.has.DIP) dip.power();
  if(cartridge.has.Event) event.power();
  if(cartridge.has.SA1) sa1.power();
  if(cartridge.has.SuperFX) superfx.power();
  if(cartridge.has.ARMDSP) armdsp.power();
  if(cartridge.has.HitachiDSP) hitachidsp.power();
  if(cartridge.has.NECDSP) necdsp.power();
  if(cartridge.has.EpsonRTC) epsonrtc.power();
  if(cartridge.has.SharpRTC) sharprtc.power();
  if(cartridge.has.SPC7110) spc7110.power();
  if(cartridge.has.SDD1) sdd1.power();
  if(cartridge.has.OBC1) obc1.power();
  if(cartridge.has.MSU1) msu1.power();
  if(cartridge.has.BSMemorySlot) bsmemory.power();
  if(cartridge.has.SufamiTurboSlotA) sufamiturboA.power();
  if(cartridge.has.SufamiTurboSlotB) sufamiturboB.power();

  if (!cpu.disabled) {
    if(cartridge.has.ICD) cpu.coprocessors.append(&icd);
    if(cartridge.has.Event) cpu.coprocessors.append(&event);
    if(cartridge.has.SA1) cpu.coprocessors.append(&sa1);
    if(cartridge.has.SuperFX) cpu.coprocessors.append(&superfx);
    if(cartridge.has.ARMDSP) cpu.coprocessors.append(&armdsp);
    if(cartridge.has.HitachiDSP) cpu.coprocessors.append(&hitachidsp);
    if(cartridge.has.NECDSP) cpu.coprocessors.append(&necdsp);
    if(cartridge.has.EpsonRTC) cpu.coprocessors.append(&epsonrtc);
    if(cartridge.has.SharpRTC) cpu.coprocessors.append(&sharprtc);
    if(cartridge.has.SPC7110) cpu.coprocessors.append(&spc7110);
    if(cartridge.has.MSU1) cpu.coprocessors.append(&msu1);
    if(cartridge.has.BSMemorySlot) cpu.coprocessors.append(&bsmemory);

    scheduler.primary(cpu);
  } else {
    scheduler.primary(smp);
  }

  controllerPort1.power(ID::Port::Controller1);
  controllerPort2.power(ID::Port::Controller2);
  expansionPort.power();

  controllerPort1.connect(settings.controllerPort1);
  controllerPort2.connect(settings.controllerPort2);
  expansionPort.connect(settings.expansionPort);
}

}
