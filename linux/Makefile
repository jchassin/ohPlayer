# ============================================================================
# Minimalist Makefile for ohPlayer
# ============================================================================

OSPLATFORM = raspbian
TARGET     = $(OSPLATFORM)/openhome-player
CXX        = clang++

# Architecture Target Definitions
TARG_ARCH  = aarch64-scarthgap-linux
BUILD_TYPE = Release
OBJ_DIR    = $(OSPLATFORM)/objs

# Core Compiler Optimization and Assembly Flags
CFLAGS = -c -Wall -std=c++17 -O0 -g -DTARG_ARCH=$(TARG_ARCH) \
         -fstack-protector -fstack-check -DDEFINE_LITTLE_ENDIAN -fpermissive \
         $(shell pkg-config --cflags glib-2.0)

# Native Installation Path Configurations
PREFIX      = /usr
BINDIR      = $(PREFIX)/bin
RESDIR      = $(PREFIX)/share/openhome-player
DOCDIR      = $(PREFIX)/share/doc/openhome-player
RESOURCEDIR = ../dependencies/$(TARG_ARCH)/ohMediaPlayer/res

LIBS = $(shell pkg-config --libs glib-2.0) \
       -lConfigUiTestUtils -lConfigUi -lSourcePlaylist -lSourceSongcast \
       -lSourceUpnpAv -lPodcast -lSourceRadio -lSourceScd -lohMediaPlayer \
       -lWebAppFramework $(DEPS_DIR)/ohNet/lib/libohNetCore.a \
       -lCodecAifc \
       -lCodecAlacApple -lCodecAlacAppleBase -lCodecPcm -lCodecAiff \
       -lCodecAiffBase -lCodecVorbis -llibOgg -lCodecFlac -lCodecWav \
       -lCodecDsdDsf -lCodecDsdDff -lCodecDsdRaw -lSourceRaop -lohPipeline \
       -lCodecAacFdkAdts -lCodecAacFdkBase -lCodecMp3 -lCodecAacFdkMp4 -lCodecAacFdk \
       -lpthread -lasound -lsoxr -ldl -lm -lOdp -lssl -lcrypto

# Static Search Path Directories
DEPS_DIR = ../dependencies/$(TARG_ARCH)
INCLUDES = -I$(DEPS_DIR)/ohMediaPlayer/include \
           -I$(DEPS_DIR)/ohNetmon/include \
           -I$(DEPS_DIR)/ohNetGenerated-$(TARG_ARCH)-$(BUILD_TYPE)/include/ohnet/OpenHome/Net/Core \
           -I$(DEPS_DIR)/ohNet/include/ohnet \
           -I$(DEPS_DIR)

LDFLAGS  = -L$(DEPS_DIR)/ohMediaPlayer/lib \
           -L$(DEPS_DIR)/ohNetmon/lib \
           -L$(DEPS_DIR)/ohNet/lib \
           -L$(DEPS_DIR)/ohNetGenerated-$(TARG_ARCH)-$(BUILD_TYPE)/lib

# Workspace Source Track Aggregations
OBJECTS  = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(wildcard *.cpp))
HEADERS  = $(wildcard *.h)

# ============================================================================
# Compilation Build Automation Targets
# ============================================================================

.PHONY: default all clean build install uninstall

default: build $(TARGET)
all: default

$(OBJ_DIR)/%.o: %.cpp $(HEADERS)
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CXX) -Wall $(LDFLAGS) -Wl,--start-group $(OBJECTS) $(LIBS) -Wl,--end-group -o $@

build:
	@mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OSPLATFORM)/objs
	rm -f $(TARGET)

install:
	install -m 755 -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(DOCDIR) $(DESTDIR)$(RESDIR) $(DESTDIR)$(RESDIR)/res
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)
	install -m 644 OpenHome-48x48.png $(DESTDIR)$(RESDIR)
	install -m 644 copyright $(DESTDIR)$(DOCDIR)
	cp -R $(RESOURCEDIR) $(DESTDIR)$(RESDIR)
	cp $(RESOURCEDIR)/lang/en-gb/ConfigOptions.txt $(DESTDIR)$(RESDIR)/res/lang

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/openhome-player
	rm -rf $(DESTDIR)$(DOCDIR)
	rm -rf $(DESTDIR)$(RESDIR)
