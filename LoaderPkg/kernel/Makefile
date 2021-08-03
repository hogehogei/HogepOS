TARGET	=	kernel.elf
CPPSRCS	=	$(wildcard *.cpp)
CSRCS   =   $(wildcard *.c)
ASMSRCS =   $(wildcard *.asm)
CPPOBJS	=	$(notdir $(CPPSRCS:.cpp=.o))
COBJS   =   $(notdir $(CSRCS:.c=.o))
COBJS   +=   hankaku_font.o
ASMOBJS =   $(notdir $(ASMSRCS:.asm=.o))
INCDIR	=	"../"

CXXFLAGS	+=	-O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
				-fno-exceptions -fno-rtti -std=c++17
LDFLAGS		+=	--entry KernelMain -z norelro --image-base 0x100000 --static -lc

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean: 
	rm -rf *.o && rm $(TARGET)

$(TARGET):	$(COBJS) $(CPPOBJS) $(ASMOBJS) Makefile
	ld.lld $(LDFLAGS) -o $(TARGET) $(CPPOBJS) $(COBJS) $(ASMOBJS)
	mv $(TARGET) ../

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -I$(INCDIR) -c $<

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -I$(INCDIR) -c $<

%.o: %.asm Makefile
	nasm -f elf64 -o $@ $<






