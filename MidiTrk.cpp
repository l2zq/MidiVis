#include "Midi.h"

void MidiTrk::ResetPtr() {
	ptr = beg;
	not_end = (ptr < end);
}
UINT32 MidiTrk::ReadVL() {
	BYTE b;
	UINT32 v = 0;
	do {
		b = *(ptr++);
		v = (v << 7) + (b & 0x7F);
	} while (b & 0x80);
	return v;
}
UINT32 MidiTrk::ReadDelta() {
	return dt = ReadVL();
}
void MidiTrk::ReadEvent(MidiEvent &e) {
	BYTE b = *(ptr++);
	if (b >= 0xF0) {
		if (b == 0xFF) {
			e.type = 1;
			e.meta.type = *(ptr++);
		}
		else
			e.type = 2;
		e.meta.len = ReadVL();
		e.meta.data = ptr;
		ptr += e.meta.len;
	}
	else {
		e.type = 0;
		e.cmd.msg = 0;
		if (b & 0x80) {
			rs = b;
			e.cmd.a1 = *(ptr++);
		}
		else
			e.cmd.a1 = b;
		e.cmd.cmd = rs;
		if ((rs & 0xE0) != 0xC0)
			e.cmd.a2 = *(ptr++);
	}
	not_end = (ptr < end);
}
BYTE MidiTrk::ReadEvent_Up(BYTE& ch, BYTE& a1) {
	BYTE b = *(ptr++);
	if (b >= 0xF0) {
		if (b == 0xFF)
			ptr++;
		ptr += ReadVL();
	}
	else {
		if (b & 0x80) {
			rs = b;
			a1 = *(ptr++);
		}
		else
			a1 = b;
		ch = rs & 0xF;
		if ((rs >> 5) != 0b110)
			b = *(ptr++);
		not_end = (ptr < end);
		return rs & 0xF0;
	}
	not_end = (ptr < end);
	return 0x00;
}
BYTE MidiTrk::ReadEvent_Dn(BYTE& ch, BYTE& a1, UINT32& msg, UINT32& mpqn) {
	BYTE b = *(ptr++);
	union {
		UINT32 ui32 = 0;
		BYTE bytes[4];
	};
	if (b >= 0xF0) {
		if (b == 0xFF && *(ptr++) == 0x51) {
			ptr++;
			bytes[2] = *(ptr++);
			bytes[1] = *(ptr++);
			bytes[0] = *(ptr++);
			mpqn = ui32;
			not_end = (ptr < end);
			return 0x51;
		}
		else
			ptr += ReadVL();
	}
	else {
		if (b & 0x80) {
			rs = b;
			bytes[1] = a1 = *(ptr++);
		}
		else
			bytes[1] = a1 = b;
		ch = (bytes[0] = rs) & 0xF;
		if ((rs >> 5) != 0b110)
			b = bytes[2] = *(ptr++);
		msg = ui32;
		not_end = (ptr < end);
		return rs & 0xF0;
	}
	not_end = (ptr < end);
	return 0x00;
}