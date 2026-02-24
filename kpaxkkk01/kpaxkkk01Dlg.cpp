#include "pch.h" // 무조건 1등으로 와야 합니다!
#include "framework.h"
#include <afxinet.h> // 인터넷 통신 헤더는 여기에!
#include "afxdialogex.h"
#include "kpaxkkk01.h"
#include "kpaxkkk01Dlg.h"
#include "afxdialogex.h"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <algorithm>
#include <vector>

// 서버 접속 정보 통합 관리
#ifdef _DEBUG
#define SERVER_IP    _T("127.0.0.1")  // 내가 테스트할 때는 로컬 접속
#define SERVER_PORT  2121
#else
#define SERVER_IP    _T("125.188.38.149") // 친구에게 줄 때는 공인 IP
#define SERVER_PORT  2121
#endif


// [추가] 커스텀 드로우 매크로 정의 (에러 해결용)
#ifndef CDDS_SUBITEMPREPAINT
#define CDDS_SUBITEMPREPAINT (CDDS_ITEMPREPAINT | 0x00020000)
#endif

// [에러 방지] 시스템 에러 메시지 변환 함수
CString GetErrorMessageKorean(DWORD dwErrorCode) {
    LPWSTR lpMsgBuf = NULL;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, dwErrorCode, MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT),
        (LPWSTR)&lpMsgBuf, 0, NULL);

    CString strError = lpMsgBuf;
    LocalFree(lpMsgBuf);
    strError.TrimRight(L"\r\n");
    if (dwErrorCode == 2) strError = L"파일을 찾을 수 없습니다.";
    if (dwErrorCode == 5) strError = L"액세스 거부 (권한 부족)";
    if (dwErrorCode == 32) strError = L"다른 프로그램이 사용 중입니다.";
    return strError;
}



// CAboutDlg 대화 상자 (기본 생성 코드)
class CAboutDlg : public CDialogEx {
public:
    CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif
protected:
    virtual void DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }
    DECLARE_MESSAGE_MAP()
public:
    //afx_msg void OnNMDblclkListDownload(NMHDR* pNMHDR, LRESULT* pResult);
};
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
    //ON_NOTIFY(NM_DBLCLK, IDC_LIST_DOWNLOAD, &CAboutDlg::OnNMDblclkListDownload)
END_MESSAGE_MAP()

// Ckpaxkkk01Dlg 대화 상자 구현
Ckpaxkkk01Dlg::Ckpaxkkk01Dlg(CWnd* pParent) : CDialogEx(IDD_KPAXKKK01_DIALOG, pParent) {
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_hSemaphore = NULL;
    m_nTotalFiles = 0;
    m_nDoneFiles = 0;
    m_bAllSelected = FALSE; // 처음엔 선택 안 된 상태
}

void Ckpaxkkk01Dlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_DOWNLOAD, m_ListCtrl);
}

BEGIN_MESSAGE_MAP(Ckpaxkkk01Dlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BTN_START, &Ckpaxkkk01Dlg::OnBnClickedBtnStart)
    ON_MESSAGE(WM_UPDATE_PROGRESS, &Ckpaxkkk01Dlg::OnUpdateProgress)
    ON_MESSAGE(WM_DOWNLOAD_COMPLETE, &Ckpaxkkk01Dlg::OnDownloadComplete)
    ON_BN_CLICKED(IDC_BTN_UPLOAD, &Ckpaxkkk01Dlg::OnBnClickedBtnUpload)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_DOWNLOAD, &Ckpaxkkk01Dlg::OnNMCustomdrawListDownload)
    ON_WM_DROPFILES()
    //ON_NOTIFY(NM_RCLICK, IDC_LIST_DOWNLOAD, &Ckpaxkkk01Dlg::OnNMRClickListDownload)
    ON_COMMAND(ID_MENU_DELETE, &Ckpaxkkk01Dlg::OnMenuDelete)
    ON_BN_CLICKED(IDC_BTN_CLEAR_FINISHED, &Ckpaxkkk01Dlg::OnBnClickedBtnClearFinished)
    ON_BN_CLICKED(IDC_CHK_AUTO_CLEAR, &Ckpaxkkk01Dlg::OnBnClickedChkAutoClear)
    ON_MESSAGE(WM_UPDATE_SPEED, &Ckpaxkkk01Dlg::OnUpdateSpeed)
    ON_BN_CLICKED(IDC_BTN_DOWNLOAD_SELECTED, &Ckpaxkkk01Dlg::OnBnClickedBtnDownloadSelected)
    ON_BN_CLICKED(IDC_BTN_SELECT_ALL, &Ckpaxkkk01Dlg::OnBnClickedBtnSelectAll)
    ON_BN_CLICKED(IDC_BTN_BROWSE, &Ckpaxkkk01Dlg::OnBnClickedBtnBrowse)
    ON_BN_CLICKED(IDC_BTN_DELETE_SELECTED, &Ckpaxkkk01Dlg::OnBnClickedBtnDeleteSelected)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_DOWNLOAD, &Ckpaxkkk01Dlg::OnNMDblclkListDownload)
END_MESSAGE_MAP()


// [추가] 실제 업로드 파일 복사를 수행하는 워커 스레드 함수
UINT UploadThreadProc(LPVOID pParam) {
    //AfxMessageBox(L"1");


    DownloadRequest* pReq = (DownloadRequest*)pParam;
    if (pReq == NULL) return 0;

    Ckpaxkkk01Dlg* pMain = (Ckpaxkkk01Dlg*)CWnd::FromHandle(pReq->hMainWnd);


    // [테스트 1] 이 메시지가 뜨는지 확인하세요.
    // 안 뜬다면 AfxBeginThread 호출 자체가 실패한 것입니다.
    // AfxMessageBox(L"스레드 생성 성공! 세마포어 대기 시작");

    // 세마포어 대기 (여기서 멈춰 있을 확률 99%)
    //WaitForSingleObject(pMain->m_hSemaphore, INFINITE);

    // [테스트 2] 이 메시지가 뜨는지 확인하세요.
    // 1번은 뜨는데 2번이 안 뜬다면, 이전 스레드들이 자리를 안 비워준 것입니다.
    // AfxMessageBox(L"세마포어 통과! 실제 전송 시작");

    //WaitForSingleObject(pMain->m_hSemaphore, INFINITE);
    //AfxMessageBox(L"2");

    // [중요] 초기화: 시작 시간을 현재 시간으로 세팅
    pReq->dwLastTick = GetTickCount();
    pReq->dwSpeedTick = GetTickCount();
    pReq->nLastBytes = 0;
    pReq->nLastPercent = -1;

    BOOL bResult = FALSE;
    int retryCount = 0;
    while (retryCount < 3) {
        bResult = CopyFileExW(
            pReq->source.c_str(), pReq->target.c_str(),
            [](LARGE_INTEGER total, LARGE_INTEGER trans, LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID data) -> DWORD {
                DownloadRequest* p = (DownloadRequest*)data;
                DWORD dwCurrentTick = GetTickCount();

                // 1. 전체 크기 저장
                if (p->nTotalBytes == 0) p->nTotalBytes = total.QuadPart;

                // 2. 속도 및 ETA 계산 (1초 주기)
                if (dwCurrentTick - p->dwSpeedTick >= 1000) {
                    double diff = (double)(trans.QuadPart - p->nLastBytes);
                    double mbps = diff / (1024.0 * 1024.0);

                    int eta = 0;
                    if (diff > 0) {
                        eta = (int)((total.QuadPart - trans.QuadPart) / diff);
                    }

                    // [핵심] 속도(*10)와 ETA를 합쳐서 UI로 전송
                    WPARAM wp = MAKEWPARAM((WORD)(mbps * 10), (WORD)eta);
                    ::PostMessage(p->hMainWnd, WM_UPDATE_SPEED, wp, (LPARAM)p->nItemIndex);

                    p->dwSpeedTick = dwCurrentTick;
                    p->nLastBytes = trans.QuadPart;
                    p->dwLastTick = dwCurrentTick; // 데이터가 움직였으므로 타임아웃 리셋
                }

                // 3. 타임아웃 체크 (10초간 멈춤 시)
                if (dwCurrentTick - p->dwLastTick > 10000) return PROGRESS_CANCEL;

                // 4. 진행률(게이지) 업데이트
                int percent = (int)((double)trans.QuadPart / (double)total.QuadPart * 100);
                if (percent > p->nLastPercent) {
                    p->nLastPercent = percent;
                    ::PostMessage(p->hMainWnd, WM_UPDATE_PROGRESS, (WPARAM)percent, (LPARAM)p->nItemIndex);
                }
                return PROGRESS_CONTINUE;
            },
            pReq, NULL, COPY_FILE_NO_BUFFERING
        );


        if (bResult) break;
        retryCount++;
        pReq->dwLastTick = GetTickCount();
        pReq->dwSpeedTick = GetTickCount();
        pReq->nLastBytes = 0;
        Sleep(2000);
    }

    ::PostMessage(pReq->hMainWnd, WM_DOWNLOAD_COMPLETE, (WPARAM)bResult, (LPARAM)pReq->nItemIndex);
    ReleaseSemaphore(pMain->m_hSemaphore, 1, NULL);
    delete pReq;
    return 0;
}

// [새로 추가] 서버로 파일을 쏘아올리는 FTP 업로드 엔진
UINT FtpUploadThreadProc(LPVOID pParam) {
    DownloadRequest* pReq = (DownloadRequest*)pParam;
    if (pReq == NULL) return 0;

    Ckpaxkkk01Dlg* pMain = (Ckpaxkkk01Dlg*)CWnd::FromHandle(pReq->hMainWnd);
    WaitForSingleObject(pMain->m_hSemaphore, INFINITE);

    pReq->dwLastTick = GetTickCount();
    pReq->dwSpeedTick = GetTickCount();
    pReq->nLastBytes = 0;
    pReq->nLastPercent = -1;

    BOOL bResult = FALSE;
    CInternetSession session(_T("KpaxWebHardClient"));
    CFtpConnection* pFtpConn = NULL;

    try {
        CString strIP = _T("125.188.38.149"); // 서버 공인 IP
        pFtpConn = session.GetFtpConnection(SERVER_IP, _T("friend"), _T("1111"), 2121, TRUE);
        pFtpConn->Command(_T("OPTS UTF8 ON"));

        // 로컬 파일 열기 (업로드할 파일)
        CFile localFile;
        if (localFile.Open(pReq->source.c_str(), CFile::modeRead | CFile::typeBinary)) {
            pReq->nTotalBytes = localFile.GetLength();

            // 서버에 생성될 파일명 (UTF-8 처리)
            CString strDestName = pReq->target.c_str(); // 파일명만 들어있어야 함
            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, strDestName, -1, NULL, 0, NULL, NULL);
            char* pUtf8Name = new char[utf8Len];
            WideCharToMultiByte(CP_UTF8, 0, strDestName, -1, pUtf8Name, utf8Len, NULL, NULL);

            // 서버 파일 열기 (쓰기 모드)
            HINTERNET hFtpFile = ::FtpOpenFileA((HINTERNET)*pFtpConn, pUtf8Name, GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_PASSIVE, 0);
            delete[] pUtf8Name;

            if (hFtpFile != NULL) {
                const DWORD BUFFER_SIZE = 64 * 1024;
                BYTE* buffer = new BYTE[BUFFER_SIZE];
                DWORD bytesWritten = 0;
                ULONGLONG totalUploaded = 0;
                UINT readCount = 0;

                while ((readCount = localFile.Read(buffer, BUFFER_SIZE)) > 0) {
                    if (::InternetWriteFile(hFtpFile, buffer, readCount, &bytesWritten)) {
                        totalUploaded += bytesWritten;

                        // UI 업데이트 (속도/진행률) 로직은 다운로드와 동일하게 적용
                        DWORD dwCurrentTick = GetTickCount();
                        if (dwCurrentTick - pReq->dwSpeedTick >= 1000) {
                            double mbps = (double)(totalUploaded - pReq->nLastBytes) / (1024.0 * 1024.0);
                            ::PostMessage(pReq->hMainWnd, WM_UPDATE_SPEED, MAKEWPARAM((WORD)(mbps * 10), 0), (LPARAM)pReq->nItemIndex);
                            pReq->dwSpeedTick = dwCurrentTick;
                            pReq->nLastBytes = totalUploaded;
                        }
                        int percent = (int)((double)totalUploaded / (double)pReq->nTotalBytes * 100);
                        if (percent > pReq->nLastPercent) {
                            pReq->nLastPercent = percent;
                            ::PostMessage(pReq->hMainWnd, WM_UPDATE_PROGRESS, (WPARAM)percent, (LPARAM)pReq->nItemIndex);
                        }
                    }
                }
                delete[] buffer;
                ::InternetCloseHandle(hFtpFile);
                bResult = TRUE;
            }
            localFile.Close();
        }
    }
    catch (CInternetException* pEx) { pEx->Delete(); }

    if (pFtpConn) { pFtpConn->Close(); delete pFtpConn; }
    session.Close();

    ::PostMessage(pReq->hMainWnd, WM_DOWNLOAD_COMPLETE, (WPARAM)bResult, (LPARAM)pReq->nItemIndex);
    ReleaseSemaphore(pMain->m_hSemaphore, 1, NULL);
    delete pReq;
    return 0;
}


// [새로 추가] 웹하드(FTP) 다운로드 엔진 (한글 인코딩 완벽 우회 + 윈도우 코어 API 직결 버전)
UINT FtpDownloadThreadProc(LPVOID pParam) {
    DownloadRequest* pReq = (DownloadRequest*)pParam;
    if (pReq == NULL) return 0;

    Ckpaxkkk01Dlg* pMain = (Ckpaxkkk01Dlg*)CWnd::FromHandle(pReq->hMainWnd);

    // 세마포어 대기
    WaitForSingleObject(pMain->m_hSemaphore, INFINITE);

    pReq->dwLastTick = GetTickCount();
    pReq->dwSpeedTick = GetTickCount();
    pReq->nLastBytes = 0;
    pReq->nLastPercent = -1;
    pReq->nTotalBytes = 0;

    BOOL bResult = FALSE;

    CInternetSession session(_T("KpaxWebHardClient"));
    CFtpConnection* pFtpConn = NULL;
    CFile localFile; // (주의: CInternetFile은 아예 지웠습니다!)

    try {

        // 1. 서버에 로그인 (패시브 모드)
        pFtpConn = session.GetFtpConnection(SERVER_IP, _T("friend"), _T("1111"), 2121, TRUE);

        // 파일질라에게 UTF-8을 쓰겠다고 선언
        pFtpConn->Command(_T("OPTS UTF8 ON"));

        // ==========================================================
        // [초강력 핵심] 파일명을 진짜 UTF-8 바이트(char 배열)로 직접 변환!
        CString strFileName = pReq->source.c_str();
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, strFileName, -1, NULL, 0, NULL, NULL);
        char* pUtf8Name = new char[utf8Len];
        WideCharToMultiByte(CP_UTF8, 0, strFileName, -1, pUtf8Name, utf8Len, NULL, NULL);

        // 윈도우 원시 API를 직접 호출하여 UTF-8 바이트를 서버에 그대로 꽂아버림!
        HINTERNET hFtpFile = ::FtpOpenFileA((HINTERNET)*pFtpConn, pUtf8Name, GENERIC_READ, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_PASSIVE, 0);
        delete[] pUtf8Name;
        // ==========================================================

        if (hFtpFile != NULL) {
            // 2. 파일 용량 알아내기
            DWORD dwFileSizeHigh = 0;
            DWORD dwFileSizeLow = ::FtpGetFileSize(hFtpFile, &dwFileSizeHigh);
            pReq->nTotalBytes = ((ULONGLONG)dwFileSizeHigh << 32) | dwFileSizeLow;

            // 3. 내 컴퓨터에 파일 쓰기 시작
            if (localFile.Open(pReq->target.c_str(), CFile::modeCreate | CFile::modeWrite | CFile::typeBinary)) {

                // 4. 다운로드 루프 (64KB 단위) - 껍데기를 버리고 API로 직접 읽기!
                const DWORD BUFFER_SIZE = 64 * 1024;
                BYTE* buffer = new BYTE[BUFFER_SIZE];
                DWORD bytesRead = 0;
                ULONGLONG totalDownloaded = 0;

                // [핵심 변경] ::InternetReadFile 코어 API 사용!
                while (::InternetReadFile(hFtpFile, buffer, BUFFER_SIZE, &bytesRead) && bytesRead > 0) {
                    localFile.Write(buffer, bytesRead);
                    totalDownloaded += bytesRead;

                    DWORD dwCurrentTick = GetTickCount();

                    // 속도 및 ETA 계산
                    if (dwCurrentTick - pReq->dwSpeedTick >= 1000) {
                        double diff = (double)(totalDownloaded - pReq->nLastBytes);
                        double mbps = diff / (1024.0 * 1024.0);
                        int eta = 0;
                        if (diff > 0 && pReq->nTotalBytes > 0) {
                            eta = (int)((pReq->nTotalBytes - totalDownloaded) / diff);
                        }
                        WPARAM wp = MAKEWPARAM((WORD)(mbps * 10), (WORD)eta);
                        ::PostMessage(pReq->hMainWnd, WM_UPDATE_SPEED, wp, (LPARAM)pReq->nItemIndex);

                        pReq->dwSpeedTick = dwCurrentTick;
                        pReq->nLastBytes = totalDownloaded;
                    }

                    // 진행률 업데이트
                    if (pReq->nTotalBytes > 0) {
                        int percent = (int)((double)totalDownloaded / (double)pReq->nTotalBytes * 100);
                        if (percent > pReq->nLastPercent) {
                            pReq->nLastPercent = percent;
                            ::PostMessage(pReq->hMainWnd, WM_UPDATE_PROGRESS, (WPARAM)percent, (LPARAM)pReq->nItemIndex);
                        }
                    }
                }
                delete[] buffer;
                bResult = TRUE;
                localFile.Close();
            }
            // 코어 핸들 닫기
            ::InternetCloseHandle(hFtpFile);
        }
    }
    catch (CInternetException* pEx) {
        TCHAR szErr[1024];
        pEx->GetErrorMessage(szErr, 1024);
        OutputDebugString(szErr);
        pEx->Delete();
    }

    // 5. 연결 종료
    if (pFtpConn != NULL) {
        pFtpConn->Close();
        delete pFtpConn;
    }
    session.Close();

    ::PostMessage(pReq->hMainWnd, WM_DOWNLOAD_COMPLETE, (WPARAM)bResult, (LPARAM)pReq->nItemIndex);
    ReleaseSemaphore(pMain->m_hSemaphore, 1, NULL);

    delete pReq;
    return 0;
}


// [추가] 실제 다운로드 파일 복사를 수행하는 워커 스레드 함수
UINT DownloadThreadProc(LPVOID pParam) {
    // 다운로드 로직은 업로드와 거의 동일하므로 UploadThreadProc를 재사용하거나 
    // 동일한 구조의 함수를 하나 더 둡니다.
    return UploadThreadProc(pParam);
}


// [추가] 윈도우 레지스트리에 내 프로그램(kpax://) 등록하기!
void RegisterURIScheme() {
    TCHAR szExePath[MAX_PATH];
    GetModuleFileName(NULL, szExePath, MAX_PATH); // 내 프로그램의 현재 위치 가져오기

    // 관리자 권한이 필요 없는 HKEY_CURRENT_USER 영역에 등록합니다.
    CString strKey = L"Software\\Classes\\kpax";
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, strKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        CString strProtocol = L"URL:Kpax Protocol";
        RegSetValueEx(hKey, NULL, 0, REG_SZ, (const BYTE*)strProtocol.GetString(), (strProtocol.GetLength() + 1) * sizeof(TCHAR));
        RegSetValueEx(hKey, L"URL Protocol", 0, REG_SZ, (const BYTE*)L"", sizeof(TCHAR));

        HKEY hCmdKey;
        CString strCmdKey = strKey + L"\\shell\\open\\command";
        if (RegCreateKeyEx(HKEY_CURRENT_USER, strCmdKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hCmdKey, NULL) == ERROR_SUCCESS) {
            CString strCommand;
            strCommand.Format(L"\"%s\" \"%%1\"", szExePath); // 프로그램 경로 "%1" 형태로 등록
            RegSetValueEx(hCmdKey, NULL, 0, REG_SZ, (const BYTE*)strCommand.GetString(), (strCommand.GetLength() + 1) * sizeof(TCHAR));
            RegCloseKey(hCmdKey);
        }
        RegCloseKey(hKey);
    }
}



//BOOL Ckpaxkkk01Dlg::OnInitDialog() {
//    CDialogEx::OnInitDialog();
//
//    SetIcon(m_hIcon, TRUE);
//    SetIcon(m_hIcon, FALSE);
//
//    // 1. 초기 설정
//    //m_hSemaphore = CreateSemaphore(NULL, 10, 10, NULL);
//    m_hSemaphore = CreateSemaphore(NULL, 2, 2, NULL);
//
//    // LVS_EX_CHECKBOXES 스타일 추가
//    m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);
//
//    // LVS_EX_CHECKBOXES 가 누락되면 GetCheck가 절대 작동하지 않습니다.
//    DWORD dwExStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER;
//    m_ListCtrl.SetExtendedStyle(dwExStyle);
//
//    m_ListCtrl.InsertColumn(0, L"파일명", LVCFMT_LEFT, 180);
//    m_ListCtrl.InsertColumn(1, L"진행률", LVCFMT_CENTER, 100);
//    m_ListCtrl.InsertColumn(2, L"상태", LVCFMT_LEFT, 220);
//
//    DragAcceptFiles(TRUE);
//    ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
//    ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
//
//    // 2. 상태바 생성
//    static UINT indicators[] = { ID_SEPARATOR };
//    if (m_StatusBar.Create(this)) {
//        m_StatusBar.SetIndicators(indicators, 1);
//        RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
//    }
//
//    UpdateTotalStatus();
//
//
//    // ... 기존 경로 설정 코드 바로 아래 ...
//   // [여기에 넣으세요!] 기존 경로 설정 코드 바로 아래입니다.
//    m_strDownloadPath = L"C:\\Test";
//    SetDlgItemText(IDC_EDIT_PATH, m_strDownloadPath);
//
//    // ==========================================================================
//    // 1. 레지스트리 자동 등록 (프로그램 실행될 때마다 최신 경로로 갱신)
//    RegisterURIScheme();
//
//    // 2. 브라우저가 던져준 "쪽지(파일명)" 낚아채기
//    CString strCmdLine = AfxGetApp()->m_lpCmdLine;
//
//    if (!strCmdLine.IsEmpty()) {
//        // [디버깅] 도대체 어떤 글자가 오는지 궁금하다면 아래 주석을 푸세요
//        // AfxMessageBox(L"수신된 데이터: " + strCmdLine);
//
//        strCmdLine.TrimLeft();
//        strCmdLine.TrimRight();
//        strCmdLine.Remove('\"'); // 따옴표 제거
//
//        CString strPrefix = L"kpax://";
//        // 'kpax://' 라는 글자가 어디에 있는지 찾습니다.
//        int nPos = strCmdLine.Find(strPrefix);
//
//        if (nPos != -1) {
//            // 접두사 이후의 글자(인코딩된 파일명)만 추출
//            CString strEncoded = strCmdLine.Mid(nPos + strPrefix.GetLength());
//            strEncoded.TrimRight(L"/"); // 끝에 붙은 슬래시 제거
//
//            // 3. 브라우저 외계어(%EB%93%9C...)를 한글로 번역
//            TCHAR szDecoded[MAX_PATH] = { 0 };
//            DWORD dwSize = MAX_PATH;
//            UrlUnescape((LPTSTR)(LPCTSTR)strEncoded, szDecoded, &dwSize, URL_UNESCAPE_INPLACE);
//
//            CString strFileName(szDecoded);
//
//            if (!strFileName.IsEmpty()) {
//                // 4. 리스트 컨트롤에 즉시 추가
//                int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), strFileName);
//                m_ListCtrl.SetItemText(nIdx, 1, L"0%");
//                m_ListCtrl.SetItemText(nIdx, 2, L"연결 중...");
//
//                // 5. 실제 다운로드 스레드 가동
//                CreateDirectory(m_strDownloadPath, NULL);
//                CString strSavePath = m_strDownloadPath + L"\\" + strFileName;
//
//                DownloadRequest* pReq = new DownloadRequest();
//                pReq->hMainWnd = this->GetSafeHwnd();
//                pReq->nItemIndex = nIdx;
//                pReq->source = (LPCTSTR)strFileName;
//                pReq->target = (LPCTSTR)strSavePath;
//                pReq->nTotalBytes = 0;
//                pReq->nLastBytes = 0;
//                pReq->nLastPercent = -1;
//                pReq->dwLastTick = GetTickCount();
//                pReq->dwSpeedTick = GetTickCount();
//
//                AfxBeginThread(FtpDownloadThreadProc, pReq);
//            }
//        }
//    }
//    // ... 이 아래는 원래 있던 return TRUE; ...
//
//    return TRUE;  // return TRUE unless you set the focus to a control
//}



//
//BOOL Ckpaxkkk01Dlg::OnInitDialog() {
//    CDialogEx::OnInitDialog();
//
//    SetIcon(m_hIcon, TRUE);
//    SetIcon(m_hIcon, FALSE);
//
//    // 1. 리스트 컨트롤 초기화 (이게 무조건 1등이어야 합니다)
//    DWORD dwExStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER;
//    m_ListCtrl.SetExtendedStyle(dwExStyle);
//
//    m_ListCtrl.InsertColumn(0, L"파일명", LVCFMT_LEFT, 180);
//    m_ListCtrl.InsertColumn(1, L"진행률", LVCFMT_CENTER, 100);
//    m_ListCtrl.InsertColumn(2, L"상태", LVCFMT_LEFT, 220);
//
//    // 2. 기본 경로 설정
//    m_strDownloadPath = L"C:\\Test";
//    SetDlgItemText(IDC_EDIT_PATH, m_strDownloadPath);
//
//    // [kpaxkkk01Dlg.cpp] OnInitDialog 내부의 웹 처리 로직 교체
//    RegisterURIScheme();
//
//    // 1. 윈도우가 보낸 "원본 생 데이터"를 가져옵니다.
//    CString strRawCmd = GetCommandLine();
//
//    // 2. [가장 중요] 눈으로 확인해봅시다.
//    // 이 창이 뜨는데 목록이 안 뜨는 건지, 이 창 자체가 안 뜨는 건지 알려주세요!
//    AfxMessageBox(L"수신된 전체 데이터: " + strRawCmd);
//
//    int nPos = strRawCmd.Find(L"kpax://");
//    CString strFileName = L"";
//
//    if (nPos != -1) {
//        // kpax:// 이후를 잘라냄
//        strFileName = strRawCmd.Mid(nPos + 7);
//        strFileName.Remove('\"'); // 따옴표 제거
//        strFileName.TrimRight(L"/ "); // 슬래시와 공백 제거
//
//        // 한글 번역 시도
//        TCHAR szDecoded[MAX_PATH] = { 0 };
//        DWORD dwSize = MAX_PATH;
//        UrlUnescape((LPTSTR)(LPCTSTR)strFileName, szDecoded, &dwSize, URL_UNESCAPE_INPLACE);
//        if (_tcslen(szDecoded) > 0) strFileName = szDecoded;
//    }
//    else {
//        // 만약 kpax:// 를 못 찾았다면, 원본이라도 넣어서 목록을 띄웁니다.
//        strFileName = L"알 수 없는 호출: " + strRawCmd;
//    }
//
//    // 3. 리스트 컨트롤에 강제 삽입
//    if (m_ListCtrl.GetSafeHwnd()) {
//        int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), strFileName);
//        m_ListCtrl.SetItemText(nIdx, 1, L"대기 중");
//        m_ListCtrl.SetItemText(nIdx, 2, L"웹 호출 확인됨");
//    }
//    else {
//        // 만약 리스트 컨트롤 자체가 준비 안 됐다면 이 메시지가 뜹니다.
//        AfxMessageBox(L"에러: 리스트 컨트롤 변수가 연결되지 않았습니다!");
//    }
//    UpdateTotalStatus();
//    return TRUE;
//}


#include <vector> // 파일 맨 위에 이 줄이 있는지 확인하세요!

BOOL Ckpaxkkk01Dlg::OnInitDialog() {
    CDialogEx::OnInitDialog();


    // [수정된 부분] kpaxkkk01Dlg.cpp의 OnInitDialog 내부
    if (!m_ImageList.GetSafeHandle()) {
        // 16x16 크기, 32비트 컬러 아이콘 리스트 생성
        m_ImageList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 2);

        // 윈도우 표준 시스템 아이콘 로드 (IDI_FOLDER 대신 표준 상수 사용)
        // IDI_APPLICATION을 사용하거나 아래처럼 직접 윈도우 핸들로 가져올 수 있습니다.
        HICON hFolderIcon = AfxGetApp()->LoadStandardIcon(IDI_APPLICATION); // 기본 앱 모양
        HICON hFileIcon = AfxGetApp()->LoadStandardIcon(IDI_WINLOGO);     // 윈도우 로고 모양

        // 만약 더 폴더다운 아이콘을 원하시면 아래 주석처리된 방식을 쓰세요 (Shell API)
        /*
        SHFILEINFO sfi;
        SHGetFileInfo(L"C:\\Windows", FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
        hFolderIcon = sfi.hIcon;
        SHGetFileInfo(L"test.txt", FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
        hFileIcon = sfi.hIcon;
        */

        m_ImageList.Add(hFolderIcon); // 0번: 폴더 대용
        m_ImageList.Add(hFileIcon);   // 1번: 파일 대용

        m_ListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);
    }

    m_ListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

    // 2. 초기 경로 설정
    m_strCurrentPath = L"/";


    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // 1. 리스트 컨트롤 초기화
    DWORD dwExStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER;
    m_ListCtrl.SetExtendedStyle(dwExStyle);

    m_ListCtrl.InsertColumn(0, L"파일명", LVCFMT_LEFT, 250);
    m_ListCtrl.InsertColumn(1, L"진행률", LVCFMT_CENTER, 100);
    m_ListCtrl.InsertColumn(2, L"상태", LVCFMT_LEFT, 200);

    m_strDownloadPath = L"C:\\Test";
    SetDlgItemText(IDC_EDIT_PATH, m_strDownloadPath);

    RegisterURIScheme();

    // 2. 명령줄 데이터 가져오기
    CString strRawCmd = GetCommandLine();
    int nPos = strRawCmd.Find(L"kpax://");
    CString strFileName = L"";

    if (nPos != -1) {
        CString strEncoded = strRawCmd.Mid(nPos + 7);
        strEncoded.Remove('\"');
        strEncoded.TrimRight(L"/ ");

        // --- [물리적 디코딩 시작] ---
        std::vector<char> utf8Bytes;
        for (int i = 0; i < strEncoded.GetLength(); ++i) {
            if (strEncoded[i] == '%') {
                int value;
                // % 뒤의 두 글자를 16진수 숫자로 변환
                if (swscanf_s(strEncoded.Mid(i + 1, 2), L"%x", &value) == 1) {
                    utf8Bytes.push_back((char)value);
                    i += 2;
                }
            }
            else {
                // 일반 영문자 처리
                utf8Bytes.push_back((char)strEncoded[i]);
            }
        }
        utf8Bytes.push_back('\0'); // 문자열 끝 표시

        // 3. 복원된 UTF-8 바이트를 유니코드로 정밀 변환
        if (!utf8Bytes.empty()) {
            int nUniLen = MultiByteToWideChar(CP_UTF8, 0, utf8Bytes.data(), -1, NULL, 0);
            if (nUniLen > 0) {
                std::vector<wchar_t> uniBuf(nUniLen);
                MultiByteToWideChar(CP_UTF8, 0, utf8Bytes.data(), -1, uniBuf.data(), nUniLen);
                strFileName = uniBuf.data(); // 드디어 깨끗한 한글 복원!
            }
        }
        // -----------------------------

        if (strFileName.IsEmpty()) strFileName = strEncoded;
    }
    else {
        UpdateTotalStatus();
        return TRUE;
    }

    // 4. 리스트 컨트롤 삽입 및 다운로드 실행
    if (m_ListCtrl.GetSafeHwnd() && !strFileName.IsEmpty()) {
        int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), strFileName);
        m_ListCtrl.SetItemText(nIdx, 1, L"0%");
        m_ListCtrl.SetItemText(nIdx, 2, L"전송 연결 중...");

        CreateDirectory(m_strDownloadPath, NULL);
        CString strSavePath = m_strDownloadPath + L"\\" + strFileName;

        DownloadRequest* pReq = new DownloadRequest();
        pReq->hMainWnd = this->GetSafeHwnd();
        pReq->nItemIndex = nIdx;
        pReq->source = (LPCTSTR)strFileName;
        pReq->target = (LPCTSTR)strSavePath;
        pReq->nTotalBytes = 0; pReq->nLastBytes = 0; pReq->nLastPercent = -1;
        pReq->dwLastTick = GetTickCount(); pReq->dwSpeedTick = GetTickCount();

        AfxBeginThread(FtpDownloadThreadProc, pReq);
    }

    UpdateTotalStatus();
    return TRUE;
}


// [상태바 및 제목줄 업데이트 함수]
void Ckpaxkkk01Dlg::UpdateTotalStatus() {
    if (!m_StatusBar.GetSafeHwnd()) return;

    m_nTotalFiles = m_ListCtrl.GetItemCount();
    
    // 리스트를 순회하며 완료된 파일 수를 정확히 계산
    int currentDone = 0;
    for (int i = 0; i < m_nTotalFiles; i++) {
        if (m_ListCtrl.GetItemText(i, 2) == L"완료") currentDone++;
    }
    m_nDoneFiles = currentDone;

    CString strStatus, strTitle;
    if (m_nTotalFiles > 0) {
        int nTotalPercent = (m_nDoneFiles * 100) / m_nTotalFiles;
        strStatus.Format(L" 진행 상황: %d / %d 완료 (%d%%)", m_nDoneFiles, m_nTotalFiles, nTotalPercent);
        strTitle.Format(L"[%d%%] 파일 전송 매니저", nTotalPercent);
    } else {
        strStatus = L" 대기 중...";
        strTitle = L"파일 전송 매니저";
    }

    m_StatusBar.SetPaneText(0, strStatus);
    SetWindowText(strTitle);
}

// [자동 삭제 로직]
void Ckpaxkkk01Dlg::ProcessAutoClear() {
    if (IsDlgButtonChecked(IDC_CHK_AUTO_CLEAR) == BST_CHECKED) {
        for (int i = m_ListCtrl.GetItemCount() - 1; i >= 0; i--) {
            if (m_ListCtrl.GetItemText(i, 2) == L"완료") {
                m_ListCtrl.DeleteItem(i);
            }
        }
    }
}

// UI 업데이트 핸들러
LRESULT Ckpaxkkk01Dlg::OnUpdateProgress(WPARAM wp, LPARAM lp) {
    int nPercent = (int)wp;
    int nIdx = (int)lp;
    CString str;
    str.Format(L"%d%%", nPercent);
    m_ListCtrl.SetItemText(nIdx, 1, str);
    m_ListCtrl.RedrawItems(nIdx, nIdx);
    return 0;
}

LRESULT Ckpaxkkk01Dlg::OnDownloadComplete(WPARAM wp, LPARAM lp) {
    BOOL bSuccess = (BOOL)wp;
    int nIdx = (int)lp;

    if (bSuccess) {
        m_ListCtrl.SetItemText(nIdx, 1, L"100%");
        m_ListCtrl.SetItemText(nIdx, 2, L"완료");
        m_ListCtrl.RedrawItems(nIdx, nIdx);
        ProcessAutoClear(); // 자동 삭제 체크
    } else {
        m_ListCtrl.SetItemText(nIdx, 2, L"실패: " + GetErrorMessageKorean(GetLastError()));
    }

    UpdateTotalStatus();


    // 전체 파일 수와 완료된 파일 수가 같은지 체크합니다.
    if (m_nDoneFiles == m_nTotalFiles && m_nTotalFiles > 0) {
        // 모든 전송이 끝났을 때 딱 한 번 폴더 열기
        ShellExecute(NULL, L"open", L"C:\\Test", NULL, NULL, SW_SHOW);

        // 축하 메시지를 띄우고 싶다면 추가
        // AfxMessageBox(L"모든 파일의 다운로드가 완료되었습니다!");
    }

    return 0;
}

// 업로드 버튼 클릭   로컬부분이다~~~~~~~~~~~~~~~~~~~~~~~
//void Ckpaxkkk01Dlg::OnBnClickedBtnUpload() {
//    CFileDialog fileDlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST, L"모든 파일 (*.*)|*.*||");
//    if (fileDlg.DoModal() == IDOK) {
//        CString strLocalPath = fileDlg.GetPathName();
//        CString strFileName = fileDlg.GetFileName();
//
//        if (!CreateDirectory(L"C:\\Test", NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return;
//
//        std::wstring strServerPath = L"C:\\Test\\" + std::wstring((LPCTSTR)strFileName);
//
//        int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), strFileName);
//        m_ListCtrl.SetItemText(nIdx, 1, L"0%");
//        m_ListCtrl.SetItemText(nIdx, 2, L"업로드 중...");
//
//        DownloadRequest* pReq = new DownloadRequest();
//        pReq->hMainWnd = this->GetSafeHwnd();
//        pReq->source = (LPCTSTR)strLocalPath;
//        pReq->target = strServerPath;
//        pReq->nItemIndex = nIdx;
//        pReq->nLastPercent = -1;
//        pReq->dwLastTick = GetTickCount();
//
//        AfxBeginThread(UploadThreadProc, pReq);
//        UpdateTotalStatus();
//    }
//}

void Ckpaxkkk01Dlg::OnBnClickedBtnUpload() {
    CFileDialog fileDlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST, L"모든 파일 (*.*)|*.*||");
    if (fileDlg.DoModal() == IDOK) {
        CString strLocalPath = fileDlg.GetPathName();
        CString strFileName = fileDlg.GetFileName();

        int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), strFileName);
        m_ListCtrl.SetItemText(nIdx, 1, L"0%");
        m_ListCtrl.SetItemText(nIdx, 2, L"업로드 준비 중...");

        DownloadRequest* pReq = new DownloadRequest();
        pReq->hMainWnd = this->GetSafeHwnd();
        pReq->source = (LPCTSTR)strLocalPath; // 내 컴퓨터 경로
        pReq->target = (LPCTSTR)strFileName;  // 서버에 저장될 이름
        pReq->nItemIndex = nIdx;

        AfxBeginThread(FtpUploadThreadProc, pReq); // FTP 업로드 스레드 실행
        UpdateTotalStatus();
    }
}

// 드래그 앤 드롭  로컬부분이다----------------------
//void Ckpaxkkk01Dlg::OnDropFiles(HDROP hDropInfo) {
//    UINT nFiles = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
//    for (UINT i = 0; i < nFiles; i++) {
//        TCHAR szFilePath[MAX_PATH];
//        if (DragQueryFile(hDropInfo, i, szFilePath, MAX_PATH)) {
//            CString strLocalPath = szFilePath;
//            CString strFileName = strLocalPath.Mid(strLocalPath.ReverseFind('\\') + 1);
//            std::wstring strServerPath = L"C:\\Test\\" + std::wstring((LPCTSTR)strFileName);
//
//            int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), strFileName);
//            m_ListCtrl.SetItemText(nIdx, 1, L"0%");
//            m_ListCtrl.SetItemText(nIdx, 2, L"연결 중...");
//
//            DownloadRequest* pReq = new DownloadRequest();
//            pReq->hMainWnd = this->GetSafeHwnd();
//            pReq->source = (LPCTSTR)strLocalPath;
//            pReq->target = strServerPath;
//            pReq->nItemIndex = nIdx;
//            pReq->nLastPercent = -1;
//            pReq->dwLastTick = GetTickCount();
//
//            AfxBeginThread(UploadThreadProc, pReq);
//        }
//    }
//    DragFinish(hDropInfo);
//    UpdateTotalStatus();
//}

void Ckpaxkkk01Dlg::OnDropFiles(HDROP hDropInfo) {
    UINT nFiles = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
    for (UINT i = 0; i < nFiles; i++) {
        TCHAR szFilePath[MAX_PATH];
        if (DragQueryFile(hDropInfo, i, szFilePath, MAX_PATH)) {
            CString strLocalPath = szFilePath;
            CString strFileName = strLocalPath.Mid(strLocalPath.ReverseFind('\\') + 1);

            int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), strFileName);
            m_ListCtrl.SetItemText(nIdx, 1, L"0%");
            m_ListCtrl.SetItemText(nIdx, 2, L"업로드 중...");

            DownloadRequest* pReq = new DownloadRequest();
            pReq->hMainWnd = this->GetSafeHwnd();
            pReq->source = (LPCTSTR)strLocalPath;
            pReq->target = (LPCTSTR)strFileName;
            pReq->nItemIndex = nIdx;

            AfxBeginThread(FtpUploadThreadProc, pReq); // FTP 업로드 스레드 실행
        }
    }
    DragFinish(hDropInfo);
    UpdateTotalStatus();
}

// 완료 목록 삭제 버튼
void Ckpaxkkk01Dlg::OnBnClickedBtnClearFinished() {
    for (int i = m_ListCtrl.GetItemCount() - 1; i >= 0; i--) {
        CString strStatus = m_ListCtrl.GetItemText(i, 2);
        if (strStatus == L"완료" || strStatus.Find(L"실패") != -1) {
            m_ListCtrl.DeleteItem(i);
        }
    }
    UpdateTotalStatus();
}

// 체크박스 클릭 핸들러
void Ckpaxkkk01Dlg::OnBnClickedChkAutoClear() {
    ProcessAutoClear();
    UpdateTotalStatus();
}

// [공용] 커스텀 드로우 (게이지 그리기)
void Ckpaxkkk01Dlg::OnNMCustomdrawListDownload(NMHDR* pNMHDR, LRESULT* pResult) {
    LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
    *pResult = CDRF_DODEFAULT;
    if (pLVCD->nmcd.dwDrawStage == CDDS_PREPAINT) *pResult = CDRF_NOTIFYITEMDRAW;
    else if (pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) *pResult = CDRF_NOTIFYSUBITEMDRAW;
    else if (pLVCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEMPREPAINT)) {
        if (pLVCD->iSubItem == 1) {
            CDC* pDC = CDC::FromHandle(pLVCD->nmcd.hdc);
            CRect rect;
            m_ListCtrl.GetSubItemRect(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);
            CString strPercent = m_ListCtrl.GetItemText(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem);
            int nPercent = _ttoi(strPercent);
            pDC->FillSolidRect(rect, RGB(255, 255, 255));
            if (nPercent > 0) {
                CRect progRect = rect;
                progRect.DeflateRect(2, 2);
                progRect.right = progRect.left + (int)(progRect.Width() * (nPercent / 100.0));
                pDC->FillSolidRect(progRect, RGB(0, 120, 215));
            }
            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(nPercent > 50 ? RGB(255, 255, 255) : RGB(0, 0, 0));
            pDC->DrawText(strPercent, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            *pResult = CDRF_SKIPDEFAULT;
        }
    }
}


// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
// 아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
// 프레임워크에서 이 작업을 자동으로 수행합니다.

void Ckpaxkkk01Dlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 아이콘을 그립니다.
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
// 이 함수를 호출합니다.
HCURSOR Ckpaxkkk01Dlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// 시스템 메뉴 명령 처리 (정보 상자 등)
void Ckpaxkkk01Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}


// 우클릭 메뉴에서 '삭제'를 눌렀을 때 실행되는 함수
void Ckpaxkkk01Dlg::OnMenuDelete()
{
    // 리스트 컨트롤에서 현재 선택된 아이템의 인덱스를 가져옵니다.
    int nItem = m_ListCtrl.GetNextItem(-1, LVNI_SELECTED);

    if (nItem != -1)
    {
        // 정말 삭제할지 확인창을 띄웁니다.
        if (AfxMessageBox(L"목록에서 삭제하시겠습니까?", MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            m_ListCtrl.DeleteItem(nItem);

            // 삭제 후 전체 개수가 변했으므로 상태바 카운트도 갱신합니다.
            UpdateTotalStatus();
        }
    }
}

// 리스트 컨트롤 우클릭 시 팝업 메뉴를 띄우는 함수 (이것도 누락되었을 수 있으니 확인하세요)
void Ckpaxkkk01Dlg::OnNMRClickListDownload(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

    if (pNMItemActivate->iItem != -1) // 아이템(행) 위에서 우클릭했을 때만
    {
        CMenu menu;
        if (menu.LoadMenu(IDR_MENU_LIST)) // 리소스에 만든 메뉴 불러오기
        {
            CMenu* pPopup = menu.GetSubMenu(0);
            CPoint pt;
            GetCursorPos(&pt);
            pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
        }
    }
    *pResult = 0;
}


//로컬 테스트
//// '시작' 버튼을 눌렀을 때 실행되는 함수
//void Ckpaxkkk01Dlg::OnBnClickedBtnStart()
//{
//    m_ListCtrl.DeleteAllItems(); // 기존 목록 청소
//
//    CFileFind finder;
//    // 서버의 공유 폴더 경로 (실제 경로로 수정하세요)
//    //CString strServerPath = L"C:\\Test\\*.*";
//    //CString strServerPath = L"H:\\PC2 2TB HDD 자료\\영화,드라마,애니메이션\\마쇼파일\\애니메이션, 전대물\\하울의움직이는성 (2004)\\*.*";
//    CString strServerPath = L"D:\\드래곤볼 시리즈\\오리지널 (1986)\\(오덕) 드래곤볼 오리지널 121-140화  한글자막.ㅋ\\*.*";
//    //H:\PC2 2TB HDD 자료\영화,드라마,애니메이션\마쇼파일\애니메이션, 전대물\나루토 (2002)
//
//    BOOL bWorking = finder.FindFile(strServerPath);
//    while (bWorking) {
//        bWorking = finder.FindNextFile();
//
//        if (finder.IsDots()) continue; // . 이나 .. 폴더 제외
//        if (finder.IsDirectory()) continue; // 일단 파일만 표시
//
//        // 리스트에 파일명 추가
//        int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), finder.GetFileName());
//        m_ListCtrl.SetItemText(nIdx, 1, L"0%");
//        m_ListCtrl.SetItemText(nIdx, 2, L"대기 중 (체크 후 다운로드)");
//    }
//    finder.Close();
//
//    UpdateTotalStatus();
//}

//파일질라 서버 호출 실제 웹서버를 부르는 화면입니다.



//void Ckpaxkkk01Dlg::OnBnClickedBtnStart()
//{
//    m_ListCtrl.DeleteAllItems();
//
//    CInternetSession session(_T("KpaxWebHardClient"));
//    CFtpConnection* pFtpConn = NULL;
//
//    try {
//        //CString strIP = _T("125.188.38.149");
//        pFtpConn = session.GetFtpConnection(SERVER_IP, _T("friend"), _T("1111"), 2121, TRUE);
//
//        // 1. 서버에 UTF8 사용 강제 명령
//        pFtpConn->Command(_T("OPTS UTF8 ON"));
//
//        // 2. [핵심] MFC 클래스 대신 윈도우 API(A버전)를 직접 사용합니다.
//        // FtpFindFirstFileA는 파일명을 바이트(char*) 그대로 가져옵니다.
//        WIN32_FIND_DATAA fd;
//        HINTERNET hFind = ::FtpFindFirstFileA((HINTERNET)*pFtpConn, "*.*", &fd, INTERNET_FLAG_RELOAD | INTERNET_FLAG_PASSIVE, 0);
//
//        if (hFind != NULL) {
//            BOOL bContinue = TRUE;
//            while (bContinue) {
//                // 폴더(.)나 상위폴더(..) 제외
//                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
//
//                    // 3. 서버가 준 UTF-8 바이트를 유니코드로 정밀 변환
//                    int nUniLen = MultiByteToWideChar(CP_UTF8, 0, fd.cFileName, -1, NULL, 0);
//                    CString strFileName;
//
//                    if (nUniLen > 0) {
//                        std::vector<wchar_t> uniBuf(nUniLen);
//                        MultiByteToWideChar(CP_UTF8, 0, fd.cFileName, -1, uniBuf.data(), nUniLen);
//                        strFileName = uniBuf.data();
//                    }
//                    else {
//                        strFileName = fd.cFileName; // 변환 실패시 그대로 출력
//                    }
//
//                    // 4. 리스트 컨트롤에 추가
//                    int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), strFileName);
//                    m_ListCtrl.SetItemText(nIdx, 1, L"0%");
//                    m_ListCtrl.SetItemText(nIdx, 2, L"대기 중");
//                }
//
//                bContinue = ::InternetFindNextFileA(hFind, &fd);
//            }
//            ::InternetCloseHandle(hFind);
//        }
//    }
//    catch (CInternetException* pEx) {
//        TCHAR szErr[1024];
//        pEx->GetErrorMessage(szErr, 1024);
//        AfxMessageBox(L"접속 에러: " + CString(szErr));
//        pEx->Delete();
//    }
//
//    if (pFtpConn) { pFtpConn->Close(); delete pFtpConn; }
//    session.Close();
//    UpdateTotalStatus();
//}
//
//
//LRESULT Ckpaxkkk01Dlg::OnUpdateSpeed(WPARAM wp, LPARAM lp) {
//    int speedData = LOWORD(wp); // 속도 (*10 상태)
//    int eta = HIWORD(wp);       // 남은 시간 (초)
//    int nIdx = (int)lp;
//
//    double mbps = speedData / 10.0;
//    CString strStatus;
//
//    if (mbps < 0.1) {
//        strStatus = L"연결 중...";
//    }
//    else {
//        // [수정] 남은 시간을 분:초 형식으로 변환
//        if (eta >= 60) {
//            strStatus.Format(L"전송 중 (%.1f MB/s) - %d분 %d초 남음", mbps, eta / 60, eta % 60);
//        }
//        else {
//            strStatus.Format(L"전송 중 (%.1f MB/s) - %d초 남음", mbps, eta);
//        }
//    }
//
//    // 리스트 컨트롤의 '상태' 컬럼 업데이트
//    m_ListCtrl.SetItemText(nIdx, 2, strStatus);
//
//    return 0;
//}
//

void Ckpaxkkk01Dlg::OnBnClickedBtnStart()
{
    m_ListCtrl.DeleteAllItems();

    // 상위 폴더 항목은 항상 최상단에 고정
    if (m_strCurrentPath != L"/") {
        int nIdx = m_ListCtrl.InsertItem(0, L"..", 0);
        m_ListCtrl.SetItemText(nIdx, 2, L"<상위 폴더>");
    }

    CInternetSession session(_T("KpaxWebHardClient"));
    CFtpConnection* pFtpConn = NULL;

    try {
        pFtpConn = session.GetFtpConnection(SERVER_IP, _T("friend"), _T("1111"), 2121, TRUE);
        pFtpConn->Command(_T("OPTS UTF8 ON"));

        CString strSearchPath = m_strCurrentPath;
        if (strSearchPath.Right(1) != L"/") strSearchPath += L"/";
        strSearchPath += L"*.*";

        int utf8PathLen = WideCharToMultiByte(CP_UTF8, 0, strSearchPath, -1, NULL, 0, NULL, NULL);
        std::vector<char> utf8Path(utf8PathLen);
        WideCharToMultiByte(CP_UTF8, 0, strSearchPath, -1, utf8Path.data(), utf8PathLen, NULL, NULL);

        WIN32_FIND_DATAA fd;
        HINTERNET hFind = ::FtpFindFirstFileA((HINTERNET)*pFtpConn, utf8Path.data(), &fd, INTERNET_FLAG_RELOAD | INTERNET_FLAG_PASSIVE, 0);

        if (hFind != NULL) {
            // [정렬을 위한 임시 저장소]
            struct FileInfo { CString name; DWORD attr; };
            std::vector<FileInfo> folders;
            std::vector<FileInfo> files;

            BOOL bContinue = TRUE;
            while (bContinue) {
                if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) {
                    bContinue = ::InternetFindNextFileA(hFind, &fd);
                    continue;
                }

                // 한글 변환
                int nUniLen = MultiByteToWideChar(CP_UTF8, 0, fd.cFileName, -1, NULL, 0);
                CString strName;
                if (nUniLen > 0) {
                    std::vector<wchar_t> uniBuf(nUniLen);
                    MultiByteToWideChar(CP_UTF8, 0, fd.cFileName, -1, uniBuf.data(), nUniLen);
                    strName = uniBuf.data();
                }
                else { strName = (CString)fd.cFileName; }

                // 폴더와 파일을 각각 다른 바구니에 담기
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    folders.push_back({ strName, fd.dwFileAttributes });
                else
                    files.push_back({ strName, fd.dwFileAttributes });

                bContinue = ::InternetFindNextFileA(hFind, &fd);
            }
            ::InternetCloseHandle(hFind);

            // [추가] 가나다순으로 이름 정렬 (선택 사항)
            auto sortFunc = [](const FileInfo& a, const FileInfo& b) { return a.name < b.name; };
            std::sort(folders.begin(), folders.end(), sortFunc);
            std::sort(files.begin(), files.end(), sortFunc);

            // 1. 폴더 바구니 먼저 리스트에 쏟아붓기
            for (auto& f : folders) {
                int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), f.name, 0);
                m_ListCtrl.SetItemText(nIdx, 2, L"<폴더>");
            }

            // 2. 파일 바구니 그다음에 쏟아붓기
            for (auto& f : files) {
                int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), f.name, 1);
                m_ListCtrl.SetItemText(nIdx, 1, L"0%");
                m_ListCtrl.SetItemText(nIdx, 2, L"대기 중");
            }
        }
    }
    catch (CInternetException* pEx) { pEx->Delete(); }

    if (pFtpConn) { pFtpConn->Close(); delete pFtpConn; }
    session.Close();
    UpdateTotalStatus();
}


LRESULT Ckpaxkkk01Dlg::OnUpdateSpeed(WPARAM wp, LPARAM lp) {
    int speedData = LOWORD(wp); // 속도 (*10 상태)
    int eta = HIWORD(wp);       // 남은 시간 (초)
    int nIdx = (int)lp;

    double mbps = speedData / 10.0;
    CString strStatus;

    if (mbps < 0.1) {
        strStatus = L"연결 중...";
    }
    else {
        // [수정] 남은 시간을 분:초 형식으로 변환
        if (eta >= 60) {
            strStatus.Format(L"전송 중 (%.1f MB/s) - %d분 %d초 남음", mbps, eta / 60, eta % 60);
        }
        else {
            strStatus.Format(L"전송 중 (%.1f MB/s) - %d초 남음", mbps, eta);
        }
    }

    // 리스트 컨트롤의 '상태' 컬럼 업데이트
    m_ListCtrl.SetItemText(nIdx, 2, strStatus);

    return 0;
}


//void Ckpaxkkk01Dlg::OnBnClickedBtnDownloadSelected()
//{
//    CreateDirectory(L"C:\\Test", NULL);
//
//    int nCount = m_ListCtrl.GetItemCount();
//    int nActivated = 0;
//
//    for (int i = 0; i < nCount; i++)
//    {
//        // 체크박스 또는 선택된 항목 확인
//        if (m_ListCtrl.GetCheck(i) || (m_ListCtrl.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED))
//        {
//            nActivated++;
//            CString strFileName = m_ListCtrl.GetItemText(i, 0);
//
//            // 메모리 에러 방지를 위해 새로 할당
//            DownloadRequest* pReq = new DownloadRequest();
//            pReq->hMainWnd = this->GetSafeHwnd();
//            pReq->nItemIndex = i;
//
//            // 1. Edit Control에 적힌 현재 경로를 가져옵니다.
//            CString strSelectedPath;
//            GetDlgItemText(IDC_EDIT_PATH, strSelectedPath);
//
//            // 2. 경로 끝에 역슬래시(\)가 있는지 확인하고 보정합니다.
//            if (strSelectedPath.Right(1) != L"\\") {
//                strSelectedPath += L"\\";
//            }
//
//            // 3. 사용자가 선택한 폴더가 실제로 존재하는지 확인하고, 없으면 생성합니다.
//            if (!strSelectedPath.IsEmpty()) {
//                CreateDirectory(strSelectedPath, NULL);
//            }
//
//            // 4. [수정 완료] 중복 선언 제거 및 경로 설정
//            CString strFullTarget = strSelectedPath + strFileName;
//
//            pReq->source = (LPCTSTR)strFileName;   // FTP 서버는 파일명만 필요함
//            pReq->target = (LPCTSTR)strFullTarget; // 내 컴퓨터에 저장될 최종 경로
//
//            pReq->nTotalBytes = 0;
//            pReq->nLastBytes = 0;
//            pReq->nLastPercent = -1;
//            pReq->dwLastTick = GetTickCount();
//            pReq->dwSpeedTick = GetTickCount();
//
//            m_ListCtrl.SetItemText(i, 2, L"전송 시작 중...");
//
//            // FTP 전용 스레드 호출
//            AfxBeginThread(FtpDownloadThreadProc, pReq);
//        }
//    }
//
//    if (nActivated == 0) AfxMessageBox(L"다운로드할 파일을 선택해주세요.");
//}

void Ckpaxkkk01Dlg::OnBnClickedBtnDownloadSelected()
{
    int nCount = m_ListCtrl.GetItemCount();
    int nActivated = 0;

    // 현재 설정된 로컬 다운로드 경로 가져오기
    CString strLocalBasePath;
    GetDlgItemText(IDC_EDIT_PATH, strLocalBasePath);
    if (strLocalBasePath.Right(1) != L"\\") strLocalBasePath += L"\\";

    for (int i = 0; i < nCount; i++)
    {
        if (m_ListCtrl.GetCheck(i) || (m_ListCtrl.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED))
        {
            CString strFileName = m_ListCtrl.GetItemText(i, 0);
            CString strStatus = m_ListCtrl.GetItemText(i, 2);

            // 폴더는 다운로드 대상에서 제외 (필요 시 로직 추가 가능)
            if (strStatus == L"<폴더>") continue;

            nActivated++;

            DownloadRequest* pReq = new DownloadRequest();
            pReq->hMainWnd = this->GetSafeHwnd();
            pReq->nItemIndex = i;

            // [핵심 수정] 서버의 전체 경로를 생성합니다.
            CString strServerFullPath = m_strCurrentPath;
            if (strServerFullPath.Right(1) != L"/") strServerFullPath += L"/";
            strServerFullPath += strFileName; // 예: /movies/inception.mp4

            // [수정] source에 파일명만 넣는 게 아니라 전체 경로를 넣습니다.
            pReq->source = (LPCTSTR)strServerFullPath;
            pReq->target = (LPCTSTR)(strLocalBasePath + strFileName);

            pReq->nTotalBytes = 0;
            pReq->nLastBytes = 0;
            pReq->nLastPercent = -1;
            pReq->dwLastTick = GetTickCount();
            pReq->dwSpeedTick = GetTickCount();

            m_ListCtrl.SetItemText(i, 2, L"전송 시작 중...");

            // FTP 전용 스레드 호출
            AfxBeginThread(FtpDownloadThreadProc, pReq);
        }
    }

    if (nActivated == 0) AfxMessageBox(L"다운로드할 파일을 선택해주세요.");
}


void Ckpaxkkk01Dlg::OnBnClickedBtnSelectAll()
{
    // 1. 현재 리스트의 전체 항목 개수를 가져옵니다.
    int nCount = m_ListCtrl.GetItemCount();
    if (nCount <= 0) return;

    // 2. 현재 상태의 반대로 바꿉니다. (꺼져있으면 켜고, 켜져있으면 끄기)
    m_bAllSelected = !m_bAllSelected;

    // 3. 루프를 돌며 모든 항목의 체크박스 상태를 변경합니다.
    for (int i = 0; i < nCount; i++)
    {
        m_ListCtrl.SetCheck(i, m_bAllSelected);
    }

    // 4. (선택 사항) 버튼 텍스트를 상태에 맞춰 바꿔주면 더 친절합니다.
    if (m_bAllSelected)
        SetDlgItemText(IDC_BTN_SELECT_ALL, L"전체 해제");
    else
        SetDlgItemText(IDC_BTN_SELECT_ALL, L"전체 선택");
}
void Ckpaxkkk01Dlg::OnBnClickedBtnBrowse()
{
    // 최신 스타일의 폴더 선택 대화상자
    CFolderPickerDialog dlg(m_strDownloadPath, OFN_FILEMUSTEXIST, this);

    if (dlg.DoModal() == IDOK)
    {
        // 선택한 경로 가져오기
        m_strDownloadPath = dlg.GetPathName();

        // Edit Control에 경로 표시
        SetDlgItemText(IDC_EDIT_PATH, m_strDownloadPath);
    }
}


//리스트 "마우스 선택" 또는 "체크박스" 둘 중 하나라도 되면 삭제
void Ckpaxkkk01Dlg::OnBnClickedBtnDeleteSelected()
{
    int nItemCount = m_ListCtrl.GetItemCount();
    BOOL bAny = FALSE;

    // 하나라도 조건에 맞는지 확인
    for (int i = 0; i < nItemCount; i++) {
        if (m_ListCtrl.GetCheck(i) || (m_ListCtrl.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED)) {
            bAny = TRUE;
            break;
        }
    }

    if (!bAny) {
        AfxMessageBox(L"삭제할 항목을 체크하거나 클릭해 주세요.");
        return;
    }

    if (AfxMessageBox(L"선택한 항목을 삭제하시겠습니까?", MB_YESNO) == IDYES) {
        for (int i = nItemCount - 1; i >= 0; i--) {
            // 체크박스가 켜져 있거나, 마우스로 선택되었거나
            if (m_ListCtrl.GetCheck(i) || (m_ListCtrl.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED)) {
                m_ListCtrl.DeleteItem(i);
            }
        }
        UpdateTotalStatus();
    }
}

// kpaxkkk01Dlg.cpp 파일 맨 아래쪽
// 함수의 소속(Scope)을 메인 대화상자로 명시합니다.
void Ckpaxkkk01Dlg::OnNMDblclkListDownload(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    int nIdx = pNMIA->iItem;

    if (nIdx != -1) {
        CString strName = m_ListCtrl.GetItemText(nIdx, 0);
        CString strStatus = m_ListCtrl.GetItemText(nIdx, 2);

        // [추가] 상위 폴더 이동 로직
        if (strName == L"..") {
            // 마지막 '/' 위치를 찾아서 그 앞까지만 남김
            int nPos = m_strCurrentPath.ReverseFind(L'/');
            if (nPos <= 0) m_strCurrentPath = L"/";
            else m_strCurrentPath = m_strCurrentPath.Left(nPos);

            OnBnClickedBtnStart(); // 목록 새로고침
        }
        // 일반 폴더 진입 로직
        else if (strStatus == L"<폴더>") {
            if (m_strCurrentPath.Right(1) != L"/") m_strCurrentPath += L"/";
            m_strCurrentPath += strName;

            OnBnClickedBtnStart();
        }
    }
    *pResult = 0;
}


