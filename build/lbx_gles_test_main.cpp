#include <stdio.h>
#include "lbx_gles2.h"
#include "system/lbx_log.h"
#include "system/lbx_console.h"
#include "lbx_core.h"
#include "system/lbx_file.h"

#include "lbx_gles_test_routines.h"


#pragma comment (lib, "lbx-gles.lib")
#pragma comment (lib, "lbx-core.lib")

// SGX 라이브러리에서 가져온 함수
void LBX_API log_handler(uint8_t type, const char* unit, const char* func, int line, const char* fmt, va_list args)
{
	static const f32_t p_unit = 0.2f, p_func = 0.12f; // percent
	static i32_t c_unit = 0, c_func = 0, c_msg = 0;
	static UString fmt_str;
	static size2_i16 last_console_size = { 0,0 };
	static u32_t last_console_tick = -1000; // 맨 처음엔 무조건 한 번 실행되도록

	//    UString s;
	char buf_msg[256], buf_unit[64], buf_func[64];
	u32_t tk = tick_now();
	if (tk - last_console_tick >= 1000) {
		size2_i16 console_size = get_console_size();
		if (console_size.width != last_console_size.width) {
			f32_t w = (f32_t)console_size.width;
			c_unit = (i32_t)(w * p_unit);
			c_unit = LBX_MIN(c_unit, sizeof(buf_unit) - 1);
			c_func = (i32_t)(w * p_func);
			c_func = LBX_MIN(c_func, sizeof(buf_func) - 1);
			//                           c_  tk  _msg|     func   |     unit   : line  
			c_msg = console_size.width - 2 - 9 - 1 - 1 - c_func - 1 - c_unit - 1 - 5;
			c_msg = LBX_MIN(c_msg, sizeof(buf_msg) - 1);
			fmt_str.printf("%%c %%5d.%%03d %%-%ds %%%ds|%%%ds:%%-5d\n", c_msg, c_func, c_unit);
			last_console_size = console_size;
		}
		last_console_tick = tk;
	}
	i32_t l = vsnprintf(buf_msg, sizeof(buf_msg), fmt, args);
	//    s.vprintf(fmt, args);
	printf(fmt_str.c_str(), type, tk / 1000, tk % 1000, ellipse_strl(buf_msg, l, c_msg, 2), ellipse_cstr(func, buf_func, c_func, 2), ellipse_cstr(extract_file_name(unit), buf_unit, c_unit, 2), line);
}


i32_t main(void)
{
	check_memory_usage(true); // reset
	tick_set_start(); // tick 초기화
	change_log_handler(&log_handler); // 로그 핸들러 연결
#ifdef _WIN32
	system("chcp 65001");
#endif //#ifdef _WIN32

	test();

    Log_("lbx-gles library function test");

	check_memory_usage(true); // reset
}