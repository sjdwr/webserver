#include "unit.h"

VOID 
SizeToUnit(
	LPSTR dst, 
	size_t dstlen,
	DWORD High, 
	DWORD Low
)
{
	BOOLEAN				_else = TRUE;
	unsigned long long	qword, seg;

	LPCSTR Unit[] = { "KB", "MB", "GB", "TB", "PB", "EB" };

	qword = High << 28;
	qword |= Low;
	seg = 0x1000000000000000;

	for (int i = 5; i >= 0; --i, seg /= 1024)
	{
		if (qword / seg > 0)
		{
			sprintf_s(dst, dstlen, "%.2f %s", (double)qword / seg, Unit[i]);
			_else = FALSE;
			return;
		}
	}

	if (_else)
		sprintf_s(dst, dstlen, "%d   B", Low);
}

VOID
MakeSession(
	LPSTR Session,
	DWORD RandomSize
)
{
	LPCSTR randomchars = "abcdefghijklmnopqrstuvwxyz0123456789";

	srand((UINT)GetTickCount());

	for (UINT Index = 0; Index < RandomSize; ++Index)
		Session[Index] = randomchars[rand() % 36];
}

DWORD
StrCount(
	LPCSTR Str,
	LPCSTR Substr
)
{
	DWORD len;
	DWORD count = 0;
	LPSTR sch = (LPSTR)Str;


	if (NULL == Str ||
		NULL == Substr ||
		0 == (len = strlen(Substr)) ||
		0 == strlen(Str))
		return 0;

	while (sch = strstr(sch, Substr))
	{
		++count;
		sch += len;
	}

	return count;
}

DWORD
Replace(
	LPSTR &Dst,
	LPSTR Src, 
	LPCSTR find, 
	LPCSTR replace
) 
{
	LPSTR sch;
	DWORD count = 0;
	DWORD len = 0;
	DWORD findLen = strlen(find); 
	DWORD repLen = strlen(replace);
	UINT Index = 0;

	Dst = Src;

	if (0 == findLen ||
		0 == repLen)
		return FALSE;

	Index = strlen(Src);

	if (findLen != repLen)
	{
		sch = Src;

		while (sch = strstr(sch, find))
		{
			++count;
			sch += findLen;
		}

		if (0 == count)
			return FALSE;
	}

	len = Index + count * (repLen - findLen);

	if (NULL == (Dst = (LPSTR)malloc(len + 1)))
		return FALSE;

	Index = 0;
	sch = Dst;

	while (*Src)
	{
		if (0 == memcmp(Src, find, findLen))
		{
			memcpy(sch, replace, repLen);
			sch += repLen;
			Src += findLen;
		}
		else
		{
			*sch++ = *Src++;
		}
	}

	*sch = NULL;

	return len;
}

DWORD 
CovTag(
	LPSTR &Dst
)
{
	LPSTR Message = Dst;
	LPSTR CovMsg;

	DWORD Len = strlen(Dst);
	DWORD res;

	if (res = Replace(CovMsg, Message, "<", "&lt;"))
	{
		free(Message);
		Len = res;
	}

	if (res = Replace(Message, CovMsg, ">", "&gt;"))
	{
		free(CovMsg);
		Len = res;
	}

	Dst = Message;
	return Len;
}

VOID
MakeRandomString(
	LPSTR Session,
	DWORD RandomSize
)
{
	LPCSTR randomchars = "abcdefghijklmnopqrstuvwxyz0123456789";
	UINT Index = 0;

	srand((UINT)GetTickCount());

	for (; Index < RandomSize; ++Index)
		Session[Index] = randomchars[rand() % 36];

	Session[Index] = NULL;
}

INT CALLBACK 
BrowseCallbackProc(
	HWND hwnd, 
	UINT uMsg, 
	LPARAM lParam,
	LPARAM lpData
)
{
	if (uMsg == BFFM_INITIALIZED)
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpData);

	return 0;
}

BOOLEAN 
BrowserFolder(
	HWND		hWndOwner,
	LPCSTR		DialogTitle,
	LPSTR		ResultPath,
	size_t		PathLen
)
{
	LPITEMIDLIST	pidlBrowse;
	BROWSEINFO		brInfo = { 0 };

	brInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT | BIF_VALIDATE;
	brInfo.hwndOwner = hWndOwner;
	brInfo.pszDisplayName = ResultPath;
	brInfo.lpszTitle = DialogTitle;
	brInfo.lpfn = BrowseCallbackProc;
	brInfo.lParam = (LPARAM)ResultPath;

	memset(ResultPath, 0, PathLen);

	if (pidlBrowse = SHBrowseForFolder(&brInfo))
	{
		SHGetPathFromIDList(pidlBrowse, ResultPath);
		return TRUE;
	}

	return FALSE;
}

VOID
RemoveSpecialChar(
	LPSTR szName,
	size_t len
)
{
	LPCSTR SpecialChar = "\\/:*?\"<>|";
	UINT c = 0;

	for (UINT i = 0; i < len; ++i)
		for (UINT j = 0; j < 9; ++j)
			if (szName[i] == SpecialChar[j])
			{
				j = 0;
				++c;
				for (UINT k = i; k < len - 1; ++k)
					szName[k] = szName[k + 1];
			}

	szName[len - c] = NULL;
}