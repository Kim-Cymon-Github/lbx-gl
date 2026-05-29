NAME=lbx-gl

PLATFORM_ID ?= x64
STRIP ?= strip

#=============================================================================
# 소스/오브젝트 목록 (basename. BUILD_OUT prefix 는 아래에서 일괄 적용)
#=============================================================================
# 라이브러리 .c
OBJ_C_NAMES   = \
    lbx_gl \
    lbx_gl_dmabuf

# 라이브러리 .cpp — -fPIC 필요. CPP_LIB_OBJS 전용 룰에서 따로 컴파일.
OBJ_CPP_NAMES = \
    lbx_gl_class

OBJ_NAMES = $(OBJ_C_NAMES) $(OBJ_CPP_NAMES)

EXT_NAMES  =
TEST_NAMES = test_lbx_gl_main

#=============================================================================
# 경로
#=============================================================================
# 휘발성 산출물은 전부 build/linux/$(PLATFORM_ID)/ 한 곳. 중간 .o/.d +
# 테스트 바이너리 + tar.gz. make clean = rm -rf $(BUILD_OUT) 한 줄로 끝.
# 배포 라이브러리(.so/.a) 만 OUT_PATH (외부 -L 참조 위치) 로 mv.
BUILD_OUT     = $(PLATFORM_ID)

TARGET        = lib$(NAME).so
STATIC_TARGET = lib$(NAME).a
OUT_PATH      = ../../lib/lbx/lib/linux/$(PLATFORM_ID)

TEST_TARGET   = test$(NAME)

OBJS         = $(addprefix $(BUILD_OUT)/,$(addsuffix .o,$(OBJ_NAMES)))
EXT_OBJS     = $(addprefix $(BUILD_OUT)/,$(addsuffix .o,$(EXT_NAMES)))
TEST_OBJS    = $(addprefix $(BUILD_OUT)/,$(addsuffix .o,$(TEST_NAMES)))

# 라이브러리에 들어가는 cpp 만 -fPIC 가 필요. 테스트 cpp 와 분기하기 위해
# 따로 모아두고 static pattern rule 로 처리한다.
CPP_LIB_OBJS = $(addprefix $(BUILD_OUT)/,$(addsuffix .o,$(OBJ_CPP_NAMES)))

# 패턴 룰이 소스를 찾을 디렉토리 목록. 위에 적힌 순서대로 검색.
# basename 충돌 없을 때만 안전 — 현재 OBJ_NAMES 는 전부 unique.
vpath %.c   ../../src/gl
vpath %.cpp ../../src/gl
vpath %.cpp ../../test

INC_PATH += -I$(KERNEL_DIR)/include/uapi
INC_PATH += -I$(KERNEL_DIR)/include
INC_PATH += -I$(KERNEL_DIR)/arch/$(ARCH)/include/
INC_PATH += -I$(KERNEL_DIR)/arch/$(ARCH)/include/generated/

INC= \
    -I../../src/ \
    -I../../lib/lbx/include/ \
    $(INC_PATH) \

DEFS= \
    -DLINUX \
    -DENABLE_LOG \
    -D_LARGEFILE64_SOURCE \
    -D_FILE_OFFSET_BITS=64 \
    -DLBX_GLES_VERSION=32 \

#   -DGL_GLEXT_PROTOTYPES=1 \
#   -DMAX_PERFORMANCE_CHECK \
#   -DEGL_EGLEXT_PROTOTYPES=1 \

LDFLAGS=-L$(OUT_PATH) -lrt -lm -lpthread -lEGL -lGLESv2 -llbx-core

# 헤더 의존성 자동 추적용 플래그.
# -MMD: 시스템 헤더 제외, 프로젝트 헤더만 .d 로 기록 (빌드 그래프 가벼움)
# -MP : 헤더 삭제 시 "no rule to make target" 에러 방지용 phony 룰 자동 추가
# CFLAGS 에 합쳐서 모든 컴파일 룰 ($(CC)/$(CXX)) 이 자동으로 .d 를 떨군다.
DEPFLAGS := -MMD -MP

CFLAGS+=$(INC) $(DEFS) -Wall -Wstrict-aliasing=0 $(DEPFLAGS)

# gbm/libdrm 은 시스템에 있을 때만 켜짐. PKG_CONFIG 가 비어있으면 그대로 건너뜀.
ifneq ($(strip $(PKG_CONFIG)),)
    LDFLAGS += $(shell pkg-config --libs gbm libdrm)
    CFLAGS  += $(shell pkg-config --cflags gbm libdrm)
endif

ifeq ($(LBX_GPU), POWERVR)
    DEFS+=-DPOWERVR=1
endif

# 테스트 실행파일은 build/linux/$P/, 배포 라이브러리는 lib/lbx/lib/linux/$P/.
# 둘 사이 상대 깊이는 3단계 (linux→build→repo-root→lib/lbx/lib/linux/$P).
# rpath 박아넣어서 LD_LIBRARY_PATH 없이도 ./test$(NAME) 실행 가능.
TEST_RPATH := '$$ORIGIN/../../../lib/lbx/lib/linux/$(PLATFORM_ID)'

# x64 한정 옵션
# - ASan(+LSan): lbx-core 가 x64 에서 -fsanitize=address 로 빌드되므로 그걸
#   link/dlopen 하는 우리 .so 와 테스트도 ASan 일관 빌드 필수. 아니면 ASan
#   런타임이 메인 실행파일 최상위에 안 와서 "runtime does not come first" 거부.
# - strip: ASan 백트레이스에 심볼이 필요하므로 no-op(true)으로 대체.
ifeq ($(PLATFORM_ID),x64)
    SANITIZER_FLAGS := -fsanitize=address -fno-omit-frame-pointer -g
    CFLAGS  += $(SANITIZER_FLAGS)
    LDFLAGS += $(SANITIZER_FLAGS)
    LDFLAGS += -Wl,-rpath,$(TEST_RPATH)
    STRIP := true
endif


all: $(TARGET) $(STATIC_TARGET) $(TEST_TARGET)

lib: $(TARGET)
$(TARGET): $(OBJS) $(EXT_OBJS) | $(OUT_PATH)
	$(CXX) -shared -Wl,-soname,$(TARGET) -o $(BUILD_OUT)/$(TARGET) $(OBJS) $(EXT_OBJS) $(LDFLAGS)
	$(STRIP) $(BUILD_OUT)/$(TARGET)
	cd $(BUILD_OUT) && tar -czvf $(TARGET).tar.gz $(TARGET)
	mv $(BUILD_OUT)/$(TARGET) $(OUT_PATH)

static: $(STATIC_TARGET)
$(STATIC_TARGET): $(OBJS) $(EXT_OBJS) | $(OUT_PATH)
	$(AR) rcs $(BUILD_OUT)/$(STATIC_TARGET) $(OBJS) $(EXT_OBJS)
	$(STRIP) $(BUILD_OUT)/$(STATIC_TARGET)
	mv $(BUILD_OUT)/$(STATIC_TARGET) $(OUT_PATH)

test: $(TEST_TARGET)
$(TEST_TARGET): $(TEST_OBJS) | $(BUILD_OUT)
	$(CXX) $(LDFLAGS) -o $(BUILD_OUT)/$(TEST_TARGET) $(TEST_OBJS) -l$(NAME) -llbx-intf -llbx-core -ldl
	$(STRIP) $(BUILD_OUT)/$(TEST_TARGET)
	cd $(BUILD_OUT) && tar -czvf $(TEST_TARGET).tar.gz $(TEST_TARGET)

run: $(TARGET) $(TEST_TARGET)
#	rpath 로도 .so 찾도록 박혀있지만, 안전망으로 LD_LIBRARY_PATH 같이 지정
	LD_LIBRARY_PATH=$(OUT_PATH):$$LD_LIBRARY_PATH ./$(BUILD_OUT)/$(TEST_TARGET)

#=============================================================================
# 패턴 룰 — vpath 가 소스 위치를 찾아준다.
#=============================================================================
$(BUILD_OUT)/%.o: %.c | $(BUILD_OUT)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

# 라이브러리에 들어가는 cpp 는 -fPIC 필수. static pattern 으로 한정.
$(CPP_LIB_OBJS): $(BUILD_OUT)/%.o: %.cpp | $(BUILD_OUT)
	$(CXX) $(CFLAGS) -fPIC -c $< -o $@

# 테스트 등 라이브러리 외 .cpp 는 -fPIC 없이.
$(BUILD_OUT)/%.o: %.cpp | $(BUILD_OUT)
	$(CXX) $(CFLAGS) -c $< -o $@

$(BUILD_OUT) $(OUT_PATH):
	mkdir -p $@

clean:
	rm -rf $(BUILD_OUT)
	rm -f $(OUT_PATH)/$(TARGET)
	rm -f $(OUT_PATH)/$(STATIC_TARGET)

testc:
	rm -f $(TEST_OBJS)
	rm -f $(BUILD_OUT)/$(TEST_TARGET)

#=============================================================================
# 헤더 의존성 끼워넣기 — 첫 빌드엔 .d 없으니 -include 로 침묵 처리.
#=============================================================================
-include $(OBJS:.o=.d)
-include $(EXT_OBJS:.o=.d)
-include $(TEST_OBJS:.o=.d)
