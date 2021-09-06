TARGET	=	kernel.elf
CPPSRCS	=	$(wildcard *.cpp usb/*.cpp usb/classdriver/*.cpp usb/xhci/*.cpp)
CSRCS   =   $(wildcard *.c)
ASMSRCS =   $(wildcard *.asm)
CPPOBJS	=	$(CPPSRCS:.cpp=.o)
COBJS   =   $(CSRCS:.c=.o)
COBJS   +=   hankaku_font.o
ASMOBJS =   $(ASMSRCS:.asm=.o)

CXXFLAGS	+=	-O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
				-fno-exceptions -fno-rtti -fno-threadsafe-statics -std=c++17
LDFLAGS		+=	--entry KernelMain -z norelro --image-base 0x100000 --static -lc

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean: 
	rm -rf *.o && rm $(TARGET)

$(TARGET):	$(COBJS) $(CPPOBJS) $(ASMOBJS) Makefile
	ld.lld $(LDFLAGS) -o $(TARGET) $(CPPOBJS) $(COBJS) $(ASMOBJS) -lc -lc++ -lc++abi
	mv $(TARGET) ../

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -I. -I.. -c $< -o $@

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -I. -I.. -c $< -o $@

%.o: %.asm Makefile
	nasm -f elf64 -o $@ $<






