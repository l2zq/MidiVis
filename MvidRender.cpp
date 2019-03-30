#include <cstdio>
#include "Midi.h"

#ifdef BUILD_RENDER

#include "MidiVisual.h"

UINT32 barColors[] = { 0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0x00FFFF, 0xFF00FF };
constexpr UINT32 nColors = (sizeof(barColors) / sizeof(UINT32));

int main(int argc, char ** argv) {
	HANDLE hFile = CreateFile(VID_FILENAME, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	MidiFile *mftop = MidiFile::OpenFile(MID_FILENAME);
	MidiFile *mfbot = mftop->Copy();
	MidiTrk *trktop = mftop->trks, *ttop;
	MidiTrk *trkbot = mfbot->trks, *tbot;
	MidiVisual *vis = new MidiVisual(mftop->ntrk, MaxBars);
	MvisNote *note;
	BYTE ch, key;
	UINT16 ntrk = mftop->ntrk, divs = mftop->divs, i;
	UINT32 scrh = (UINT32)(ScrHeight * divs),
		mindt, msg, scrt = 0, mpqn = 500000,
		left, rght, top, bot, ix, iy, color;
	DWORD dwWrite;
	constexpr UINT32
		VidNoteWidth = VidWidth / 128,
		VidPixelCount = VidHeight * VidWidth,
		VidMemLen = VidPixelCount * sizeof(UINT32);
	PUINT32
		vidMem = new UINT32[VidPixelCount];
	UINT64 tCurrent = 0, tNextEvt = 0, tNextTick = 0, tNextFrm = 0;
	UINT64 fMin = 0, fSec = 0, fCnt = 0;
	for (i = 0; i < ntrk; i++) {
		if (trktop[i].not_end) trktop[i].ReadDelta();
		if (trkbot[i].not_end) trkbot[i].dt = trkbot[i].ReadVL() + scrh;
	}
	while (mfbot->NotAllEnd()) {
		if (tCurrent == tNextEvt) {
			for (
				i = 0, mindt = -1, ttop = trktop, tbot = trkbot;
				i < ntrk;
				i++, ttop++, tbot++)
			{
				while (ttop->not_end&&ttop->dt == 0) {
					switch (ttop->ReadEvent_Up(ch, key)) {
					case 0x80: vis->Up_NoteOFF(i, ch, key, scrt); break;
					case 0x90: vis->Up_NoteON(i, ch, key, scrt); break;
					}
					if (ttop->not_end) ttop->ReadDelta();
				}
				while (tbot->not_end&&tbot->dt == 0) {
					switch (tbot->ReadEvent_Dn(ch, key, msg, mpqn)) {
					case 0x80: vis->Dn_NoteOFF(i, ch, key); break;
					case 0x90: vis->Dn_NoteON(i, ch, key); break;
					}
					if (tbot->not_end) tbot->ReadDelta();
				}
				if (ttop->not_end&&ttop->dt < mindt) mindt = ttop->dt;
				if (tbot->not_end&&tbot->dt < mindt) mindt = tbot->dt;
			}
			if (mindt != -1) {
				for (i = 0; i < ntrk; i++) {
					trktop[i].dt -= mindt;
					trkbot[i].dt -= mindt;
				}
				tNextEvt += (UINT64)mindt * mpqn / divs;
			}
		}
		if (tCurrent == tNextFrm) {
			constexpr UINT64 FrameInterval = 1000000 / Framerate;
			tNextFrm += FrameInterval;
			printf("\rTime: %02llu:%02llu.%02lluf Bars: %-7I64d", fMin, fSec, fCnt, vis->vna_id);
			fCnt++;
			if (fCnt >= Framerate) {
				fCnt = 0; fSec++;
				if (fSec >= 60) { fSec = 0; fMin++; }
			}
			memset(vidMem, 0xFF, VidMemLen);
			note = vis->vn_head.v_next;
			while (note) {
				color = barColors[note->trk % nColors];
				left = VidNoteWidth * note->key;
				rght = VidNoteWidth + left;
				if (note->end == -1) bot = 0;
				else bot = VidBarsHeight * (scrt - note->end) / scrh;
				if ((top = VidBarsHeight * (scrt - note->beg) / scrh) > VidBarsHeight)
					top = VidBarsHeight;
				top = VidHeight - top;
				bot = VidHeight - bot;
				for (ix = left; ix < rght; ix++) for (iy = top; iy < bot; iy++)
					vidMem[iy*VidWidth + ix] = color;
				note = note->v_next;
			}
			
			bot = VidHeight - VidBarsHeight;
			for (ix = 0; ix < VidWidth; ix++)
				vidMem[bot*VidWidth + ix] = 0x00;

			top = 0;
			bot = VidHeight - VidBarsHeight - 1;
			for (key = 0; key < 128; key++) {
				if (vis->kn[key].head.k_next) {
					color = barColors[vis->kn[key].tail->trk % nColors];
					left = VidNoteWidth * key;
					rght = VidNoteWidth + left;
					for (ix = left; ix < rght; ix++) for (iy = top; iy < bot; iy++)
						vidMem[iy*VidWidth + ix] = color;
				}
			}
			WriteFile(hFile, vidMem, VidMemLen, &dwWrite, NULL);
		}
		if (tCurrent == tNextTick) {
			tNextTick += mpqn / divs;
			scrt++;
		}
		tCurrent = tNextEvt;
		if (tCurrent > tNextTick)
			tCurrent = tNextTick;
		if (tCurrent > tNextFrm)
			tCurrent = tNextFrm;
	}
	printf("\nBars Left: %I64d\n", vis->vna_id);
	delete vidMem;
	delete vis;
	delete mfbot;
	delete mftop;
	CloseHandle(hFile);
	system("pause");
	return 0;
}

#endif