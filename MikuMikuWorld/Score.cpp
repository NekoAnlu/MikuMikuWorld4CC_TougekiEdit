#include <choc/memory/choc_xxHash.h>
#include "Score.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "Constants.h"
#include "File.h"
#include "IO.h"
#include <unordered_set>

using namespace IO;

namespace MikuMikuWorld
{
	id_t nextSkillID = 1;
	id_t nextHiSpeedID = 1;
	id_t getNextSkillID()
	{
		uint8_t data[sizeof(id_t)];
		std::memcpy(data, &nextSkillID, sizeof(id_t));
		nextSkillID = choc::hash::xxHash64::hash(&data, sizeof(id_t), HASH_SEED + 2);
		return nextSkillID;
	}
	id_t getNextHiSpeedID()
	{
		uint8_t data[sizeof(id_t)];
		std::memcpy(data, &nextHiSpeedID, sizeof(id_t));
		nextHiSpeedID = choc::hash::xxHash64::hash(&data, sizeof(id_t), HASH_SEED + 3);
		return nextHiSpeedID;
	}

	enum NoteFlags
	{
		NOTE_CRITICAL = 1 << 0,
		NOTE_FRICTION = 1 << 1
	};

	enum HoldFlags
	{
		HOLD_START_HIDDEN = 1 << 0,
		HOLD_END_HIDDEN = 1 << 1,
		HOLD_GUIDE = 1 << 2
	};

	// Define the new Cyanvas version. This will be used in serialize/deserialize.
	constexpr int MMW_NEW_CYANVAS_VERSION = 7;

	Score::Score()
	{
		metadata.title = "";
		metadata.author = "";
		metadata.artist = "";
		metadata.musicOffset = 0;

		tempoChanges.push_back(Tempo());
		timeSignatures[0] = { 0, 4, 4 };
		auto id = getNextHiSpeedID();
		hiSpeedChanges[id] = HiSpeedChange{ id, 0, 1.0f, 0 };

		fever.startTick = fever.endTick = -1;
	}

	Note readNote(NoteType type, BinaryReader* reader, int cyanvasVersion)
	{
		// printf("%d\n", cyanvasVersion);

		Note note(type);

		if (cyanvasVersion <= 5)
		{
			note.tick = reader->readUInt32();
			note.lane = (float)reader->readInt32();
			note.width = (float)reader->readUInt32();
		}
		else
		{
			note.tick = reader->readUInt32();
			note.lane = reader->readSingle();
			note.width = reader->readSingle();
		}

		if (cyanvasVersion >= 4)
			note.layer = reader->readUInt32();

		if (!note.hasEase())
			note.flick = (FlickType)reader->readUInt32();

		unsigned int flags = reader->readUInt32();
		note.critical = (bool)(flags & NOTE_CRITICAL);
		note.friction = (bool)(flags & NOTE_FRICTION);

		// Type-specific properties for newer versions
		if (cyanvasVersion >= MMW_NEW_CYANVAS_VERSION)
		{
			if (note.getType() == NoteType::Event)
			{
				note.eventName = reader->readString();
			}
			else if (note.getType() == NoteType::Danmaku)
			{
				note.ExtraSpeed = reader->readSingle();
			}
		}
		
		// If it's a Slide note (formerly Tap), ensure it has a default flick if read from older version
		// or if type was just determined.
		// For MMW_NEW_CYANVAS_VERSION and above, Slide notes are expected to have their flick type written.
		// This logic might be better placed after note creation in deserializeScore for older versions.
		if (note.getType() == NoteType::Slide && cyanvasVersion < MMW_NEW_CYANVAS_VERSION)
		{
			// Older Tap notes didn't explicitly save FlickType::Default if it was None,
			// but Slide must be flick.
			if (note.flick == FlickType::None)
				note.flick = FlickType::Default;
		}


		return note;
	}

	// writeNote now takes cyanvasVersion to decide format
	void writeNote(const Note& note, BinaryWriter* writer, int cyanvasVersion)
	{
		if (cyanvasVersion >= MMW_NEW_CYANVAS_VERSION)
		{
			writer->writeByte((uint8_t)note.getType());
		}
		// For older versions, type is implicit from the block (tap, damage) or fixed (hold parts)

		writer->writeInt32(note.tick);
		writer->writeSingle(note.lane);
		writer->writeSingle(note.width);
		writer->writeInt32(note.layer);

		// Slide notes always have a flick type, even if it's Default.
		// Other non-hold types might or might not.
		// HoldStart/Mid don't have flick written here (hasEase is true).
		if (!note.hasEase()) // True for Slide, Danmaku, Ten, Event, HoldEnd
		{
			writer->writeInt32((int)note.flick);
		}
		
		unsigned int flags{};
		if (note.critical)
			flags |= NOTE_CRITICAL;
		if (note.friction)
			flags |= NOTE_FRICTION;
		writer->writeInt32(flags);

		// Type-specific properties for newer versions
		if (cyanvasVersion >= MMW_NEW_CYANVAS_VERSION)
		{
			if (note.getType() == NoteType::Event)
			{
				writer->writeString(note.eventName);
			}
			else if (note.getType() == NoteType::Danmaku)
			{
				writer->writeSingle(note.ExtraSpeed);
			}
		}
	}

	ScoreMetadata readMetadata(BinaryReader* reader, int version, int cyanvasVersion)
	{
		ScoreMetadata metadata;
		metadata.title = reader->readString();
		metadata.author = reader->readString();
		metadata.artist = reader->readString();
		metadata.musicFile = reader->readString();
		metadata.musicOffset = reader->readSingle();

		if (version > 1)
			metadata.jacketFile = reader->readString();

		if (cyanvasVersion >= 1)
			metadata.laneExtension = reader->readUInt32();

		return metadata;
	}

	void writeMetadata(const ScoreMetadata& metadata, BinaryWriter* writer)
	{
		writer->writeString(metadata.title);
		writer->writeString(metadata.author);
		writer->writeString(metadata.artist);
		writer->writeString(metadata.musicFile);
		writer->writeSingle(metadata.musicOffset);
		writer->writeString(metadata.jacketFile);
		writer->writeInt32(metadata.laneExtension);
	}

	void readScoreEvents(Score& score, int version, int cyanvasVersion, BinaryReader* reader)
	{
		// time signature
		int timeSignatureCount = reader->readUInt32();
		if (timeSignatureCount)
			score.timeSignatures.clear();

		for (int i = 0; i < timeSignatureCount; ++i)
		{
			int measure = reader->readUInt32();
			int numerator = reader->readUInt32();
			int denominator = reader->readUInt32();
			score.timeSignatures[measure] = { measure, numerator, denominator };
		}

		// bpm
		int tempoCount = reader->readUInt32();
		if (tempoCount)
			score.tempoChanges.clear();

		for (int i = 0; i < tempoCount; ++i)
		{
			int tick = reader->readUInt32();
			float bpm = reader->readSingle();
			score.tempoChanges.push_back({ tick, bpm });
		}

		// hi-speed
		if (version > 2)
		{
			int hiSpeedCount = reader->readUInt32();
			for (int i = 0; i < hiSpeedCount; ++i)
			{
				int tick = reader->readUInt32();
				float speed = reader->readSingle();
				int layer = 0;
				if (cyanvasVersion >= 4)
					layer = reader->readUInt32();
				id_t id = getNextHiSpeedID();
				score.hiSpeedChanges[id] = HiSpeedChange{ id, tick, speed, layer };
			}
		}

		// skills and fever
		if (version > 1)
		{
			int skillCount = reader->readUInt32();
			for (int i = 0; i < skillCount; ++i)
			{
				int tick = reader->readUInt32();
				id_t id = getNextSkillID();
				score.skills.emplace(id, SkillTrigger{ id, tick });
			}

			score.fever.startTick = reader->readUInt32();
			score.fever.endTick = reader->readUInt32();
		}
	}

	void writeScoreEvents(const Score& score, BinaryWriter* writer)
	{
		writer->writeInt32(score.timeSignatures.size());
		for (const auto& [_, timeSignature] : score.timeSignatures)
		{
			writer->writeInt32(timeSignature.measure);
			writer->writeInt32(timeSignature.numerator);
			writer->writeInt32(timeSignature.denominator);
		}

		writer->writeInt32(score.tempoChanges.size());
		for (const auto& tempo : score.tempoChanges)
		{
			writer->writeInt32(tempo.tick);
			writer->writeSingle(tempo.bpm);
		}

		writer->writeInt32(score.hiSpeedChanges.size());
		for (const auto& [_, hiSpeed] : score.hiSpeedChanges)
		{
			writer->writeInt32(hiSpeed.tick);
			writer->writeSingle(hiSpeed.speed);
			writer->writeInt32(hiSpeed.layer);
		}

		writer->writeInt32(score.skills.size());
		for (const auto& [_, skill] : score.skills)
		{
			writer->writeInt32(skill.tick);
		}

		writer->writeInt32(score.fever.startTick);
		writer->writeInt32(score.fever.endTick);
	}

	Score deserializeScore(const std::string& filename)
	{
		Score score;
		BinaryReader reader(filename);
		if (!reader.isStreamValid())
			return score;

		std::string signature = reader.readString();
		if (signature != "MMWS" && signature != "CCMMWS")
			throw std::runtime_error("Not a MMWS file.");

		bool isCyanvas = signature == "CCMMWS";

		int version = reader.readUInt16();
		int cyanvasVersion = reader.readUInt16();
		if (isCyanvas && cyanvasVersion == 0)
		{
			cyanvasVersion = 1;
		}

		uint32_t metadataAddress{};
		uint32_t eventsAddress{};
		uint32_t tapsAddress{};
		uint32_t holdsAddress{};
		uint32_t damagesAddress{};
		uint32_t layersAddress{};
		uint32_t waypointsAddress{};
		uint32_t genericNotesAddress{}; // New for V7+

		if (version > 2) // version refers to the base MMWS format version, not cyanvasVersion
		{
			if (cyanvasVersion >= MMW_NEW_CYANVAS_VERSION)
			{
				metadataAddress = reader.readUInt32();
				eventsAddress = reader.readUInt32();
				holdsAddress = reader.readUInt32();
				layersAddress = reader.readUInt32();
				waypointsAddress = reader.readUInt32();
				genericNotesAddress = reader.readUInt32();
			}
			else // Older cyanvas versions or base MMWS
			{
				metadataAddress = reader.readUInt32();
				eventsAddress = reader.readUInt32();
				tapsAddress = reader.readUInt32();
				holdsAddress = reader.readUInt32();
				if (isCyanvas) // damagesAddress was only in CCMMWS before V7
					damagesAddress = reader.readUInt32();
				if (cyanvasVersion >= 4)
					layersAddress = reader.readUInt32();
				if (cyanvasVersion >= 5)
					waypointsAddress = reader.readUInt32();
			}
			reader.seek(metadataAddress);
		}

		score.metadata = readMetadata(&reader, version, cyanvasVersion);

		if (version > 2) reader.seek(eventsAddress);
		readScoreEvents(score, version, cyanvasVersion, &reader);

		if (cyanvasVersion >= MMW_NEW_CYANVAS_VERSION)
		{
			// Read generic notes (all non-hold notes)
			if (genericNotesAddress > 0) reader.seek(genericNotesAddress);
			int genericNoteCount = reader.readUInt32();
			score.notes.reserve(genericNoteCount); // Reserve space for all notes (generic + hold parts)
			for (int i = 0; i < genericNoteCount; ++i)
			{
				// readNote now reads type from stream
				Note note = readNote(&reader, cyanvasVersion); 
				note.ID = Note::getNextID();
				score.notes[note.ID] = note;
			}

			// Read holds (hold note parts are also in score.notes, but their structure is defined here)
			if (holdsAddress > 0) reader.seek(holdsAddress);
			int holdCount = reader.readUInt32();
			score.holdNotes.reserve(holdCount);
			for (int i = 0; i < holdCount; ++i)
			{
				HoldNote hold;
				unsigned int flags = reader.readUInt32(); // Assuming flags are always present for holds in new version

				if (flags & HOLD_START_HIDDEN) hold.startType = HoldNoteType::Hidden;
				if (flags & HOLD_END_HIDDEN) hold.endType = HoldNoteType::Hidden;
				if (flags & HOLD_GUIDE) hold.startType = hold.endType = HoldNoteType::Guide;

				// Read start note for the hold
				Note start = readNote(&reader, cyanvasVersion); // Reads type from stream
				start.ID = Note::getNextID();
				hold.start.ease = (EaseType)reader.readUInt32();
				hold.start.ID = start.ID;
				hold.fadeType = (FadeType)reader.readUInt32();
				hold.guideColor = (GuideColor)reader.readUInt32();
				score.notes[start.ID] = start;

				int stepCount = reader.readUInt32();
				hold.steps.reserve(stepCount);
				for (int k = 0; k < stepCount; ++k)
				{
					Note mid = readNote(&reader, cyanvasVersion); // Reads type from stream
					mid.ID = Note::getNextID();
					mid.parentID = start.ID;
					score.notes[mid.ID] = mid;

					HoldStep step{};
					step.type = (HoldStepType)reader.readUInt32();
					step.ease = (EaseType)reader.readUInt32();
					step.ID = mid.ID;
					hold.steps.push_back(step);
				}

				Note end = readNote(&reader, cyanvasVersion); // Reads type from stream
				end.ID = Note::getNextID();
				end.parentID = start.ID;
				score.notes[end.ID] = end;

				hold.end = end.ID;
				score.holdNotes[start.ID] = hold;
			}
		}
		else // Logic for older versions (cyanvasVersion < MMW_NEW_CYANVAS_VERSION)
		{
			// Read taps (old NoteType::Tap, now Slide)
			if (tapsAddress > 0) reader.seek(tapsAddress);
			int tapNoteCount = reader.readUInt32();
			score.notes.reserve(tapNoteCount); // Initial reserve
			for (int i = 0; i < tapNoteCount; ++i)
			{
				Note note = readNote(NoteType::Tap, &reader, cyanvasVersion); // Pass old type
				note.ID = Note::getNextID();
				// Convert Tap to Slide and ensure flick
				note.type = NoteType::Slide; 
				if (note.flick == FlickType::None) {
					note.flick = FlickType::Default;
				}
				score.notes[note.ID] = note;
			}

			// Read holds (older format)
			if (holdsAddress > 0) reader.seek(holdsAddress);
			int holdCount = reader.readUInt32();
			score.holdNotes.reserve(holdCount);
			for (int i = 0; i < holdCount; ++i)
			{
				HoldNote hold;
				unsigned int flags{};
				if (version > 3) flags = reader.readUInt32();

				if (flags & HOLD_START_HIDDEN) hold.startType = HoldNoteType::Hidden;
				if (flags & HOLD_END_HIDDEN) hold.endType = HoldNoteType::Hidden;
				if (flags & HOLD_GUIDE) hold.startType = hold.endType = HoldNoteType::Guide;

				Note start = readNote(NoteType::Hold, &reader, cyanvasVersion);
				start.ID = Note::getNextID();
				hold.start.ease = (EaseType)reader.readUInt32();
				hold.start.ID = start.ID;
				if (cyanvasVersion >= 2) hold.fadeType = (FadeType)reader.readUInt32();
				if (cyanvasVersion >= 3) hold.guideColor = (GuideColor)reader.readUInt32();
				else hold.guideColor = start.critical ? GuideColor::Yellow : GuideColor::Green;
				score.notes[start.ID] = start;

				int stepCount = reader.readUInt32();
				hold.steps.reserve(stepCount);
				for (int k = 0; k < stepCount; ++k)
				{
					Note mid = readNote(NoteType::HoldMid, &reader, cyanvasVersion);
					mid.ID = Note::getNextID();
					mid.parentID = start.ID;
					score.notes[mid.ID] = mid;

					HoldStep step{};
					step.type = (HoldStepType)reader.readUInt32();
					step.ease = (EaseType)reader.readUInt32();
					step.ID = mid.ID;
					hold.steps.push_back(step);
				}

				Note end = readNote(NoteType::HoldEnd, &reader, cyanvasVersion);
				end.ID = Note::getNextID();
				end.parentID = start.ID;
				score.notes[end.ID] = end;
				hold.end = end.ID;
				score.holdNotes[start.ID] = hold;
			}

			// Read damages (old NoteType::Damage)
			if (isCyanvas && damagesAddress > 0 && cyanvasVersion >= 1) // Check damagesAddress > 0
			{
				reader.seek(damagesAddress);
				int damageCount = reader.readUInt32();
				// score.notes.reserve(score.notes.size() + damageCount); // Already reserved or can grow
				for (int i = 0; i < damageCount; ++i)
				{
					// Read and discard old damage note data
					// Use NoteType::Slide (formerly Tap) as a placeholder for the byte structure
					readNote(NoteType::Slide, &reader, cyanvasVersion); 
					// Do not assign ID or add to score.notes
				}
			}
		}

		// Common: Layers and Waypoints (check their respective addresses and versions)
		if (layersAddress > 0 && cyanvasVersion >= 4) // layersAddress could be 0 if file is very old
		{
			score.layers.clear();
			reader.seek(layersAddress);
			int layerCount = reader.readUInt32();
			score.layers.reserve(layerCount);
			for (int i = 0; i < layerCount; ++i)
			{
				std::string name = reader.readString();
				score.layers.push_back({ name });
			}
		}

		if (waypointsAddress > 0 && cyanvasVersion >= 5) // waypointsAddress could be 0
		{
			score.waypoints.clear();
			reader.seek(waypointsAddress);
			int waypointCount = reader.readUInt32();
			score.waypoints.reserve(waypointCount);
			for (int i = 0; i < waypointCount; ++i)
			{
				std::string name = reader.readString();
				int tick = reader.readUInt32();
				score.waypoints.push_back({ name, tick });
			}
		}

		reader.close();
		return score;
	}

	void serializeScore(const Score& score, const std::string& filename)
	{
		BinaryWriter writer(filename);
		if (!writer.isStreamValid())
			return;

		// signature
		writer.writeString("CCMMWS");

		// version
		writer.writeInt16(4);
		// cyanvas version
		writer.writeInt16(MMW_NEW_CYANVAS_VERSION);

		// offsets address in order: metadata -> events -> holds -> layers -> waypoints -> generic_notes
		// Old: taps, damages are removed for MMW_NEW_CYANVAS_VERSION
		uint32_t offsetsAddress = writer.getStreamPosition();
		writer.writeNull(sizeof(uint32_t) * 6); // 6 addresses for new format

		uint32_t metadataAddress = writer.getStreamPosition();
		writeMetadata(score.metadata, &writer);

		uint32_t eventsAddress = writer.getStreamPosition();
		writeScoreEvents(score, &writer);

		// Collect all note IDs that are part of hold notes
		std::unordered_set<id_t> holdNoteConstituentIDs;
		for (const auto& pair : score.holdNotes) {
			const HoldNote& hold = pair.second;
			holdNoteConstituentIDs.insert(hold.start.ID);
			holdNoteConstituentIDs.insert(hold.end);
			for (const auto& step : hold.steps) {
				holdNoteConstituentIDs.insert(step.ID);
			}
		}
		
		uint32_t holdsAddress = writer.getStreamPosition();
		writer.writeInt32(score.holdNotes.size());
		for (const auto& [id, hold] : score.holdNotes)
		{
			unsigned int flags{};
			if (hold.startType == HoldNoteType::Guide)
				flags |= HOLD_GUIDE;
			if (hold.startType == HoldNoteType::Hidden)
				flags |= HOLD_START_HIDDEN;
			if (hold.endType == HoldNoteType::Hidden)
				flags |= HOLD_END_HIDDEN;
			writer.writeInt32(flags);

			// note data
			const Note& start = score.notes.at(hold.start.ID);
			writeNote(start, &writer, MMW_NEW_CYANVAS_VERSION); // Use new writeNote
			writer.writeInt32((int)hold.start.ease);
			writer.writeInt32((int)hold.fadeType);
			writer.writeInt32((int)hold.guideColor);

			// steps
			int stepCount = hold.steps.size();
			writer.writeInt32(stepCount);
			for (const auto& step : hold.steps)
			{
				const Note& mid = score.notes.at(step.ID);
				writeNote(mid, &writer, MMW_NEW_CYANVAS_VERSION); // Use new writeNote
				writer.writeInt32((int)step.type);
				writer.writeInt32((int)step.ease);
			}

			// end
			const Note& end = score.notes.at(hold.end);
			writeNote(end, &writer, MMW_NEW_CYANVAS_VERSION); // Use new writeNote
		}
		
		uint32_t layersAddress = writer.getStreamPosition();
		writer.writeInt32(score.layers.size());
		for (const auto& layer : score.layers)
		{
			writer.writeString(layer.name);
		}

		uint32_t waypointsAddress = writer.getStreamPosition();
		writer.writeInt32(score.waypoints.size());
		for (const auto& waypoint : score.waypoints)
		{
			writer.writeString(waypoint.name);
			writer.writeInt32(waypoint.tick);
		}

		uint32_t genericNotesAddress = writer.getStreamPosition();
		// Write count for generic notes first, then the notes
		int genericNoteCount = 0;
		// First pass to count generic notes
		for (const auto& [id, note] : score.notes)
		{
			if (holdNoteConstituentIDs.find(id) == holdNoteConstituentIDs.end())
			{
				genericNoteCount++;
			}
		}
		writer.writeInt32(genericNoteCount);

		// Second pass to write generic notes
		for (const auto& [id, note] : score.notes)
		{
			if (holdNoteConstituentIDs.find(id) == holdNoteConstituentIDs.end())
			{
				writeNote(note, &writer, MMW_NEW_CYANVAS_VERSION); // Use new writeNote
			}
		}
		
		// write offset addresses
		writer.seek(offsetsAddress);
		writer.writeInt32(metadataAddress);
		writer.writeInt32(eventsAddress);
		// writer.writeInt32(tapsAddress); // Removed
		writer.writeInt32(holdsAddress);
		// writer.writeInt32(damagesAddress); // Removed
		writer.writeInt32(layersAddress);
		writer.writeInt32(waypointsAddress);
		writer.writeInt32(genericNotesAddress); // Added

		writer.flush();
		writer.close();
	}
}
