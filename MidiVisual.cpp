#include <cstdio>
#include "MidiVisual.h"

void KeyNotes::Init() {
	head.k_next = NULL;
	tail = &head;
}
void KeyNotes::Append(MvisNote* note) {
	tail->k_next = note;
	note->k_prev = tail;
	note->k_next = NULL;
	tail = note;
}
void KeyNotes::Remove(MvisNote* note) {
	if (note->k_prev->k_next = note->k_next)
		note->k_next->k_prev = note->k_prev;
	else
		tail = note->k_prev;
}
void TrkNotes::Init() {
	head.t_next = NULL;
	tail = mid1 = mid2 = &head;
	upOnCnt = dnOnCnt = 0;
}
void TrkNotes::Append(MvisNote* note) {
	tail->t_next = note;
	note->t_next = NULL;
	tail = note;
	upOnCnt++;
}
MvisNote* TrkNotes::IncMid1() {
	if (upOnCnt == 0)
		return NULL;
	upOnCnt--;
	return mid1 = mid1->t_next;
}
MvisNote* TrkNotes::IncMid2() {
	dnOnCnt++;
	return mid2 = mid2->t_next;
}
MvisNote* TrkNotes::RemFirst() {
	if (dnOnCnt == 0)
		return NULL;
	dnOnCnt--;
	MvisNote *note = head.t_next;
	head.t_next = note->t_next;
	if (note == tail) {
		tail = mid1 = mid2 = &head;
		return note;
	}
	if (note == mid1)
		mid1 = &head;
	if (note == mid2)
		mid2 = &head;
	return note;
}
MidiVisual::MidiVisual(UINT16 ntrk, UINT64 vn_prealloc) {
	vn_head.v_next = NULL;
	vn_tail = &vn_head;
	tn = new TckNotes[ntrk];
	vna_id = 0;
	if (vna_cnt = vn_prealloc) {
		vn_alloc = new MvisNote[vna_cnt];
		vna_ids = new UINT64[vna_cnt];
		for (UINT64 i = 0; i < vna_cnt; i++)
			vna_ids[i] = i;
	}
	for (BYTE key = 0; key < 128; key++) {
		kn[key].Init();
		for (BYTE ch = 0; ch < 16; ch++)
			for (UINT16 trk = 0; trk < ntrk; trk++)
				tn[trk][ch][key].Init();
	}
}
MidiVisual::~MidiVisual() {
	delete[] tn;
	if (vna_cnt)
		delete[] vn_alloc;
	else {
		MvisNote *curr = vn_head.v_next, *next;
		while (curr) {
			next = curr->v_next;
			delete curr;
			curr = next;
		}
	}
}
void MidiVisual::Up_NoteON(UINT16 trk, BYTE ch, BYTE key, UINT32 tick) {
	MvisNote *note;
	if (vna_cnt) {
		if (vna_id == vna_cnt)
			note = NULL; // fuck it
		else
			note = vn_alloc + vna_ids[vna_id++];
	}
	else {
		vna_id++;
		note = new MvisNote;
	}
	note->trk = trk;
	note->ch = ch;
	note->key = key;
	note->beg = tick;
	note->end = -1;
	vn_tail->v_next = note;
	note->v_prev = vn_tail;
	note->v_next = NULL;
	vn_tail = note;
	tn[trk][ch][key].Append(note);
}
void MidiVisual::Up_NoteOFF(UINT16 trk, BYTE ch, BYTE key, UINT32 tick) {
	MvisNote *note = tn[trk][ch][key].IncMid1();
	if(note)
		note->end = tick;
}
void MidiVisual::Dn_NoteON(UINT16 trk, BYTE ch, BYTE key) {
	MvisNote *note = tn[trk][ch][key].IncMid2();
	kn[key].Append(note);
}
void MidiVisual::Dn_NoteOFF(UINT16 trk, BYTE ch, BYTE key) {
	MvisNote *note = tn[trk][ch][key].RemFirst();
	if (note) {
		kn[key].Remove(note);
		if (note->v_prev->v_next = note->v_next)
			note->v_next->v_prev = note->v_prev;
		else
			vn_tail = note->v_prev;
		if (vna_cnt)
			vna_ids[--vna_id] = note - vn_alloc;
		else {
			delete note;
			vna_id--;
		}
	}
}