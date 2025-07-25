#pragma once
#include <stdint.h>

namespace MikuMikuWorld
{
	enum class TimelineMode : uint8_t
	{
		Select,
		InsertTap,
		MakeCritical,
		InsertFlick,
		InsertLong,
		InsertGuide,
		InsertLongMid,
		InsertDamage,
		InsertBPM,
		InsertTimeSign,
		InsertHiSpeed,
		TimelineModeMax,
		// 以下为被删除按键
		MakeFriction,
	};

	//enum class TimelineMode : uint8_t
	//{
	//	Select,
	//	InsertTap,
	//	InsertLong,
	//	InsertLongMid,
	//	InsertFlick,
	//	MakeCritical,
	//	MakeFriction,
	//	InsertGuide,
	//	InsertDamage,
	//	InsertBPM,
	//	InsertTimeSign,
	//	InsertHiSpeed,
	//	TimelineModeMax
	//};

	/*constexpr const char* timelineModes[]{ "select", "tap",      "hold",           "hold_step",
		                                   "flick",  "critical", "trace",          "guide",
		                                   "damage", "bpm",      "time_signature", "hi_speed" };*/
	//mod 删除不需要的按键
	constexpr const char* timelineModes[]{ "select", "tap", "critical","flick", "hold", "guide","hold_step",
										   "damage", "bpm","time_signature", "hi_speed" };

	constexpr int divisions[]{ 4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192 };

	enum class SnapMode : uint8_t
	{
		Relative,
		Absolute,
		IndividualAbsolute,
		SnapModeMax
	};

	enum class LaneSnapMode : uint8_t
	{
		NoSnap,
		Relative,
		LaneSnapModeMax
	};

	constexpr const char* snapModes[]{ "snap_mode_relative", "snap_mode_absolute",
		                               "snap_mode_individual_absolute" };
	constexpr const char* laneSnapModes[]{ "no_snap", "lanesnap_mode_relative"};

	constexpr int timeSignatureDenominators[]{ 2, 4, 8, 16, 32, 64 };

	enum class EventType : uint8_t
	{
		None,
		Bpm,
		TimeSignature,
		HiSpeed,
		Skill,
		Fever,
		Waypoint,
		EventTypeMax
	};

	constexpr const char* eventTypes[]{ "none",  "bpm",   "time_signature", "hi_speed",
		                                "skill", "fever", "waypoint" };

	enum class Direction : uint8_t
	{
		Down,
		Up,
		DirectionMax
	};
}
