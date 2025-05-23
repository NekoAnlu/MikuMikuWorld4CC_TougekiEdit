#include "Note.h"
#include "Constants.h"
#include "Score.h"
#include <algorithm>
#include <choc/memory/choc_xxHash.h>

namespace MikuMikuWorld
{
	int nextID = 1;

	int Note::getNextID()
	{
		uint8_t data[sizeof(int)];
		std::memcpy(data, &nextID, sizeof(int));
		nextID = choc::hash::xxHash64::hash(&data, sizeof(int), HASH_SEED + 1);
		return nextID;
	}

	Note::Note(NoteType _type)
	    : type{ _type }, parentID{ static_cast<id_t>(-1) }, tick{ 0 }, lane{ 0 }, width{ 3 },
	      critical{ false }, friction{ false }
	{
		if (type == NoteType::Slide)
			flick = FlickType::Default;
	}

	Note::Note(NoteType _type, int _tick, float _lane, float _width)
	    : type{ _type }, parentID{ static_cast<id_t>(-1) }, tick{ _tick }, lane{ _lane },
	      width{ _width }, critical{ false }, friction{ false }
	{
		if (type == NoteType::Slide)
			flick = FlickType::Default;
	}

	Note::Note()
	    : type{ NoteType::Slide }, parentID{ static_cast<id_t>(-1) }, tick{ 0 }, lane{ 0 },
	      width{ 3 }, critical{ false }, friction{ false }
	{
		// Slide notes are always flick by default
		flick = FlickType::Default;
	}

	bool Note::isFlick() const
	{
		// Slide notes are always flick.
		if (type == NoteType::Slide)
			return true;

		// HoldEnd can be flick if a flick type is assigned.
		if (type == NoteType::HoldEnd && flick != FlickType::None)
			return true;
		
		// Danmaku, Bell, Ten, Event are never flick.
		// Hold, HoldMid are never flick.
		return false;
	}

	bool Note::hasEase() const { return type == NoteType::Hold || type == NoteType::HoldMid; }

	bool Note::canFlick() const
	{
		// Determines if a note *can be* assigned/modified for flick property
		switch (type)
		{
		case NoteType::Slide:   // Slide notes are inherently flickable (e.g. to change direction)
		case NoteType::HoldEnd: // HoldEnd notes can be made flick
			return true;
		// Danmaku, Bell, Ten, Event cannot be made flick.
		// Hold, HoldMid cannot be made flick.
		case NoteType::Danmaku:
		case NoteType::Bell:
		case NoteType::Ten:
		case NoteType::Event:
		case NoteType::Hold:
		case NoteType::HoldMid:
		default:
			return false;
		}
	}

	bool Note::canTrace() const
	{
		// Determines if a note type can be part of a hold trace
		switch (type)
		{
		case NoteType::Slide: // Slide (formerly Tap) can initiate or be part of a trace
		case NoteType::Hold:
		case NoteType::HoldEnd:
			return true;
		// Other types are typically not part of traces
		case NoteType::Danmaku:
		case NoteType::Bell:
		case NoteType::Ten:
		case NoteType::Event:
		case NoteType::HoldMid:
		default:
			return false;
		}
	}

	void cycleFlick(Note& note)
	{
		if (note.getType() == NoteType::Hold || note.getType() == NoteType::HoldMid)
			return;

		note.flick = (FlickType)(((int)note.flick + 1) % (int)FlickType::FlickTypeCount);
	}

	void cycleStepEase(HoldStep& note)
	{
		note.ease = (EaseType)(((int)note.ease + 1) % (int)EaseType::EaseTypeCount);
	}

	void cycleStepType(HoldStep& note)
	{
		note.type = (HoldStepType)(((int)note.type + 1) % (int)HoldStepType::HoldStepTypeCount);
	}

	void sortHoldSteps(const Score& score, HoldNote& note)
	{
		std::stable_sort(note.steps.begin(), note.steps.end(),
		                 [&score](const HoldStep& s1, const HoldStep& s2)
		                 {
			                 const Note& n1 = score.notes.at(s1.ID);
			                 const Note& n2 = score.notes.at(s2.ID);
			                 return n1.tick == n2.tick ? n1.lane < n2.lane : n1.tick < n2.tick;
		                 });
	}

	int getFlickArrowSpriteIndex(const Note& note)
	{
		int startIndex = note.critical ? 24 : 12;
		return startIndex + ((std::min(note.width, 6.f) - 1) * 2) +
		       (note.flick != FlickType::Default ? 1 : 0);
	}

	int getNoteSpriteIndex(const Note& note)
	{
		int index = 3; // Default sprite index (e.g. for a normal tap-like note)

		switch (note.getType())
		{
		case NoteType::Slide:
			if (note.friction)
				index = note.critical ? 5 : 6; // Critical Friction Slide : Normal Friction Slide
			else if (note.critical)
				index = 0; // Critical Slide (assumed to be flick, sprite 0)
			else
				index = 1; // Normal Slide (assumed to be flick, sprite 1)
			break;

		case NoteType::Hold:
		case NoteType::HoldEnd: // HoldEnd might use flick arrow sprites if flickable, handled by getFlickArrowSpriteIndex
			index = 2; // Standard hold note sprite
			if (note.getType() == NoteType::HoldEnd && note.isFlick()) {
				// Potentially, flick arrow should be drawn on top by caller.
				// Or, return a specific sprite index for "HoldEnd with Flick".
				// For now, stick to base HoldEnd sprite. Caller handles arrow.
			}
			break;

		case NoteType::HoldMid:
			index = note.critical ? 8 : 7; // Critical Hold Mid : Normal Hold Mid
			break;

		case NoteType::Danmaku:
			// Danmaku notes are not flickable. Use tap-like sprites.
			index = note.critical ? 0 : 3; // Critical Danmaku : Normal Danmaku (using tap sprites 0 and 3)
			break;

		case NoteType::Bell:
			index = 12; // Placeholder sprite index for Bell
			break;

		case NoteType::Ten:
			// Ten notes are not flickable. Use tap-like sprites.
			index = note.critical ? 0 : 3; // Critical Ten : Normal Ten (using tap sprites 0 and 3)
			break;

		case NoteType::Event:
			index = -1; // No sprite for Event notes
			break;
		
		default:
			// Should not be reached if all types are handled
			index = 3;
			break;
		}
		return index;
	}

	int getCcNoteSpriteIndex(const Note& note)
	{
		int index = -1; // Default to no CC sprite
		// NoteType::Damage was removed, so no case for it.
		// Other types (Bell, Ten, etc.) are not CC notes by current definition.
		return index;
	}

	int getFrictionSpriteIndex(const Note& note)
	{
		// This function seems specific to friction notes that are separate visual elements.
		// Slide notes that are friction will have their sprite determined by getNoteSpriteIndex.
		// If this is for a generic "friction effect" sprite, its usage might need review.
		// Assuming it's for the distinct friction note visuals:
		if (note.getType() == NoteType::Slide && note.friction) { // Example
			return note.critical ? 10 : 11; // Critical friction slide effect : normal friction slide effect (using 11 instead of 9 for flick-like)
		}
		// Original logic for non-Slide friction notes (if any were intended to use this)
		// return note.critical ? 10 : note.flick != FlickType::None ? 11 : 9;
		// Since Slide is always flick, this simplifies for Slide:
		return note.critical ? 10 : 11; // Fallback or general friction sprite
	}

	int findHoldStep(const HoldNote& note, int stepID)
	{
		for (int index = 0; index < note.steps.size(); ++index)
		{
			if (note.steps[index].ID == stepID)
				return index;
		}

		return -1;
	}

	std::string_view getNoteSE(const Note& note, const Score& score)
	{
		switch (note.getType())
		{
		case NoteType::Slide:
			// Slide notes are always flick. Friction takes precedence.
			if (note.friction)
				return note.critical ? SE_CRITICAL_FRICTION : SE_FRICTION;
			else
				return note.critical ? SE_CRITICAL_FLICK : SE_FLICK;

		case NoteType::Hold: // SE for tapping a hold note start
			return note.critical ? SE_CRITICAL_CONNECT : SE_CONNECT; // Or perfect/critical perfect

		case NoteType::HoldMid:
			{
				const HoldNote& hold = score.holdNotes.at(note.parentID);
				int pos = findHoldStep(hold, note.ID);
				if (pos != -1 && hold.steps[pos].type == HoldStepType::Hidden)
					return "";
				return note.critical ? SE_CRITICAL_TICK : SE_TICK;
			}

		case NoteType::HoldEnd:
			// HoldEnd can be flick or normal.
			if (note.isFlick()) // isFlick() checks type and flick property
				return note.critical ? SE_CRITICAL_FLICK : SE_FLICK;
			else if (note.friction) // HoldEnd can also be friction (if design allows)
				return note.critical ? SE_CRITICAL_FRICTION : SE_FRICTION;
			else
				return note.critical ? SE_CRITICAL_CONNECT : SE_CONNECT; // Standard HoldEnd sound

		case NoteType::Danmaku:
		case NoteType::Ten:
			// Not flickable. Can be critical.
			// SE_CRITICAL_SLIDE is the renamed SE_CRITICAL_TAP
			return note.critical ? SE_CRITICAL_SLIDE : SE_PERFECT;

		case NoteType::Bell:
			return "bell_se"; // Placeholder SE for Bell

		case NoteType::Event:
			return ""; // Events have no sound by default
		
		default:
			// Should ideally not be reached.
			return SE_PERFECT;
		}
	}
	}
}
