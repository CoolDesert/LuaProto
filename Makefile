BINDIR?=/usr/bin
LIBDIR?=/usr/lib/x86_64-linux-gnu
INCDIR?=/usr/include
CC=g++
# Use C++20 standard with optimizations and NDEBUG
CFLAGS=-fPIC --shared -std=c++20 -O2 -DNDEBUG -Wall -Wextra

PROTO_SRCS=example.proto
PROTO_CC=$(PROTO_SRCS:.proto=.pb.cc)
PROTO_H=$(PROTO_SRCS:.proto=.pb.h)

all: proto.so
	@echo "Build complete!"

$(PROTO_CC) $(PROTO_H): $(PROTO_SRCS)
	@echo "Generating protobuf files from $<..."
	@$(BINDIR)/protoc --cpp_out=. $<
	@echo "Generated $(PROTO_CC) and $(PROTO_H)"

proto.so: LuaProto.cpp $(PROTO_CC)
	$(CC) $(CFLAGS) -o $@ $^ -I$(INCDIR) -L$(LIBDIR) -lprotobuf

clean:
	@echo "Cleaning build artifacts..."
	@rm -f *.o *.so *.pb.cc *.pb.h 2>/dev/null || true
	@echo "Clean complete!"

.PHONY: all clean