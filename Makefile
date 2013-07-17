NAME=debayer
CXXFLAGS=-Wall -std=c++0x
INCLUDES=-I/opt/vc/include \
		 -I/opt/vc/include/interface/vcos/pthreads \
		 -I/opt/vc/include/interface/vmcs_host/linux \
		 -I/usr/include
LDFLAGS=-L/opt/vc/lib -lGLESv2 -lEGL -lbcm_host `pkg-config --libs sdl` -lopencv_core -lopencv_imgproc -lopencv_highgui
SRCS=texture.cpp debayer.cpp
OBJS=$(SRCS:%.cpp=%.o)

INCDIR=-I./Common -I$(SDKSTAGE)/opt/vc/include -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads
LIBS=-lGLESv2 -lEGL -lm -lbcm_host -L$(SDKSTAGE)/opt/vc/lib


all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

texture.o: debayer.h
debayer.o: debayer.h

clean:
	rm -f $(OBJS)
	rm -f $(NAME)
