#pragma once
#include <stdint.h>

namespace MikuMikuWorld
{
	enum class TimelineMode : uint8_t
	{
		Select,
		InsertSlide, // Renamed from InsertTap
		InsertLong,
		InsertLongMid,
		InsertFlick,
		MakeCritical,
		MakeFriction,
		InsertGuide,
		// InsertDamage, // Removed
		InsertBPM,
		InsertTimeSign,
		InsertHiSpeed,
		TimelineModeMax // This might need adjustment if enum count changes affect anything
	};

	constexpr const char* timelineModes[]{ "select", "slide",    "hold",           "hold_step", // Renamed "tap" to "slide"
		                                   "flick",  "critical", "trace",          "guide",
		                                   /*"damage",*/ "bpm",      "time_signature", "hi_speed" }; // Removed "damage"

	constexpr int divisions[]{ 4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192 };

	enum class SnapMode : uint8_t
	{
		Relative,
		Absolute,
		IndividualAbsolute,
		SnapModeMax
	};

	constexpr const char* snapModes[]{ "snap_mode_relative", "snap_mode_absolute",
		                               "snap_mode_individual_absolute" };

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
