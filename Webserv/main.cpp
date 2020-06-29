#include "main.h"

HINSTANCE		hInst; 
BOOLEAN			WebServerStarted = FALSE;
HWND			___MyDialoghWnd;
CHAR			DOWNLOAD_PATH[MAX_PATH];
CHAR			UPLOAD_PATH[MAX_PATH];
CHAR			SERVER_IP[15];
BYTE			UpAndChatActive = 0;

INT_PTR CALLBACK
DlgProc(
	HWND		hDlg,
	UINT		message,
	WPARAM		wParam,
	LPARAM		lParam
);

int APIENTRY 
WinMain(
	HINSTANCE	hInstance,
	HINSTANCE	hPrevInstance,
    LPSTR		lpCmdLine,
    int			nCmdShow
)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    return FALSE;
}

INT_PTR CALLBACK 
DlgProc(
	HWND		hDlg, 
	UINT		message, 
	WPARAM		wParam, 
	LPARAM		lParam
)
{
    switch (message)
    {
		case WM_INITDIALOG:
		{
			___MyDialoghWnd = hDlg;

			HWND hChildWnd = FindWindowEx(GetDlgItem(hDlg, IDC_SERVERIP), NULL, "Edit", NULL);

			if (hChildWnd)
				SendMessage(hChildWnd, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);

			EnableWindow(GetDlgItem(hDlg, IDC_UPLOADPATH), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_BROWSEUPPATH), FALSE);

			SendDlgItemMessage(hDlg, IDC_PORT, WM_SETTEXT, (WPARAM)0x02, (LPARAM)"80");
			SendDlgItemMessage(hDlg, IDC_UPLOADTIMEOUT, WM_SETTEXT, (WPARAM)0x04, (LPARAM)"6000");
			SendDlgItemMessage(hDlg, IDC_DOWNLOADTIMEOUT, WM_SETTEXT, (WPARAM)0x04, (LPARAM)"6000");

			GetHostIP(GetDlgItem(hDlg, IDC_SERVERIP));

			break;
		}
		case WM_COMMAND:
		{
			if (HIWORD(wParam) == BN_CLICKED)
			{
				switch (LOWORD(wParam))
				{
					case IDC_WEBSERVSTART:
					{
						LPCSTR	BtnName = "";
						WORD	ServerPort;
						DWORD	send_timeout;
						DWORD	recv_timeout;
						DWORD	checkPath;

						if (FALSE == WebServerStarted)
						{
							ServerPort = (WORD)GetDlgItemInt(hDlg, IDC_PORT, NULL, FALSE);
							send_timeout = (DWORD)GetDlgItemInt(hDlg, IDC_DOWNLOADTIMEOUT, NULL, FALSE);
							recv_timeout = (DWORD)GetDlgItemInt(hDlg, IDC_UPLOADTIMEOUT, NULL, FALSE);
							UpAndChatActive = (IsDlgButtonChecked(hDlg, IDC_ENALBEUPLOAD) == BST_CHECKED) |
								((IsDlgButtonChecked(hDlg, IDC_ENALBECHAT) == BST_CHECKED) << 1);

							SendDlgItemMessage(hDlg, IDC_UPLOADPATH, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)UPLOAD_PATH);
							SendDlgItemMessage(hDlg, IDC_DOWNLOADPATH, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)DOWNLOAD_PATH);
							SendDlgItemMessage(hDlg, IDC_SERVERIP, WM_GETTEXT, (WPARAM)sizeof(SERVER_IP), (LPARAM)SERVER_IP);

							if (NULL == *SERVER_IP)
							{
								MessageBox(hDlg, "주소를 선택해 주세요.", "알림", MB_ICONEXCLAMATION);
								return (INT_PTR)FALSE;
							}

							checkPath = GetFileAttributes(UPLOAD_PATH);

							if (UpAndChatActive & 0x01 && (
								INVALID_FILE_ATTRIBUTES == checkPath ||
								FALSE == (FILE_ATTRIBUTE_DIRECTORY & checkPath)))
							{
								MessageBox(hDlg, "유효한 업로드 경로가 아닙니다.", "알림", MB_ICONEXCLAMATION);
								return (INT_PTR)FALSE;
							}

							checkPath = GetFileAttributes(DOWNLOAD_PATH);

							if (INVALID_FILE_ATTRIBUTES == checkPath ||
								FALSE == (FILE_ATTRIBUTE_DIRECTORY & checkPath))
							{
								MessageBox(hDlg, "유효한 다운로드 경로가 아닙니다.", "알림", MB_ICONEXCLAMATION);
								return (INT_PTR)FALSE;
							}

							if (0 == ServerPort)
							{
								MessageBox(hDlg, "포트 주소의 범위는 1~65535 입니다.", "알림", MB_ICONEXCLAMATION);
								return (INT_PTR)FALSE;
							}

							if (FALSE == StartWebServer(ServerPort, send_timeout, recv_timeout, ProcessDataThread))
							{
								MessageBox(0, "Server 초기화 실패!", "Error", 16);
								PostQuitMessage(0);
							}

							BtnName = "중지";
						}
						else
						{
							StopWebServer();
							BtnName = "시작";
						}

						WebServerStarted = !WebServerStarted;

						SendDlgItemMessage(hDlg, IDC_WEBSERVSTART, WM_SETTEXT, (WPARAM)strlen(BtnName), (LPARAM)BtnName);

						for (DWORD Index = IDC_SERVERIP; Index <= IDC_PORT; ++Index)
							EnableWindow(GetDlgItem(hDlg, Index), !WebServerStarted);

						if (FALSE == WebServerStarted)
						{
							EnableWindow(GetDlgItem(hDlg, IDC_UPLOADPATH), UpAndChatActive & 0x01);
							EnableWindow(GetDlgItem(hDlg, IDC_BROWSEUPPATH), UpAndChatActive & 0x01);
						}

						break;
					}
					case IDC_BROWSEUPPATH:
					{
						if (BrowserFolder(hDlg, "폴더를 선택하세요.", UPLOAD_PATH, MAX_PATH))
							SendDlgItemMessage(hDlg, IDC_UPLOADPATH, WM_SETTEXT, (WPARAM)MAX_PATH, (LPARAM)UPLOAD_PATH);

						break;
					}
					case IDC_BROWSEDOWNPATH:
					{
						if (BrowserFolder(hDlg, "폴더를 선택하세요.", DOWNLOAD_PATH, MAX_PATH))
							SendDlgItemMessage(hDlg, IDC_DOWNLOADPATH, WM_SETTEXT, (WPARAM)MAX_PATH, (LPARAM)DOWNLOAD_PATH);

						break;
					}
					case IDC_ENALBEUPLOAD:
					{
						BOOLEAN bEnable;

						bEnable = BST_CHECKED == SendDlgItemMessage(hDlg, IDC_ENALBEUPLOAD, 
							BM_GETCHECK, (WPARAM)NULL, (LPARAM)NULL);

						EnableWindow(GetDlgItem(hDlg, IDC_UPLOADPATH), bEnable);
						EnableWindow(GetDlgItem(hDlg, IDC_BROWSEUPPATH), bEnable);

						break;
					}
				}
			}

			break;
		}
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			break;
		}
    }

    return 0;
}