//encoding=UTF-8
#define _WIN32_WINNT 0x0600 //支持vista及以上系统api
#define ACE_NAME      L"SGuard64.exe"
// #define ACE_NAME      L"notepad.exe"
#define SCRIPT_NAME   L"ACEDisappear.cmd"
#define SCANNING_TIME 1*1000


#include <windows.h>
#include <tlhelp32.h>//进程快照相关的头文件
#include <shlwapi.h> //拼接路径使用的头文件
#include <stdio.h>
/*执行脚本使用*/
#include <shellapi.h>

/*---------- 自定义消息和消息ID ----------*/
#define WM_TRAY      (WM_USER + 1)
#define ID_TRAY_EXIT 2001

/*---------- 全局变量 ----------*/
NOTIFYICONDATAW g_nid = {0}; //托盘图标结构
HMENU           g_hMenu = NULL;//托盘菜单句柄
HANDLE          g_hThread = NULL;//扫描线程句柄
BOOL            g_bRun = TRUE;  //线程退出标记

/*---------- 函数声明 ----------*/
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CreateTrayIcon(HWND);
void RemoveTrayIcon(void);
void ShowTrayMenu(HWND);
DWORD WINAPI MonitorThread(LPVOID); //监控线程函数

/*---------- 监控线程(原main的循环逻辑被移到此处) ----------*/
DWORD WINAPI MonitorThread(LPVOID)
{
    //获取脚本路径(注意路径处理)
    wchar_t self[MAX_PATH];//当前程序路径
    GetModuleFileNameW(NULL,self, MAX_PATH);//NULL表示当前模块,self表示返回的程序路径大小
    PathRemoveFileSpecW(self);//去掉文件名得到当前程序的目录
    const wchar_t* fileName = SCRIPT_NAME;
    wchar_t fullPath[MAX_PATH];
    PathCombineW(fullPath, self , fileName);//拼接完整路径fullPath

    const wchar_t *TargetExe = ACE_NAME;//要监视的进程
    const wchar_t *OpenFile = fullPath;//要执行的脚本
    //为了使用相对路径需要拼接当前程序路径找到同目录的脚本路径
    printf("\nScript Location:\n");
    wprintf(fullPath);

    while (g_bRun)
    {
        //创建进程快照
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
        //第二个参数0表示不指定系统进程
        if(hSnap == INVALID_HANDLE_VALUE){
            //如果快照失败之后
            printf("CreateToolhelp32Snapshot failed");
            continue; //继续下一次
        }
        PROCESSENTRY32W pe;//进程信息结构体
        pe.dwSize  = sizeof(pe);
        if(!Process32FirstW(hSnap,&pe)){
            //获取第一个进程的时候
            printf("Process32FirstW failed. Cloud not get the first Process.");
            CloseHandle(hSnap);
            continue; //继续下一次
        }

        //遍历进程列表是否找到脚本
        do{
            if(_wcsicmp(pe.szExeFile,  TargetExe) == 0){
                //第二个参数使用runas表示以管理员权限执行脚本,open表示以普通权限执行
                HINSTANCE hResult = ShellExecuteW(NULL, L"runas", OpenFile, NULL, self, SW_SHOWNORMAL);
                if ((INT_PTR)hResult <= 32) {
                    printf("\nShellExecute failed with error code: %d\n", (INT_PTR)hResult);
                } else {
                    printf("\nDetected The Process ACE-Guard Client.exe\n");
                }
                break;
            }
            //szExeFile是进程的名称是宽字符要用%ls打印
        }while(Process32NextW(hSnap,&pe));

        CloseHandle(hSnap);
        Sleep(SCANNING_TIME); //等待时间,现在是1秒
    }
    return 0;
}

/*消息处理函数主要响应托盘菜单的消息*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
        CreateTrayIcon(hWnd);//创建托盘
        //创建监控线程(原main的循环)
        g_hThread = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);
        return 0;

    case WM_TRAY: //托盘消息
        if (lp == WM_RBUTTONUP) //右键弹出菜单
        {
            SetForegroundWindow(hWnd);//防止菜单消失
            ShowTrayMenu(hWnd);
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wp) == ID_TRAY_EXIT)
        {
            g_bRun = FALSE;        //通知线程退出
            if (g_hThread) WaitForSingleObject(g_hThread, 3000);
            PostQuitMessage(0);    //退出消息循环
            return 0;
        }
        break;

    case WM_DESTROY:
    case WM_CLOSE:
        RemoveTrayIcon(); //删除托盘
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wp, lp);
}

/*程序最小化到托盘,创建托盘图标*/
void CreateTrayIcon(HWND hWnd)
{
    g_nid.cbSize  = sizeof(g_nid);
    g_nid.hWnd   = hWnd;
    g_nid.uID    = 1234;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAY;
    g_nid.hIcon  = (HICON)LoadImageW(NULL, IDI_INFORMATION, IMAGE_ICON, 0, 0, LR_SHARED);
    /* 替换 CreateTrayIcon 中的 wcscpy_s 调用 */
    wcscpy_s(g_nid.szTip,  ARRAYSIZE(g_nid.szTip),  L"ACE-Guard Watching");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

/*程序退出时删除托盘*/
void RemoveTrayIcon(void)
{
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

/*右键弹出显示托盘菜单*/
void ShowTrayMenu(HWND hWnd)
{
    if (!g_hMenu)
    {
        g_hMenu = CreatePopupMenu();
        AppendMenuW(g_hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    }
    POINT pt;
    GetCursorPos(&pt);
    TrackPopupMenu(g_hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
}

/*真正main函数WinMain主要用于创建消息循环*/
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int)
{
    // 1. 注册窗口类
    const wchar_t CLASS_NAME[] = L"ACE_TrayCls";
    WNDCLASSEXW wc = {0};
    wc.cbSize         = sizeof(wc);
    wc.lpfnWndProc    = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName  = CLASS_NAME;
    RegisterClassExW(&wc);

    // 2. 创建消息-only窗口，不可见窗口
    HWND hWnd = CreateWindowExW(0, CLASS_NAME, L"ACE_TrayWnd", 0,
                                0, 0, 0, 0, HWND_MESSAGE, NULL, hInst, NULL);

    // 3. 消息循环
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}