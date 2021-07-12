TARGET	=	kernel.elf
SRCS	=	$(wildcard *.cpp)
OBJS	=	$(notdir $(SRCS:.cpp=.o))
INCDIR	=	"../"

CXXFLAGS	+=	-O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
				-fno-exceptions -fno-rtti -std=c++17
LDFLAGS		+=	--entry KernelMain -z norelro --image-base 0x100000 --static

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean: 
	rm -rf *.o && rm $(TARGET)

$(TARGET):	$(OBJS) Makefile
	ld.lld $(LDFLAGS) -o $(TARGET) $(OBJS)
	mv $(TARGET) ../

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -I$(INCDIR) -c $<


