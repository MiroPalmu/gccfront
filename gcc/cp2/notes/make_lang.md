# Inclomplete list of parts in Make-lang.in

# Driver

Driver objects variables:
    - GCCRS_D_OBJS
    - GCCGO_OBJS
    - GXX2_D_OBJS

All contain the same objects:
    - $(GCC_OBJS)
    - lang/*spec.o

Which are given to the driver target.

# Objects

Variables:
    - GRS_OBJS
    - GO_OBJS
    - CXX2_OBJS 

All contain the same objects in form: `lang/*.o`,
but no `lang/*spec.o`.

Rust defines RUST_ALL_OBJS containing GRS_OBJS
and RUST_TARGET_OBJS which is not defined in its Make-lang.in.

All mandatory lang_OBJS contain:
	- *_OBJS
		- RUST_ALL_OBJS (~= GRS_OBJS)
		- GO_OBJS
		- CXX2_OBJS
	- lang/*spec.o
		- rust/rustspec.o
		- go/gospec.o
		- cp2/g++2spec.o

# Compiler proper

Compiler proper dependencies:
	- RUST_ALL_OBJS (~= GRS_OBJS)
	- GO_OBJS 
	- CXX2_OBJS

# Rust

## Driver

GCCRS_D_OBJS = $(GCC_OJBS) rust/rustspec.o

gccrs: $(gccrs_D_OBJS)

## Objects

GRS_OBJS = `rust/*.o` (no rust/rustspec.o)
RUST_ALL_OBJS = $(GRS_OBJS) $(RUST_TARGET_OBJS???)

rust_OBJS = $(RUST_ALL_OBJS) rust/rustspec.o

## Compiler proper

crab1: $(RUST_ALL_OBJS)

# GO

## Driver

GCCGO_OBJS = $(GCC_OBJS) go/gospec.o

gccgo: $(GCCGO_OBJS)

# Compiler proper

GO_OBJS = `go/*.o` (no go/gospec.o)

go_OBJS = $(GO_OBJS) go/gospec.o

## Compiler proper

go1: $(GO_OBS)

# C++2

## Driver

GXX2_D_OBJS = $(GCC_OBJS) cp2/g++2spec.o

g++2: $(GXX2_D_OBJS)

## Objects

CXX2_OBJS = `cp2/*.o` (no cp2/g++2spec.o)

c++2_OBJS = $(CXX2_OBJS) cp2/g++2spec.o

## Compiler proper

cc1plus2: $(CXX2_ALL_OBJS)
