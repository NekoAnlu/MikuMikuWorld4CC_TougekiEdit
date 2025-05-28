#pragma once

#include <cstdint>

namespace MikuMikuWorld
{
	constexpr uint64_t HASH_SEED = 0x3939cc00;
	// TODO: Make it uint64_t
	typedef int id_t;

	constexpr int TICKS_PER_BEAT = 480;

	constexpr float MEASURE_WIDTH = 30.0f;
	constexpr int MIN_LANE_WIDTH = 24;
	constexpr int MAX_LANE_WIDTH = 36;
	constexpr int MIN_NOTES_HEIGHT = 24;
	constexpr int MAX_NOTES_HEIGHT = 36;

	constexpr int MIN_TIME_SIGNATURE = 1;
	constexpr int MAX_TIME_SIGNATURE_NUMERATOR = 32;
	constexpr int MAX_TIME_SIGNATURE_DENOMINATOR = 64;
	constexpr float MIN_BPM = 10;
	constexpr float MAX_BPM = 10000;

	//Mod 添加自定义材质 不使用图集
	constexpr const char* BELL_TEX = "bell";
	constexpr const char* TEN_TEX = "ten";
	constexpr const char* FLICK_LEFT_TEX = "flick_left";
	constexpr const char* FLICK_RIGHT_TEX = "flick_right";
	constexpr const char* FLICK_UP_TEX = "flick_up";
	constexpr const char* DAMAKU_TEX = "danmaku";
	constexpr const char* DAMAKU_CENTER_TEX = "danmaku_center";
	constexpr const char* DAMAKU_LEFT_TEX = "danmaku_left";
	constexpr const char* DAMAKU_RIGHT_TEX = "danmaku_right";

	constexpr const char* NOTES_TEX = "notes1";
	constexpr const char* CC_NOTES_TEX = "notes2";
	constexpr const char* HOLD_PATH_TEX = "longNoteLine";
	constexpr const char* TOUCH_LINE_TEX = "touchLine_eff";
	constexpr const char* GUIDE_COLORS_TEX = "guideColors";
	constexpr const char* APP_CONFIG_FILENAME = "app_config.json";
	constexpr const char* IMGUI_CONFIG_FILENAME = "imgui_config.ini";

	constexpr const char* SUS_EXTENSION = ".sus";
	constexpr const char* USC_EXTENSION = ".usc";
	constexpr const char* MMWS_EXTENSION = ".mmws";
	constexpr const char* CC_MMWS_EXTENSION = ".ccmmws";
}
