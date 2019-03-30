#include "Midi.h"

MidiFile* MidiFile::OpenFile(LPCSTR filename) {
	HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;
	DWORD dwRead;
	BYTE buf[14];
	if (!ReadFile(hFile, buf, 14, &dwRead, NULL)
		|| dwRead != 14
		|| *(PUINT32)buf != 0x6468544d)
	{
		CloseHandle(hFile);
		return NULL;
	}
	buf[9] = buf[11];
	UINT16 ntrk = *(PUINT16)(buf + 9);
	buf[11] = buf[13];
	UINT16 divs = *(PUINT16)(buf + 11), i;
	LARGE_INTEGER liSize;
	if (!GetFileSizeEx(hFile, &liSize)) {
		CloseHandle(hFile);
		return NULL;
	}
	PBYTE data = new BYTE[liSize.QuadPart - 14 - 8 * (UINT32)ntrk], read_ptr = data;
	MidiTrk *trks = new MidiTrk[ntrk];
	for (i = 0; i < ntrk; i++) {
		if (!ReadFile(hFile, buf, 8, &dwRead, NULL)
			|| dwRead != 8
			|| *(PUINT32)buf != 0x6b72544d)
		{
			delete trks;
			delete data;
			CloseHandle(hFile);
			return NULL;
		}
		buf[1] = buf[7];
		buf[2] = buf[6];
		buf[3] = buf[5];
		UINT32 length = *(PUINT32)(buf + 1);
		if (!ReadFile(hFile, read_ptr, length, &dwRead, NULL) || dwRead != length) {
			delete trks;
			delete data;
			CloseHandle(hFile);
			return NULL;
		}
		trks[i].beg = read_ptr;
		trks[i].ptr = read_ptr;
		trks[i].end = read_ptr + length;
		trks[i].not_end = (length > 0);
		read_ptr += length;
	}
	CloseHandle(hFile);
	MidiFile *mf = new MidiFile;
	mf->ntrk = ntrk;
	mf->divs = divs;
	mf->data = data;
	mf->trks = trks;
	mf->not_end = 1;
	mf->NotAllEnd();
	return mf;
}
MidiFile::~MidiFile() {
	if (data)
		delete[] data;
	delete[] trks;
}
MidiFile* MidiFile::Copy() {
	MidiFile* mf = new MidiFile;
	mf->ntrk = ntrk;
	mf->divs = divs;
	mf->data = NULL;
	mf->not_end = not_end;
	mf->trks = new MidiTrk[ntrk];
	for (UINT16 i = 0; i < ntrk; i++)
		mf->trks[i] = trks[i];
	return mf;
}
BYTE MidiFile::NotAllEnd() {
	if (not_end) {
		for (UINT16 i = 0; i < ntrk; i++)
			if (trks[i].not_end)
				return not_end = 1;
		return not_end = 0;
	}
	return 0;
}
void MidiFile::ResetAllPtr() {
	for (UINT i = 0; i < ntrk; i++)
		trks[i].ResetPtr();
	not_end = 1;
	NotAllEnd();
}