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
END_MESSAGE_MAP()


// [추가] 실제 업로드 파일 복사를 수행하는 워커 스레드 함수
UINT UploadThreadProc(LPVOID pParam) {
    DownloadRequest* pReq = (DownloadRequest*)pParam;
    if (pReq == NULL) return 0;

    Ckpaxkkk01Dlg* pMain = (Ckpaxkkk01Dlg*)CWnd::FromHandle(pReq->hMainWnd);

    // 1. 세마포어 대기
    WaitForSingleObject(pMain->m_hSemaphore, INFINITE);

    // 대기 시간이 길었을 수 있으므로 통과 직후 시간과 전송량 초기화
    pReq->dwLastTick = GetTickCount();
    pReq->nLastBytes = 0;
    pReq->nLastPercent = -1;

    BOOL bResult = FALSE;
    int retryCount = 0;
    const int MAX_RETRY = 3;

    while (retryCount < MAX_RETRY) {
        bResult = CopyFileExW(
            pReq->source.c_str(), pReq->target.c_str(),
            [](LARGE_INTEGER total, LARGE_INTEGER trans, LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID data) -> DWORD {
                DownloadRequest* p = (DownloadRequest*)data;
                DWORD dwCurrentTick = GetTickCount();


                // 1. 타임아웃 체크 (p-> 멤버 사용)
                if (trans.QuadPart > p->nLastBytes) {
                    p->dwLastTick = dwCurrentTick;
                }
                if (dwCurrentTick - p->dwLastTick > 10000) return PROGRESS_CANCEL;

                // 2. 속도 계산 (1초 주기)
                if (dwCurrentTick - p->dwSpeedTick >= 1000) {
                    double diff = (double)(trans.QuadPart - p->nLastBytes);
                    double mbps = diff / (1024.0 * 1024.0);

                    // UI에 속도 전송
                    ::PostMessage(p->hMainWnd, WM_UPDATE_SPEED, (WPARAM)(mbps * 100), (LPARAM)p->nItemIndex);

                    p->dwSpeedTick = dwCurrentTick;
                    p->nLastBytes = trans.QuadPart;
                }

                // [핵심 수정] static을 쓰지 않고 p->nLastBytes를 사용합니다.
                if (trans.QuadPart > p->nLastBytes) {
                    p->dwLastTick = dwCurrentTick; // 데이터 전송이 확인되면 시간 갱신
                    p->nLastBytes = trans.QuadPart;
                }

                // 10초간 전송량 변화가 전혀 없으면 끊긴 것으로 간주하고 취소(재시도로 유도)
                if (dwCurrentTick - p->dwLastTick > 10000) {
                    return PROGRESS_CANCEL;
                }

                // UI 업데이트
                int percent = (int)((double)trans.QuadPart / (double)total.QuadPart * 100);
                if (percent > p->nLastPercent || percent == 100) {
                    p->nLastPercent = percent;
                    ::PostMessage(p->hMainWnd, WM_UPDATE_PROGRESS, (WPARAM)percent, (LPARAM)p->nItemIndex);
                }
                return PROGRESS_CONTINUE;
            },
            pReq, NULL, COPY_FILE_NO_BUFFERING
        );

        if (bResult) break; // 성공 시 루프 탈출

        // 실패 시 재시도 준비
        retryCount++;
        pReq->dwLastTick = GetTickCount(); // 재시도 전 시간 리셋
        pReq->nLastBytes = 0;              // 전송량 리셋
        Sleep(2000); // 네트워크 안정을 위해 2초 대기 후 다시 시도
    }

    // 결과 보고 및 세마포어 반납
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
    m_hSemaphore = CreateSemaphore(NULL, 3, 3, NULL);
    m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    m_ListCtrl.InsertColumn(0, L"파일명", LVCFMT_LEFT, 180);
    m_ListCtrl.InsertColumn(1, L"진행률", LVCFMT_CENTER, 100);
    m_ListCtrl.InsertColumn(2, L"상태", LVCFMT_LEFT, 120);

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
    int nCount = m_ListCtrl.GetItemCount();
    if (nCount == 0)
    {
        AfxMessageBox(L"전송할 파일이 목록에 없습니다.");
        return;
    }

    // 리스트의 모든 항목을 확인하며 대기 중인 파일을 전송 시작
    for (int i = 0; i < nCount; i++)
    {
        CString strStatus = m_ListCtrl.GetItemText(i, 2);

        // 이미 완료되었거나 진행 중인 파일은 건너뛰고, "연결 중..." 또는 "대기 중" 상태인 것만 시작
        if (strStatus == L"연결 중..." || strStatus == L"대기 중")
        {
            m_ListCtrl.SetItemText(i, 2, L"전송 중...");

            // 스레드에 전달할 데이터 설정
            DownloadRequest* pReq = new DownloadRequest();
            pReq->hMainWnd = this->GetSafeHwnd();
            pReq->nItemIndex = i;
            pReq->nLastPercent = -1;
            pReq->dwLastTick = GetTickCount();

            // 리스트에서 파일명과 경로 정보를 가져와서 설정 (사용자 환경에 맞게 조정 필요)
            CString strFileName = m_ListCtrl.GetItemText(i, 0);
            //pReq->source = L"\\\\Server\\Public\\" + std::wstring((LPCTSTR)strFileName); // 예시 서버 경로
            pReq->target = L"C:\\Test\\" + std::wstring((LPCTSTR)strFileName);           // 예시 저장 경로

            // 워커 스레드 생성 및 시작
            AfxBeginThread(DownloadThreadProc, pReq);
        }
    }

    UpdateTotalStatus();
}

LRESULT Ckpaxkkk01Dlg::OnUpdateSpeed(WPARAM wp, LPARAM lp) {
    double mbps = (double)wp / 100.0; // 다시 소수점으로 복원
    int nIdx = (int)lp;

    CString strSpeed;
    if (mbps < 0.1) strSpeed = L"준비 중...";
    else strSpeed.Format(L"전송 중 (%.2f MB/s)", mbps);

    // 2번 컬럼(상태)에 속도 표시
    m_ListCtrl.SetItemText(nIdx, 2, strSpeed);
    return 0;
}