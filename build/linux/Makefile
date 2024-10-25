NAME=lbx-gl

PLATFORM_ID ?= x64
STRIP ?= strip

OBJS=\
	lbx_gl.o \
	lbx_gles2.o \

EXT_OBJS=\

TEST_OBJS=\
	lbx_gles_test_main.o \

TARGET = lib$(NAME).so
STATIC_TARGET = lib$(NAME).a
OUT_PATH = ../../lib/lbx/lib/linux/$(PLATFORM_ID)

TEST_TARGET = test$(NAME)
TEST_OUT_PATH = ../../test/linux/$(PLATFORM_ID)

INC_PATH += -I$(KERNEL_DIR)/include/uapi
INC_PATH += -I$(KERNEL_DIR)/include
INC_PATH += -I$(KERNEL_DIR)/arch/$(ARCH)/include/
INC_PATH += -I$(KERNEL_DIR)/arch/$(ARCH)/include/generated/

INC= \
	-I../../src \
	-I../../lib/lbx/include \
	$(INC_PATH) \

DEFS= \
	-DLINUX \
	-DGLES=31\
	-DENABLE_LOG \
	-D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 \

#	-DLBX_TRACE_MEMORY \
#	-DGL_GLEXT_PROTOTYPES=1 \
#	-DMAX_PERFORMANCE_CHECK \
#	-DEGL_EGLEXT_PROTOTYPES=1 \

#GLFLAGS+=-lEGL -lGLESv2

LDFLAGS=-L$(OUT_PATH) -lrt -lm -lpthread -llbx-core -lEGL -lGLESv2

CFLAGS+=$(INC) $(DEFS) -Wall -Wstrict-aliasing=0

#-Wall 

ifeq ($(LBX_GPU), POWERVR)
	DEFS+=-DPOWERVR=1
endif


all: $(TARGET) $(STATIC_TARGET) $(TEST_TARGET)

lib: $(TARGET)
$(TARGET): $(OBJS) $(EXT_OBJS)
	mkdir -p $(OUT_PATH)
	mkdir -p $(TEST_OUT_PATH)
	$(CXX) -shared -Wl,-soname,$(TARGET) -o $(TARGET) $(OBJS) $(EXT_OBJS) $(LDFLAGS)
	$(STRIP) $(TARGET)
	tar -czvf $(TARGET).tar.gz $(TARGET)
#	mv $(TARGET).2 $(TARGET)
	mv $(TARGET) $(OUT_PATH)
#	rm $(TARGET)

static: $(STATIC_TARGET)
$(STATIC_TARGET): $(OBJS) $(EXT_OBJS)
	mkdir -p $(OUT_PATH)
	$(AR) rcs $(STATIC_TARGET) $(OBJS) $(EXT_OBJS)
	$(STRIP) $(STATIC_TARGET)
#	mv $(STATIC_TARGET) $(STATIC_TARGET)
	mv $(STATIC_TARGET) $(OUT_PATH)

test: $(TEST_TARGET)
$(TEST_TARGET): $(TEST_OBJS)
	mkdir -p $(OUT_PATH)
	mkdir -p $(TEST_OUT_PATH)
	$(CXX) -o $(TEST_TARGET) $(TEST_OBJS) $(GLFLAGS) $(LDFLAGS) -l$(NAME) -ldl
	$(STRIP) $(TEST_TARGET)
	tar -czvf $(TEST_TARGET).tar.gz $(TEST_TARGET)
#	rm $(TEST_TARGET)

run: $(TARGET) $(TEST_TARGET)
#	PATH 환경변수를 수정하고 프로그램을 실행
	LD_LIBRARY_PATH=$(OUT_PATH):$$LD_LIBRARY_PATH ./$(TEST_TARGET)

#TEST_OBJS
lbx_gles_test_main.o: ../../test/lbx_gles_test_main.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

#OBJS
lbx_gl.o: ../../src/lbx_gl.c 
	$(CC) $(CFLAGS) -fPIC -c $< -o $@
lbx_gles2.o: ../../src/lbx_gles2.cpp
	$(CXX) $(CFLAGS) -fPIC -c $< -o $@

#EXT_OBJS


#ETC
clean:
	rm -rf $(OBJS) $(EXT_OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET) $(STATIC_TARGET)
	rm -f $(TARGET).tar.gz
	rm -f $(TEST_TARGET).tar.gz
	rm -f $(OUT_PATH)/$(TARGET)
	rm -f $(OUT_PATH)/$(STATIC_TARGET)

testc:
	rm -f $(TEST_OBJS)
	rm -f $(TEST_TARGET)
