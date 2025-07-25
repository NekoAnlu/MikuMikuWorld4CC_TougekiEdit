#pragma once
#include "Constants.h"
#include "NoteTypes.h"
#include <string>
#include <vector>
#include <stdexcept>

namespace MikuMikuWorld
{
	constexpr int MIN_NOTE_WIDTH = 0;
	constexpr int MAX_NOTE_WIDTH = 20;
	constexpr int MIN_LANE = 0;
	constexpr int MAX_LANE = 14;
	constexpr int NUM_LANES = 14;

	constexpr const char* SE_PERFECT = "perfect";
	constexpr const char* SE_FLICK = "flick";
	constexpr const char* SE_TICK = "tick";
	constexpr const char* SE_FRICTION = "friction";
	constexpr const char* SE_CONNECT = "connect";
	constexpr const char* SE_CRITICAL_TAP = "critical_tap";
	constexpr const char* SE_CRITICAL_FLICK = "critical_flick";
	constexpr const char* SE_CRITICAL_TICK = "critical_tick";
	constexpr const char* SE_CRITICAL_FRICTION = "critical_friction";
	constexpr const char* SE_CRITICAL_CONNECT = "critical_connect";

	constexpr const char* SE_NAMES[] = { SE_PERFECT,         SE_FLICK,         SE_TICK,
		                                 SE_FRICTION,        SE_CONNECT,       SE_CRITICAL_TAP,
		                                 SE_CRITICAL_FLICK,  SE_CRITICAL_TICK, SE_CRITICAL_FRICTION,
		                                 SE_CRITICAL_CONNECT };

	constexpr float flickArrowWidths[] = { 0.95f, 1.25f, 1.8f, 2.3f, 2.6f, 3.2f };

	constexpr float flickArrowHeights[] = { 1, 1.05f, 1.2f, 1.4f, 1.5f, 1.6f };

	enum class ZIndex : int32_t
	{
		// mod 修改guide在最下
		Guide,
		HoldLine,
		HoldTick,
		Note,
		FrictionTick,
		zCount
	};

	struct NoteTextures
	{
		int notes;
		int holdPath;
		int touchLine;
		int ccNotes;
		int guideColors;
		//mod
		int bell;
		int ten;
		int danmaku;
		int danmaku_center;
		int danmaku_left;
		int danmaku_right;
		int flick_left;
		int flick_right;
		int flick_up;
	};

	extern NoteTextures noteTextures;

	class Note
	{
	  private:
		NoteType type;
		//-------------Mod 添加属性
		// 是否可以被调整大小
		//bool resizeAble;

	  public:
		static int getNextID();

		id_t ID;
		id_t parentID;
		int tick;
		float lane;
		float width;
		bool critical{ false };
		bool friction{ false };
		//-------------Mod 添加属性
		// 额外速度
		float extraSpeed;
		// 是否可以被调整宽度
		bool resizeAble{ false };
		// Damage类型
		DamageType damageType { DamageType::Circle };
		DamageDirection damageDirection { DamageDirection::None };
		//-------------Mod End

		FlickType flick{ FlickType::None };

		int layer{ 0 };

		explicit Note(NoteType _type);
		explicit Note(NoteType _type, int tick, float lane, float width);
		Note();

		constexpr NoteType getType() const { return type; }
		constexpr bool isHold() const
		{
			return type == NoteType::Hold || type == NoteType::HoldMid || type == NoteType::HoldEnd;
		}

		bool isFlick() const;
		bool hasEase() const;
		bool canFlick() const;
		bool canTrace() const;


	};

	struct HoldStep
	{
		id_t ID;
		HoldStepType type;
		EaseType ease;
	};

	class HoldNote
	{
	  public:
		HoldStep start;
		std::vector<HoldStep> steps;
		id_t end;

		HoldNoteType startType{};
		HoldNoteType endType{};

		// mod Hold当event额外属性且都是公共的
		HoldEventType holdEventType{};
		int colorsetID;
		bool highlight;
		std::string colorInHex{ "#000000" };
		// mod end

		FadeType fadeType{ FadeType::Out };
		GuideColor guideColor{ GuideColor::Black };


		constexpr bool isGuide() const
		{
			return startType == HoldNoteType::Guide || endType == HoldNoteType::Guide;
		}

		// mod
		constexpr bool isLaser() const
		{
			return holdEventType == HoldEventType::Event_Laser;
		}

		// mod
		constexpr bool isWarning() const
		{
			return holdEventType == HoldEventType::Event_Warning;
		}

		/**
		 * @brief Retrieve HoldStep according to given `index` within `[-1, steps.size()-1]`,
		 *        where -1 stands for the start step
		 * @throw `std::out_of_range` if `index` is invalid
		 * @warning Reference returned by this method can be invalidated by vector reallocation
		 */
		HoldStep& operator[](int index)
		{
			if (index < -1 || index >= (int)steps.size())
				throw std::out_of_range("Index out of range in HoldNote[]");
			return index == -1 ? start : steps[index];
		}
		/**
		 * @brief Retrieve HoldStep according to given `index` within `[-1, steps.size()-1]`,
		 *        where -1 stands for the start step
		 * @throw `std::out_of_range` if `index` is invalid
		 * @warning Reference returned by this method can be invalidated by vector reallocation
		 */
		const HoldStep& operator[](int index) const
		{
			if (index < -1 || index >= (int)steps.size())
				throw std::out_of_range("Index out of range in HoldNote[]");
			return index == -1 ? start : steps[index];
		}
		/**
		 * @brief Retrieve note ID according to given `index` within `[-1, steps.size()]`,
		 *        where -1 stands for the start note and `steps.size()` stands for the end note
		 * @throw `std::out_of_range` if `index` is invalid
		 */
		int id_at(int index) const
		{
			if (index < -1 || index > (int)steps.size())
				throw std::out_of_range("Index out of range in HoldNote::id_at");
			return (index == steps.size()) ? end : (index == -1 ? start.ID : steps[index].ID);
		}
	};

	struct Score;

	void cycleFlick(Note& note);
	void cycleStepEase(HoldStep& note);
	void cycleStepType(HoldStep& note);
	void sortHoldSteps(const Score& score, HoldNote& note);
	int findHoldStep(const HoldNote& note, int stepID);

	int getFlickArrowSpriteIndex(const Note& note);
	int getNoteSpriteIndex(const Note& note);
	int getCcNoteSpriteIndex(const Note& note);
	int getFrictionSpriteIndex(const Note& note);
	std::string_view getNoteSE(const Note& note, const Score& score);
}
