#pragma once
#include <windows.h>

#define BUILD_PLAYER

constexpr UINT32 MaxBars = 2 << 20;
constexpr UINT32 VidWidth = 640, VidHeight = 360, VidBarsHeight = 320;
constexpr UINT32 Framerate = 60;
constexpr FLOAT ScrHeight = 1.0;
#define VID_FILENAME "D:\\Ywt5dxu\\sftdp17.vid"
#define MID_FILENAME "\\sftdp17.mid"

typedef union {
	BYTE type;
	struct {
		BYTE x;
		union {
			UINT32 msg;
			struct { BYTE cmd, a1, a2; };
		};
	} cmd;
	struct {
		BYTE x;
		UINT32 len;
		BYTE *data, type;
	} meta;
} MidiEvent;

class MidiTrk {
public:
	BYTE *ptr, *beg, *end, not_end, rs;
	UINT32 dt;
	void ResetPtr();
	UINT32 ReadVL();
	UINT32 ReadDelta();
	void ReadEvent(MidiEvent &e);
	BYTE ReadEvent_Up(BYTE& ch, BYTE& key);
	BYTE ReadEvent_Dn(BYTE& ch, BYTE& key, UINT32& msg, UINT32& mpqn);
};

class MidiFile {
public:
	BYTE *data, not_end;
	UINT16 ntrk, divs;
	MidiTrk *trks;
	static MidiFile* OpenFile(LPCSTR filename);
	~MidiFile();
	MidiFile* Copy();
	BYTE NotAllEnd();
	void ResetAllPtr();
};