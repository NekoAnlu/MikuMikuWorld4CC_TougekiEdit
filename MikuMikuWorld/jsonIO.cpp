#include "JsonIO.h"

using namespace nlohmann;

namespace jsonIO
{
	/// <summary>
	/// cv��
	/// </summary>
	/// <param name="note"></param>
	/// <returns></returns>
	mmw::Note jsonToNote(const json& data, mmw::NoteType type)
	{
		mmw::Note note(type);

		note.tick = tryGetValue<int>(data, "tick", 0);
		note.lane = tryGetValue<float>(data, "lane", 0);
		note.width = tryGetValue<float>(data, "width", 3);
		//mod
		note.extraSpeed = tryGetValue<float>(data, "extraSpeed", 1);
		note.resizeAble = tryGetValue<int>(data, "resizeAble", 1) == 1 ? true : false;
		note.damageType = static_cast<mmw::DamageType>(tryGetValue<int>(data, "damageType", 0));
		note.damageDirection = static_cast<mmw::DamageDirection>(tryGetValue<int>(data, "damageDirection", 0));


		if (note.getType() != mmw::NoteType::HoldMid)
		{
			note.critical = tryGetValue<bool>(data, "critical", false);
			note.friction = tryGetValue<bool>(data, "friction", false);
		}

		if (!note.hasEase())
		{
			std::string flickString = tryGetValue<std::string>(data, "flick", "none");
			std::transform(flickString.begin(), flickString.end(), flickString.begin(), ::tolower);

			if (flickString == "up" || flickString == "default")
				note.flick = mmw::FlickType::Default;
			else if (flickString == "left")
				note.flick = mmw::FlickType::Left;
			else if (flickString == "right")
				note.flick = mmw::FlickType::Right;
		}

		return note;
	}


	/// <summary>
	/// cv��
	/// </summary>
	/// <param name="note"></param>
	/// <returns></returns>
	static json noteToJson(const mmw::Note& note)
	{
		json data;
		data["tick"] = note.tick;
		data["lane"] = note.lane;
		data["width"] = note.width;
		//mod
		data["extraSpeed"] = note.extraSpeed;
		data["resizeAble"] = note.resizeAble ? 1 : 0;
		data["damageType"] = note.damageType;
		data["damageDirection"] = note.damageDirection;

		if (note.getType() != mmw::NoteType::HoldMid)
		{
			data["critical"] = note.critical;
			data["friction"] = note.friction;
		}

		if (!note.hasEase())
		{
			data["flick"] = mmw::flickTypes[(int)note.flick];
		}

		return data;
	}

	json noteSelectionToJson(const mmw::Score& score,
	                         const std::unordered_set<mmw::id_t>& selection,
	                         const std::unordered_set<mmw::id_t>& hiSpeedSelection, int baseTick)
	{
		json data, notes, holds, damages, hiSpeedChanges;
		std::unordered_set<mmw::id_t> selectedNotes;
		std::unordered_set<mmw::id_t> selectedHolds;
		std::unordered_set<mmw::id_t> selectedDamages;

		for (mmw::id_t id : selection)
		{
			if (score.notes.find(id) == score.notes.end())
				continue;

			const mmw::Note& note = score.notes.at(id);
			switch (note.getType())
			{
			case mmw::NoteType::Tap:
				selectedNotes.insert(note.ID);
				break;
			case mmw::NoteType::Hold:
				selectedHolds.insert(note.ID);
				break;

			case mmw::NoteType::HoldMid:
			case mmw::NoteType::HoldEnd:
				selectedHolds.insert(note.parentID);
				break;

			case mmw::NoteType::Damage:
				selectedDamages.insert(note.ID);
				break;
			default:
				break;
			}
		}

		for (mmw::id_t id : selectedNotes)
		{
			const mmw::Note& note = score.notes.at(id);
			json data = noteToJson(note);
			data["tick"] = note.tick - baseTick;

			notes.push_back(data);
		}
		for (mmw::id_t id : selectedDamages)
		{
			const mmw::Note& note = score.notes.at(id);
			json data = noteToJson(note);
			data["tick"] = note.tick - baseTick;

			damages.push_back(data);
		}
		for (int id : hiSpeedSelection)
		{
			const mmw::HiSpeedChange& note = score.hiSpeedChanges.at(id);
			data["tick"] = note.tick - baseTick;
			data["speed"] = note.speed;

			hiSpeedChanges.push_back(data);
		}

		for (mmw::id_t id : selectedHolds)
		{
			const mmw::HoldNote& hold = score.holdNotes.at(id);
			const mmw::Note& start = score.notes.at(hold.start.ID);
			const mmw::Note& end = score.notes.at(hold.end);

			json holdData, stepsArray;

			json holdStart = noteToJson(start);
			holdStart["tick"] = start.tick - baseTick;
			holdStart["ease"] = mmw::easeTypes[(int)hold.start.ease];
			holdStart["type"] = mmw::holdTypes[(int)hold.startType];

			for (auto& step : hold.steps)
			{
				const mmw::Note& mid = score.notes.at(step.ID);
				json stepData = noteToJson(mid);
				stepData["tick"] = mid.tick - baseTick;
				stepData["type"] = mmw::stepTypes[(int)step.type];
				stepData["ease"] = mmw::easeTypes[(int)step.ease];

				stepsArray.push_back(stepData);
			}

			json holdEnd = noteToJson(end);
			holdEnd["tick"] = end.tick - baseTick;
			holdEnd["type"] = mmw::holdTypes[(int)hold.endType];

			holdData["start"] = holdStart;
			holdData["steps"] = stepsArray;
			holdData["end"] = holdEnd;
			holdData["fade"] = mmw::fadeTypes[(int)hold.fadeType];
			holdData["guide"] = mmw::guideColors[(int)hold.guideColor];

			//mod
			holdData["holdEventType"] = hold.holdEventType;
			holdData["colorsetID"] = hold.colorsetID;
			holdData["highlight"] = hold.highlight ? 1 : 0;
			holdData["colorInHex"] = hold.colorInHex;

			holds.push_back(holdData);
		}

		data["notes"] = notes;
		data["holds"] = holds;
		data["damages"] = damages;
		data["hiSpeedChanges"] = hiSpeedChanges;
		return data;
	}
}
