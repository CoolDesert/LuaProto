# Detect operating system
UNAME_S := $(shell uname -s)

# Platform-specific settings
ifeq ($(UNAME_S),Darwin)
    # macOS settings
    BINDIR?=/opt/homebrew/bin
    LIBDIR?=/opt/homebrew/lib
    INCDIR?=/opt/homebrew/include
    CC=clang++
    # macOS-specific linker flags
    PLATFORM_LDFLAGS=-undefined dynamic_lookup
    # Lua include path for macOS
    LUA_INCDIR?=$(INCDIR)/lua
else ifeq ($(UNAME_S),Linux)
    # Linux settings
    BINDIR?=/usr/bin
    LIBDIR?=/usr/lib/x86_64-linux-gnu
    INCDIR?=/usr/include
    CC=g++
    # Linux-specific linker flags
    PLATFORM_LDFLAGS=
    # Lua include path for Linux
    LUA_INCDIR?=/usr/include/lua5.3
else
    $(error Unsupported operating system: $(UNAME_S))
endif

# Common C++20 flags with optimizations
CFLAGS=-fPIC --shared -std=c++20 -O2 -DNDEBUG -Wall -Wextra $(PLATFORM_LDFLAGS)

PROTO_SRCS=example.proto
PROTO_CC=$(PROTO_SRCS:.proto=.pb.cc)
PROTO_H=$(PROTO_SRCS:.proto=.pb.h)

all: proto.so
	@echo "Build complete!"

info:
	@echo "Detected OS: $(UNAME_S)"
	@echo "Compiler: $(CC)"
	@echo "Binary directory: $(BINDIR)"
	@echo "Library directory: $(LIBDIR)"
	@echo "Include directory: $(INCDIR)"
	@echo "Lua include directory: $(LUA_INCDIR)"

$(PROTO_CC) $(PROTO_H): $(PROTO_SRCS)
	@echo "Generating protobuf files from $<..."
	@$(BINDIR)/protoc --cpp_out=. $<
	@echo "Generated $(PROTO_CC) and $(PROTO_H)"

proto.so: LuaProto.cpp $(PROTO_CC)
	$(CC) $(CFLAGS) -o $@ $^ -I$(INCDIR) -I$(LUA_INCDIR) -L$(LIBDIR) -lprotobuf

clean:
	@echo "Cleaning build artifacts..."
	@rm -f *.o *.so *.pb.cc *.pb.h 2>/dev/null || true
	@echo "Clean complete!"

.PHONY: all clean info