BINDIR=/opt/homebrew/bin
LIBDIR=/opt/homebrew/lib
INCDIR=/opt/homebrew/include
CC=clang++
# NDEBUG is very important
CFLAGS=-fPIC --shared -undefined dynamic_lookup -std=c++20 -DNDEBUG

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