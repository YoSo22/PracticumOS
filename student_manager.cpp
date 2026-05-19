#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define ID_LISTBOX 1001
#define ID_BTN_ADD 1002
#define ID_BTN_EDIT 1003
#define ID_BTN_DELETE 1004
#define ID_BTN_VIEW_TABLE 1005
#define ID_BTN_VIEW_CARD 1006
#define ID_BTN_SORT_ALPHA_ASC 1007
#define ID_BTN_SORT_ALPHA_DESC 1008
#define ID_BTN_SORT_AVG_ASC 1009
#define ID_BTN_SORT_AVG_DESC 1010
#define ID_BTN_SAVE 1011
#define ID_BTN_LOAD 1012
#define ID_STATIC_INFO 1013

#define MAX_STUDENTS 1000
#define CSV_FILE L"students.csv"

typedef struct {
    wchar_t fio[256];
    wchar_t recordBook[64];
    int grades[5];
    double avgGrade;
} Student;

Student students[MAX_STUDENTS];
int studentCount = 0;
int viewMode = 0; // 0 - табличный, 1 - карточный
int currentCardIndex = 0;

HWND hMainWnd;
HWND hListBox;
HWND hStatusBar;
HFONT hFont;

void CalculateAvgGrade(Student* s) {
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += s->grades[i];
    }
    s->avgGrade = (double)sum / 5.0;
}

// Рекурсивная быстрая сортировка (для алфавита)
int PartitionRecursive(Student arr[], int low, int high, int desc) {
    wchar_t pivot[256];
    wcscpy(pivot, arr[high].fio);
    int i = low - 1;
    
    for (int j = low; j <= high - 1; j++) {
        int cmp = wcscmp(arr[j].fio, pivot);
        if ((desc && cmp > 0) || (!desc && cmp <= 0)) {
            i++;
            Student temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }
    Student temp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;
    return i + 1;
}

void QuickSortRecursive(Student arr[], int low, int high, int desc) {
    if (low < high) {
        int pi = PartitionRecursive(arr, low, high, desc);
        QuickSortRecursive(arr, low, pi - 1, desc);
        QuickSortRecursive(arr, pi + 1, high, desc);
    }
}

// Нерекурсивная быстрая сортировка (для среднего балла)
typedef struct {
    int low;
    int high;
} StackItem;

void QuickSortNonRecursive(Student arr[], int low, int high, int desc) {
    if (low >= high) return;
    
    StackItem stack[MAX_STUDENTS];
    int top = -1;
    
    stack[++top].low = low;
    stack[top].high = high;
    
    while (top >= 0) {
        int l = stack[top].low;
        int h = stack[top].high;
        top--;
        
        int pivotIdx = h;
        double pivotVal = arr[pivotIdx].avgGrade;
        int i = l - 1;
        
        for (int j = l; j <= h - 1; j++) {
            int condition = desc ? (arr[j].avgGrade > pivotVal) : (arr[j].avgGrade <= pivotVal);
            if (condition) {
                i++;
                Student temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
        Student temp = arr[i + 1];
        arr[i + 1] = arr[h];
        arr[h] = temp;
        int pi = i + 1;
        
        if (pi - 1 > l) {
            stack[++top].low = l;
            stack[top].high = pi - 1;
        }
        if (pi + 1 < h) {
            stack[++top].low = pi + 1;
            stack[top].high = h;
        }
    }
}

void SaveToCSV() {
    FILE* f = _wfopen(CSV_FILE, L"w, ccs=UTF-8");
    if (!f) {
        MessageBox(hMainWnd, L"Не удалось сохранить файл!", L"Ошибка", MB_ICONERROR);
        return;
    }
    
    for (int i = 0; i < studentCount; i++) {
        fwprintf(f, L"%s,%s,%d,%d,%d,%d,%d\n",
            students[i].fio,
            students[i].recordBook,
            students[i].grades[0],
            students[i].grades[1],
            students[i].grades[2],
            students[i].grades[3],
            students[i].grades[4]);
    }
    
    fclose(f);
}

void LoadFromCSV() {
    FILE* f = _wfopen(CSV_FILE, L"r, ccs=UTF-8");
    if (!f) {
        return;
    }
    
    studentCount = 0;
    wchar_t line[1024];
    
    while (fgetws(line, sizeof(line)/sizeof(wchar_t), f) && studentCount < MAX_STUDENTS) {
        size_t len = wcslen(line);
        if (len > 0 && line[len-1] == L'\n') line[len-1] = L'\0';
        if (len > 1 && line[len-2] == L'\r') line[len-2] = L'\0';
        
        wchar_t* token = wcstok(line, L",");
        if (!token) continue;
        wcscpy(students[studentCount].fio, token);
        
        token = wcstok(NULL, L",");
        if (!token) continue;
        wcscpy(students[studentCount].recordBook, token);
        
        for (int i = 0; i < 5; i++) {
            token = wcstok(NULL, L",");
            if (!token) break;
            students[studentCount].grades[i] = _wtoi(token);
        }
        
        CalculateAvgGrade(&students[studentCount]);
        studentCount++;
    }
    
    fclose(f);
}

INT_PTR CALLBACK StudentDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static int editIndex;
    static HWND hFio, hRecordBook, hGrades[5];
    
    switch (uMsg) {
        case WM_INITDIALOG:
            editIndex = (int)lParam;
            
            hFio = GetDlgItem(hwndDlg, 2001);
            hRecordBook = GetDlgItem(hwndDlg, 2002);
            for (int i = 0; i < 5; i++) {
                hGrades[i] = GetDlgItem(hwndDlg, 2003 + i);
            }
            
            if (editIndex >= 0 && editIndex < studentCount) {
                SetWindowText(hFio, students[editIndex].fio);
                SetWindowText(hRecordBook, students[editIndex].recordBook);
                
                wchar_t buf[16];
                for (int i = 0; i < 5; i++) {
                    swprintf(buf, 16, L"%d", students[editIndex].grades[i]);
                    SetWindowText(hGrades[i], buf);
                }
                SetWindowText(GetDlgItem(hwndDlg, IDOK), L"Изменить");
            } else {
                SetWindowText(GetDlgItem(hwndDlg, IDOK), L"Добавить");
            }
            return TRUE;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                wchar_t buf[256];
                
                GetWindowText(hFio, buf, 256);
                if (wcslen(buf) == 0) {
                    MessageBox(hwndDlg, L"Введите ФИО", L"Ошибка", MB_ICONWARNING);
                    return TRUE;
                }
                
                if (editIndex >= 0 && editIndex < studentCount) {
                    wcscpy(students[editIndex].fio, buf);
                } else {
                    if (studentCount >= MAX_STUDENTS) {
                        MessageBox(hwndDlg, L"Максимальное количество студентов достигнуто", L"Ошибка", MB_ICONERROR);
                        EndDialog(hwndDlg, IDCANCEL);
                        return TRUE;
                    }
                    wcscpy(students[studentCount].fio, buf);
                }
                
                GetWindowText(hRecordBook, buf, 64);
                if (wcslen(buf) == 0) {
                    MessageBox(hwndDlg, L"Введите номер зачетки", L"Ошибка", MB_ICONWARNING);
                    return TRUE;
                }
                
                if (editIndex >= 0 && editIndex < studentCount) {
                    wcscpy(students[editIndex].recordBook, buf);
                } else {
                    wcscpy(students[studentCount].recordBook, buf);
                }
                
                int valid = 1;
                if (editIndex >= 0 && editIndex < studentCount) {
                    for (int i = 0; i < 5; i++) {
                        GetWindowText(hGrades[i], buf, 16);
                        int grade = _wtoi(buf);
                        if (grade < 2 || grade > 5) {
                            valid = 0;
                            break;
                        }
                        students[editIndex].grades[i] = grade;
                    }
                    if (valid) CalculateAvgGrade(&students[editIndex]);
                } else {
                    for (int i = 0; i < 5; i++) {
                        GetWindowText(hGrades[i], buf, 16);
                        int grade = _wtoi(buf);
                        if (grade < 2 || grade > 5) {
                            valid = 0;
                            break;
                        }
                        students[studentCount].grades[i] = grade;
                    }
                    if (valid) {
                        CalculateAvgGrade(&students[studentCount]);
                        studentCount++;
                    }
                }
                
                if (!valid) {
                    MessageBox(hwndDlg, L"Оценки должны быть от 2 до 5", L"Ошибка", MB_ICONWARNING);
                    return TRUE;
                }
                
                EndDialog(hwndDlg, IDOK);
                return TRUE;
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwndDlg, IDCANCEL);
                return TRUE;
            }
            return TRUE;
    }
    return FALSE;
}

void UpdateListBox() {
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    
    if (viewMode == 0) {
        for (int i = 0; i < studentCount; i++) {
            wchar_t buf[512];
            swprintf(buf, 512, L"%-40s | %-15s | %.2f", 
                students[i].fio, students[i].recordBook, students[i].avgGrade);
            SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)buf);
        }
    } else {
        if (studentCount > 0 && currentCardIndex >= 0 && currentCardIndex < studentCount) {
            wchar_t buf[512];
            swprintf(buf, 512, 
                L"=== Карточка студента %d из %d ===\n\n"
                L"ФИО: %s\n"
                L"Номер зачетки: %s\n"
                L"Оценки: %d  %d  %d  %d  %d\n"
                L"Средний балл: %.2f",
                currentCardIndex + 1, studentCount,
                students[currentCardIndex].fio,
                students[currentCardIndex].recordBook,
                students[currentCardIndex].grades[0],
                students[currentCardIndex].grades[1],
                students[currentCardIndex].grades[2],
                students[currentCardIndex].grades[3],
                students[currentCardIndex].grades[4],
                students[currentCardIndex].avgGrade);
            SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)buf);
        } else if (studentCount == 0) {
            SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"Список пуст");
        }
    }
    
    wchar_t status[256];
    swprintf(status, 256, L"Студентов: %d | Режим: %s", studentCount, viewMode == 0 ? L"Таблица" : L"Карточки");
    SetWindowText(hStatusBar, status);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
            
            hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOINTEGRALHEIGHT,
                10, 10, 760, 300, hwnd, (HMENU)ID_LISTBOX, NULL, NULL);
            SendMessage(hListBox, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            HWND hBtn;
            int btnY = 320;
            int btnH = 30;
            int btnW = 110;
            int spacing = 5;
            
            hBtn = CreateWindow(L"BUTTON", L"Добавить (Ins)",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, btnY, btnW, btnH, hwnd, (HMENU)ID_BTN_ADD, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hBtn = CreateWindow(L"BUTTON", L"Изменить (Enter)",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10 + (btnW + spacing), btnY, btnW, btnH, hwnd, (HMENU)ID_BTN_EDIT, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hBtn = CreateWindow(L"BUTTON", L"Удалить (Del)",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10 + 2*(btnW + spacing), btnY, btnW, btnH, hwnd, (HMENU)ID_BTN_DELETE, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            btnY += btnH + spacing;
            
            hBtn = CreateWindow(L"BUTTON", L"Режим: Таблица",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, btnY, btnW, btnH, hwnd, (HMENU)ID_BTN_VIEW_TABLE, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hBtn = CreateWindow(L"BUTTON", L"Режим: Карточки",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10 + (btnW + spacing), btnY, btnW, btnH, hwnd, (HMENU)ID_BTN_VIEW_CARD, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hBtn = CreateWindow(L"BUTTON", L"Сохранить CSV",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10 + 2*(btnW + spacing), btnY, btnW, btnH, hwnd, (HMENU)ID_BTN_SAVE, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hBtn = CreateWindow(L"BUTTON", L"Загрузить CSV",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10 + 3*(btnW + spacing), btnY, btnW, btnH, hwnd, (HMENU)ID_BTN_LOAD, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            btnY += btnH + spacing + 10;
            
            CreateWindow(L"STATIC", L"Сортировка:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, btnY, 100, 20, hwnd, NULL, NULL, NULL);
            
            hBtn = CreateWindow(L"BUTTON", L"А-Я ^",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                100, btnY, 70, btnH, hwnd, (HMENU)ID_BTN_SORT_ALPHA_ASC, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hBtn = CreateWindow(L"BUTTON", L"Я-А v",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                175, btnY, 70, btnH, hwnd, (HMENU)ID_BTN_SORT_ALPHA_DESC, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hBtn = CreateWindow(L"BUTTON", L"Балл ^",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                250, btnY, 70, btnH, hwnd, (HMENU)ID_BTN_SORT_AVG_ASC, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hBtn = CreateWindow(L"BUTTON", L"Балл v",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                325, btnY, 70, btnH, hwnd, (HMENU)ID_BTN_SORT_AVG_DESC, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hStatusBar = CreateWindowEx(0, L"STATIC", L"Студентов: 0",
                WS_CHILD | WS_VISIBLE | WS_BORDER | SS_LEFT,
                10, 420, 760, 25, hwnd, (HMENU)ID_STATIC_INFO, NULL, NULL);
            SendMessage(hStatusBar, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            return 0;
        }
        
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BTN_ADD: {
                    INT_PTR result = DialogBoxParam(NULL, MAKEINTRESOURCE(100), hwnd, StudentDialogProc, (LPARAM)-1);
                    if (result == IDOK) {
                        UpdateListBox();
                        SaveToCSV();
                    }
                    break;
                }
                case ID_BTN_EDIT: {
                    int sel = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
                    if (sel == LB_ERR) {
                        MessageBox(hwnd, L"Выберите студента для редактирования", L"Предупреждение", MB_ICONWARNING);
                        break;
                    }
                    if (viewMode == 1) sel = currentCardIndex;
                    
                    INT_PTR result = DialogBoxParam(NULL, MAKEINTRESOURCE(100), hwnd, StudentDialogProc, (LPARAM)sel);
                    if (result == IDOK) {
                        UpdateListBox();
                        SaveToCSV();
                    }
                    break;
                }
                case ID_BTN_DELETE: {
                    int sel = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
                    if (sel == LB_ERR) {
                        MessageBox(hwnd, L"Выберите студента для удаления", L"Предупреждение", MB_ICONWARNING);
                        break;
                    }
                    if (viewMode == 1) sel = currentCardIndex;
                    
                    if (MessageBox(hwnd, L"Удалить этого студента?", L"Подтверждение", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        for (int i = sel; i < studentCount - 1; i++) {
                            students[i] = students[i + 1];
                        }
                        studentCount--;
                        if (viewMode == 1 && currentCardIndex >= studentCount) {
                            currentCardIndex = studentCount - 1;
                            if (currentCardIndex < 0) currentCardIndex = 0;
                        }
                        UpdateListBox();
                        SaveToCSV();
                    }
                    break;
                }
                case ID_BTN_VIEW_TABLE:
                    viewMode = 0;
                    UpdateListBox();
                    break;
                case ID_BTN_VIEW_CARD:
                    viewMode = 1;
                    currentCardIndex = 0;
                    UpdateListBox();
                    break;
                case ID_BTN_SORT_ALPHA_ASC:
                    QuickSortRecursive(students, 0, studentCount - 1, 0);
                    UpdateListBox();
                    SaveToCSV();
                    break;
                case ID_BTN_SORT_ALPHA_DESC:
                    QuickSortRecursive(students, 0, studentCount - 1, 1);
                    UpdateListBox();
                    SaveToCSV();
                    break;
                case ID_BTN_SORT_AVG_ASC:
                    QuickSortNonRecursive(students, 0, studentCount - 1, 0);
                    UpdateListBox();
                    SaveToCSV();
                    break;
                case ID_BTN_SORT_AVG_DESC:
                    QuickSortNonRecursive(students, 0, studentCount - 1, 1);
                    UpdateListBox();
                    SaveToCSV();
                    break;
                case ID_BTN_SAVE:
                    SaveToCSV();
                    break;
                case ID_BTN_LOAD:
                    LoadFromCSV();
                    UpdateListBox();
                    break;
            }
            return 0;
            
        case WM_KEYDOWN:
            switch (wParam) {
                case VK_INSERT:
                    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_BTN_ADD, 0), 0);
                    break;
                case VK_RETURN:
                    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_BTN_EDIT, 0), 0);
                    break;
                case VK_DELETE:
                    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_BTN_DELETE, 0), 0);
                    break;
                case VK_LEFT:
                    if (viewMode == 1 && currentCardIndex > 0) {
                        currentCardIndex--;
                        UpdateListBox();
                    }
                    break;
                case VK_RIGHT:
                    if (viewMode == 1 && currentCardIndex < studentCount - 1) {
                        currentCardIndex++;
                        UpdateListBox();
                    }
                    break;
            }
            return 0;
            
        case WM_DESTROY:
            SaveToCSV();
            DeleteObject(hFont);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    setlocale(LC_ALL, "Russian");
    
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"StudentManagerClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Не удалось зарегистрировать класс окна", L"Ошибка", MB_ICONERROR);
        return 0;
    }
    
    hMainWnd = CreateWindowEx(
        0,
        L"StudentManagerClass",
        L"Менеджер студентов (WinAPI)",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 500,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hMainWnd) {
        MessageBox(NULL, L"Не удалось создать окно", L"Ошибка", MB_ICONERROR);
        return 0;
    }
    
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    
    LoadFromCSV();
    UpdateListBox();
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
