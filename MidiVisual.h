#pragma once
#define NULL 0
typedef unsigned char  BYTE;
typedef unsigned short UINT16;
typedef unsigned int   UINT32;
typedef unsigned long long UINT64;

class MvisNote {
public:
	MvisNote
		*t_next,
		*k_prev, *k_next,
		*v_prev, *v_next;
	UINT16 trk;
	BYTE ch, key;
	UINT32 beg, end;
};
class KeyNotes {
public:
	MvisNote head, *tail;
	void Init();
	void Append(MvisNote*);
	void Remove(MvisNote*);
};
class TrkNotes {
public:
	MvisNote head, *tail, *mid1, *mid2;
	UINT32 upOnCnt, dnOnCnt;
	void Init();
	void Append(MvisNote*);
	MvisNote* IncMid1();
	MvisNote* IncMid2();
	MvisNote* RemFirst();
};
using TckNotes = TrkNotes[16][128];

class MidiVisual {
public:
	MvisNote vn_head, *vn_tail, *vn_alloc;
	UINT64 vna_id, vna_cnt, *vna_ids;
	KeyNotes kn[128];
	TckNotes *tn;
	MidiVisual(UINT16, UINT64);
	~MidiVisual();
	void Up_NoteON(UINT16, BYTE, BYTE, UINT32);
	void Up_NoteOFF(UINT16, BYTE, BYTE, UINT32);
	void Dn_NoteON(UINT16, BYTE, BYTE);
	void Dn_NoteOFF(UINT16, BYTE, BYTE);
};