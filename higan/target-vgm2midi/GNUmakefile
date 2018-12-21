name := vgm2midi

# flags += -DDEBUG_NSF

objects += ui-vgm2midi ui-resource
objects := $(objects:%=obj/%.o)

obj/ui-vgm2midi.o: $(ui)/vgm2midi.cpp $(ui)/midi.cpp $(ui)/wave.cpp $(ui)/resource/resource.cpp
# obj/ui-program.o: $(ui)/program/program.cpp
# obj/ui-input.o: $(ui)/input/input.cpp
# obj/ui-settings.o: $(ui)/settings/settings.cpp
# obj/ui-tools.o: $(ui)/tools/tools.cpp
# obj/ui-presentation.o: $(ui)/presentation/presentation.cpp
obj/ui-resource.o: $(ui)/resource/resource.cpp

all: $(objects)
	$(info Linking out/$(name) ...)
	+@$(compiler) -o out/$(name) $(objects) $(options)

verbose: nall.verbose all;

install:
ifeq ($(shell id -un),root)
	$(error "make install should not be run as root")
else ifeq ($(platform),windows)
else ifeq ($(platform),macos)
	mkdir -p ~/Library/Application\ Support/$(name)/
	mkdir -p ~/Library/Application\ Support/$(name)/systems/
	mkdir -p ~/Emulation/
	cp -R out/$(name).app /Applications/$(name).app
	cp -R systems/* ~/Library/Application\ Support/$(name)/systems/
else ifneq ($(filter $(platform),linux bsd),)
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/$(name)/
	mkdir -p $(prefix)/share/$(name)/systems/
	cp out/$(name) $(prefix)/bin/$(name)
	cp -R systems/* $(prefix)/share/$(name)/systems/
	cp $(ui)/resource/$(name).desktop $(prefix)/share/applications/$(name).desktop
	cp $(ui)/resource/$(name).png $(prefix)/share/icons/$(name).png
endif

uninstall:
ifeq ($(shell id -un),root)
	$(error "make uninstall should not be run as root")
else ifeq ($(platform),windows)
else ifeq ($(platform),macos)
	rm -rf /Applications/$(name).app
else ifneq ($(filter $(platform),linux bsd),)
	rm -f $(prefix)/bin/$(name)
	rm -f $(prefix)/share/applications/$(name).desktop
	rm -f $(prefix)/share/icons/$(name).png
endif
