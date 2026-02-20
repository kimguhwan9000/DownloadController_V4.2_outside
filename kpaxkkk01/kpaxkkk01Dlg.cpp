#include "pch.h"
#include "framework.h"
#include "kpaxkkk01.h"
#include "kpaxkkk01Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
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
};
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
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
    ON_NOTIFY(NM_RCLICK, IDC_LIST_DOWNLOAD, &Ckpaxkkk01Dlg::OnNMRClickListDownload)
    ON_COMMAND(ID_MENU_DELETE, &Ckpaxkkk01Dlg::OnMenuDelete)
    ON_BN_CLICKED(IDC_BTN_CLEAR_FINISHED, &Ckpaxkkk01Dlg::OnBnClickedBtnClearFinished)
    ON_BN_CLICKED(IDC_CHK_AUTO_CLEAR, &Ckpaxkkk01Dlg::OnBnClickedChkAutoClear)
    ON_MESSAGE(WM_UPDATE_SPEED, &Ckpaxkkk01Dlg::OnUpdateSpeed)
    ON_BN_CLICKED(IDC_BTN_DOWNLOAD_SELECTED, &Ckpaxkkk01Dlg::OnBnClickedBtnDownloadSelected)
    ON_BN_CLICKED(IDC_BTN_SELECT_ALL, &Ckpaxkkk01Dlg::OnBnClickedBtnSelectAll)
    ON_BN_CLICKED(IDC_BTN_BROWSE, &Ckpaxkkk01Dlg::OnBnClickedBtnBrowse)
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

// [추가] 실제 다운로드 파일 복사를 수행하는 워커 스레드 함수
UINT DownloadThreadProc(LPVOID pParam) {
    // 다운로드 로직은 업로드와 거의 동일하므로 UploadThreadProc를 재사용하거나 
    // 동일한 구조의 함수를 하나 더 둡니다.
    return UploadThreadProc(pParam);
}



BOOL Ckpaxkkk01Dlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // 1. 초기 설정
    //m_hSemaphore = CreateSemaphore(NULL, 10, 10, NULL);
    m_hSemaphore = CreateSemaphore(NULL, 2, 2, NULL);

    // LVS_EX_CHECKBOXES 스타일 추가
    m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);

    // LVS_EX_CHECKBOXES 가 누락되면 GetCheck가 절대 작동하지 않습니다.
    DWORD dwExStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER;
    m_ListCtrl.SetExtendedStyle(dwExStyle);

    m_ListCtrl.InsertColumn(0, L"파일명", LVCFMT_LEFT, 180);
    m_ListCtrl.InsertColumn(1, L"진행률", LVCFMT_CENTER, 100);
    m_ListCtrl.InsertColumn(2, L"상태", LVCFMT_LEFT, 220);

    DragAcceptFiles(TRUE);
    ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
    ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);

    // 2. 상태바 생성
    static UINT indicators[] = { ID_SEPARATOR };
    if (m_StatusBar.Create(this)) {
        m_StatusBar.SetIndicators(indicators, 1);
        RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
    }

    UpdateTotalStatus();



    // 초기 기본 경로 설정
    m_strDownloadPath = L"C:\\Test";
    SetDlgItemText(IDC_EDIT_PATH, m_strDownloadPath);
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

// 업로드 버튼 클릭
void Ckpaxkkk01Dlg::OnBnClickedBtnUpload() {
    CFileDialog fileDlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST, L"모든 파일 (*.*)|*.*||");
    if (fileDlg.DoModal() == IDOK) {
        CString strLocalPath = fileDlg.GetPathName();
        CString strFileName = fileDlg.GetFileName();

        if (!CreateDirectory(L"C:\\Test", NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return;

        std::wstring strServerPath = L"C:\\Test\\" + std::wstring((LPCTSTR)strFileName);

        int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), strFileName);
        m_ListCtrl.SetItemText(nIdx, 1, L"0%");
        m_ListCtrl.SetItemText(nIdx, 2, L"업로드 중...");

        DownloadRequest* pReq = new DownloadRequest();
        pReq->hMainWnd = this->GetSafeHwnd();
        pReq->source = (LPCTSTR)strLocalPath;
        pReq->target = strServerPath;
        pReq->nItemIndex = nIdx;
        pReq->nLastPercent = -1;
        pReq->dwLastTick = GetTickCount();

        AfxBeginThread(UploadThreadProc, pReq);
        UpdateTotalStatus();
    }
}

// 드래그 앤 드롭
void Ckpaxkkk01Dlg::OnDropFiles(HDROP hDropInfo) {
    UINT nFiles = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
    for (UINT i = 0; i < nFiles; i++) {
        TCHAR szFilePath[MAX_PATH];
        if (DragQueryFile(hDropInfo, i, szFilePath, MAX_PATH)) {
            CString strLocalPath = szFilePath;
            CString strFileName = strLocalPath.Mid(strLocalPath.ReverseFind('\\') + 1);
            std::wstring strServerPath = L"C:\\Test\\" + std::wstring((LPCTSTR)strFileName);

            int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), strFileName);
            m_ListCtrl.SetItemText(nIdx, 1, L"0%");
            m_ListCtrl.SetItemText(nIdx, 2, L"연결 중...");

            DownloadRequest* pReq = new DownloadRequest();
            pReq->hMainWnd = this->GetSafeHwnd();
            pReq->source = (LPCTSTR)strLocalPath;
            pReq->target = strServerPath;
            pReq->nItemIndex = nIdx;
            pReq->nLastPercent = -1;
            pReq->dwLastTick = GetTickCount();

            AfxBeginThread(UploadThreadProc, pReq);
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


// '시작' 버튼을 눌렀을 때 실행되는 함수
void Ckpaxkkk01Dlg::OnBnClickedBtnStart()
{
    m_ListCtrl.DeleteAllItems(); // 기존 목록 청소

    CFileFind finder;
    // 서버의 공유 폴더 경로 (실제 경로로 수정하세요)
    //CString strServerPath = L"C:\\Test\\*.*";
    CString strServerPath = L"H:\\PC2 2TB HDD 자료\\영화,드라마,애니메이션\\마쇼파일\\애니메이션, 전대물\\하울의움직이는성 (2004)\\*.*";
    //H:\PC2 2TB HDD 자료\영화,드라마,애니메이션\마쇼파일\애니메이션, 전대물\나루토 (2002)

    BOOL bWorking = finder.FindFile(strServerPath);
    while (bWorking) {
        bWorking = finder.FindNextFile();

        if (finder.IsDots()) continue; // . 이나 .. 폴더 제외
        if (finder.IsDirectory()) continue; // 일단 파일만 표시

        // 리스트에 파일명 추가
        int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), finder.GetFileName());
        m_ListCtrl.SetItemText(nIdx, 1, L"0%");
        m_ListCtrl.SetItemText(nIdx, 2, L"대기 중 (체크 후 다운로드)");
    }
    finder.Close();

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


void Ckpaxkkk01Dlg::OnBnClickedBtnDownloadSelected()
{
    CreateDirectory(L"C:\\Test", NULL);

    int nCount = m_ListCtrl.GetItemCount();
    int nActivated = 0;

    for (int i = 0; i < nCount; i++)
    {
        // 체크박스 또는 선택된 항목 확인
        if (m_ListCtrl.GetCheck(i) || (m_ListCtrl.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED))
        {
            nActivated++;
            CString strFileName = m_ListCtrl.GetItemText(i, 0);

            // 메모리 에러 방지를 위해 새로 할당
            DownloadRequest* pReq = new DownloadRequest();
            pReq->hMainWnd = this->GetSafeHwnd();
            pReq->nItemIndex = i;

            // 경로 문자열 생성
            CString strBase = L"H:\\PC2 2TB HDD 자료\\영화,드라마,애니메이션\\마쇼파일\\애니메이션, 전대물\\하울의움직이는성 (2004)\\";
            CString strFullSource = strBase + strFileName;
            //CString strFullTarget = L"C:\\Test\\" + strFileName;


            // 1. Edit Control에 적힌 현재 경로를 가져옵니다.
            CString strSelectedPath;
            GetDlgItemText(IDC_EDIT_PATH, strSelectedPath); // ID를 직접 지정해서 값을 가져옴

            // 2. 경로 끝에 역슬래시(\)가 있는지 확인하고 보정합니다. (매우 중요!)
            if (strSelectedPath.Right(1) != L"\\") {
                strSelectedPath += L"\\";
            }

            // 3. 사용자가 선택한 폴더가 실제로 존재하는지 확인하고, 없으면 생성합니다.
            if (!strSelectedPath.IsEmpty()) {
                CreateDirectory(strSelectedPath, NULL);
            }

            // ... 루프(for문) 안에서 ...

            // 4. [변경 핵심] 고정 경로 대신 strSelectedPath 변수를 사용합니다.
            CString strFullTarget = strSelectedPath + strFileName;

            // [핵심] 포인터가 아니라 실제 값을 복사해서 스레드에 넘김
            pReq->source = (LPCTSTR)strFullSource;
            pReq->target = (LPCTSTR)strFullTarget;

            pReq->nTotalBytes = 0;
            pReq->nLastBytes = 0;
            pReq->nLastPercent = -1;
            pReq->dwLastTick = GetTickCount();
            pReq->dwSpeedTick = GetTickCount();

            m_ListCtrl.SetItemText(i, 2, L"전송 시작 중...");

            // [주의] 딱 한 번만 호출하세요! 
            // 이전에 아래쪽에 다른 AfxBeginThread가 남아있다면 반드시 지우세요.
            AfxBeginThread(UploadThreadProc, pReq);
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
