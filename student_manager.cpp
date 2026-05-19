#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <algorithm>
#include <stack>
#include <sstream>
#include <iomanip>
#include <fstream>

#pragma comment(lib, "comctl32.lib")

// Структура данных о студенте
struct Student {
    std::wstring fio;        // ФИО
    int recordBookNumber;    // Номер зачетки
    int grades[5];           // 5 оценок
    
    double getAverage() const {
        int sum = 0;
        for (int i = 0; i < 5; i++) {
            sum += grades[i];
        }
        return static_cast<double>(sum) / 5.0;
    }
};

// Глобальные переменные
std::vector<Student> g_students;
HWND g_hMainWnd = nullptr;
HWND g_hListView = nullptr;
HWND g_hStatusBar = nullptr;
bool g_viewMode = true; // true - табличный, false - карточками
int g_sortOrder = 0;    // 0 - без сортировки, 1 - по убыванию ср.балла, 2 - по возрастанию ср.балла, 3 - по алфавиту
std::wstring g_dataFile = L"students.csv"; // Файл для хранения данных

// Идентификаторы элементов управления
#define ID_MENU_FILE_EXIT 1001
#define ID_MENU_FILE_ADD 1002
#define ID_MENU_FILE_EDIT 1003
#define ID_MENU_FILE_DELETE 1004
#define ID_MENU_VIEW_TABLE 1005
#define ID_MENU_VIEW_CARDS 1006
#define ID_MENU_SORT_DESC_AVG 1007
#define ID_MENU_SORT_ASC_AVG 1008
#define ID_MENU_SORT_ALPHA 1009
#define ID_MENU_SORT_NONE 1010
#define ID_DIALOG_STUDENT 2000
#define ID_EDIT_FIO 2001
#define ID_EDIT_RECORD 2002
#define ID_EDIT_GRADE1 2003
#define ID_EDIT_GRADE2 2004
#define ID_EDIT_GRADE3 2005
#define ID_EDIT_GRADE4 2006
#define ID_EDIT_GRADE5 2007
#define ID_BTN_OK 2008
#define ID_BTN_CANCEL 2009

// Объявления функций
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK StudentDlgProc(HWND, UINT, WPARAM, LPARAM);
void InitListView(HWND hWnd);
void UpdateListView();
void ShowStudentDialog(int index);
void SortByAverageNonRecursive(bool ascending);
void SortByNameRecursive(int left, int right);
void UpdateStatusBar();
void SaveDataToFile(const std::wstring& filename);
void LoadDataFromFile(const std::wstring& filename);

// Рекурсивная быстрая сортировка по ФИО
void SortByNameRecursive(int left, int right) {
    if (left >= right) return;
    
    int i = left, j = right;
    std::wstring pivot = g_students[(left + right) / 2].fio;
    
    while (i <= j) {
        while (g_students[i].fio < pivot) i++;
        while (g_students[j].fio > pivot) j--;
        if (i <= j) {
            std::swap(g_students[i], g_students[j]);
            i++;
            j--;
        }
    }
    
    SortByNameRecursive(left, j);
    SortByNameRecursive(i, right);
}

// Нерекурсивная быстрая сортировка по среднему баллу
void SortByAverageNonRecursive(bool ascending) {
    if (g_students.empty()) return;
    
    std::stack<std::pair<int, int>> stack;
    stack.push({0, static_cast<int>(g_students.size()) - 1});
    
    while (!stack.empty()) {
        auto [left, right] = stack.top();
        stack.pop();
        
        if (left >= right) continue;
        
        int i = left, j = right;
        double pivot = g_students[(left + right) / 2].getAverage();
        
        while (i <= j) {
            while ((ascending && g_students[i].getAverage() < pivot) || 
                   (!ascending && g_students[i].getAverage() > pivot)) i++;
            while ((ascending && g_students[j].getAverage() > pivot) || 
                   (!ascending && g_students[j].getAverage() < pivot)) j--;
            if (i <= j) {
                std::swap(g_students[i], g_students[j]);
                i++;
                j--;
            }
        }
        
        if (j > left) stack.push({left, j});
        if (i < right) stack.push({i, right});
    }
}

// Сохранение данных в CSV файл
void SaveDataToFile(const std::wstring& filename) {
    std::wofstream file(filename);
    if (!file.is_open()) {
        MessageBox(g_hMainWnd, L"Не удалось открыть файл для записи", L"Ошибка", MB_ICONERROR);
        return;
    }
    
    // Записываем BOM для UTF-8
    file << L"\xEF\xBB\xBF";
    
    for (const auto& student : g_students) {
        file << student.fio << L"," 
             << student.recordBookNumber << L","
             << student.grades[0] << L","
             << student.grades[1] << L","
             << student.grades[2] << L","
             << student.grades[3] << L","
             << student.grades[4] << L"\n";
    }
    
    file.close();
}

// Загрузка данных из CSV файла
void LoadDataFromFile(const std::wstring& filename) {
    std::wifstream file(filename);
    if (!file.is_open()) {
        return; // Файл не существует - это нормально при первом запуске
    }
    
    g_students.clear();
    
    std::wstring line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        Student s;
        std::wstringstream ss(line);
        std::wstring token;
        
        // ФИО
        if (std::getline(ss, token, L',')) {
            s.fio = token;
        }
        
        // Номер зачетки
        if (std::getline(ss, token, L',')) {
            s.recordBookNumber = _wtoi(token.c_str());
        }
        
        // Оценки
        for (int i = 0; i < 5; i++) {
            if (std::getline(ss, token, L',')) {
                s.grades[i] = _wtoi(token.c_str());
            } else {
                s.grades[i] = 0;
            }
        }
        
        g_students.push_back(s);
    }
    
    file.close();
}

// Инициализация ListView
void InitListView(HWND hWnd) {
    g_hListView = CreateWindowEx(
        0, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        0, 0, 0, 0,
        hWnd, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    // Добавляем колонки для табличного вида
    LVCOLUMN lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    
    lvc.iSubItem = 0;
    lvc.pszText = const_cast<LPWSTR>(L"№");
    lvc.cx = 40;
    ListView_InsertColumn(g_hListView, 0, &lvc);
    
    lvc.iSubItem = 1;
    lvc.pszText = const_cast<LPWSTR>(L"ФИО");
    lvc.cx = 200;
    ListView_InsertColumn(g_hListView, 1, &lvc);
    
    lvc.iSubItem = 2;
    lvc.pszText = const_cast<LPWSTR>(L"Зачетка");
    lvc.cx = 80;
    ListView_InsertColumn(g_hListView, 2, &lvc);
    
    lvc.iSubItem = 3;
    lvc.pszText = const_cast<LPWSTR>(L"Оценки");
    lvc.cx = 150;
    ListView_InsertColumn(g_hListView, 3, &lvc);
    
    lvc.iSubItem = 4;
    lvc.pszText = const_cast<LPWSTR>(L"Ср. балл");
    lvc.cx = 80;
    ListView_InsertColumn(g_hListView, 4, &lvc);
    
    ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}

// Обновление ListView
void UpdateListView() {
    ListView_DeleteAllItems(g_hListView);
    
    for (size_t i = 0; i < g_students.size(); i++) {
        LVITEM lvi = {};
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = static_cast<int>(i);
        lvi.iSubItem = 0;
        lvi.lParam = i;
        
        std::wstring num = std::to_wstring(i + 1);
        lvi.pszText = const_cast<LPWSTR>(num.c_str());
        ListView_InsertItem(g_hListView, &lvi);
        
        // ФИО
        ListView_SetItemText(g_hListView, static_cast<int>(i), 1, 
                            const_cast<LPWSTR>(g_students[i].fio.c_str()));
        
        // Номер зачетки
        std::wstring record = std::to_wstring(g_students[i].recordBookNumber);
        ListView_SetItemText(g_hListView, static_cast<int>(i), 2, 
                            const_cast<LPWSTR>(record.c_str()));
        
        // Оценки
        std::wstring grades;
        for (int j = 0; j < 5; j++) {
            if (j > 0) grades += L", ";
            grades += std::to_wstring(g_students[i].grades[j]);
        }
        ListView_SetItemText(g_hListView, static_cast<int>(i), 3, 
                            const_cast<LPWSTR>(grades.c_str()));
        
        // Средний балл
        std::wstringstream ss;
        ss << std::fixed << std::setprecision(2) << g_students[i].getAverage();
        ListView_SetItemText(g_hListView, static_cast<int>(i), 4, 
                            const_cast<LPWSTR>(ss.str().c_str()));
    }
    
    UpdateStatusBar();
}

// Обновление статусной строки
void UpdateStatusBar() {
    wchar_t buffer[256];
    swprintf(buffer, 256, L"Всего студентов: %zu", g_students.size());
    SendMessage(g_hStatusBar, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
}

// Диалог ввода/редактирования студента
INT_PTR CALLBACK StudentDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static int* pIndex = nullptr;
    
    switch (msg) {
    case WM_INITDIALOG: {
        pIndex = reinterpret_cast<int*>(lParam);
        
        HWND hFio = GetDlgItem(hDlg, ID_EDIT_FIO);
        HWND hRecord = GetDlgItem(hDlg, ID_EDIT_RECORD);
        HWND hGrade1 = GetDlgItem(hDlg, ID_EDIT_GRADE1);
        HWND hGrade2 = GetDlgItem(hDlg, ID_EDIT_GRADE2);
        HWND hGrade3 = GetDlgItem(hDlg, ID_EDIT_GRADE3);
        HWND hGrade4 = GetDlgItem(hDlg, ID_EDIT_GRADE4);
        HWND hGrade5 = GetDlgItem(hDlg, ID_EDIT_GRADE5);
        
        if (*pIndex >= 0 && *pIndex < static_cast<int>(g_students.size())) {
            SetWindowText(hDlg, L"Редактирование студента");
            Student& s = g_students[*pIndex];
            
            SetWindowText(hFio, s.fio.c_str());
            
            wchar_t buf[32];
            swprintf(buf, 32, L"%d", s.recordBookNumber);
            SetWindowText(hRecord, buf);
            
            for (int i = 0; i < 5; i++) {
                swprintf(buf, 32, L"%d", s.grades[i]);
                SetWindowText(GetDlgItem(hDlg, ID_EDIT_GRADE1 + i), buf);
            }
        } else {
            SetWindowText(hDlg, L"Добавление студента");
        }
        
        return TRUE;
    }
    
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_OK) {
            wchar_t buf[256];
            
            GetWindowText(GetDlgItem(hDlg, ID_EDIT_FIO), buf, 256);
            std::wstring fio = buf;
            
            GetWindowText(GetDlgItem(hDlg, ID_EDIT_RECORD), buf, 32);
            int record = _wtoi(buf);
            
            int grades[5];
            for (int i = 0; i < 5; i++) {
                GetWindowText(GetDlgItem(hDlg, ID_EDIT_GRADE1 + i), buf, 32);
                grades[i] = _wtoi(buf);
                if (grades[i] < 2 || grades[i] > 5) {
                    MessageBox(hDlg, L"Оценки должны быть от 2 до 5", L"Ошибка", MB_ICONERROR);
                    return TRUE;
                }
            }
            
            if (fio.empty()) {
                MessageBox(hDlg, L"Введите ФИО", L"Ошибка", MB_ICONERROR);
                return TRUE;
            }
            
            if (*pIndex >= 0 && *pIndex < static_cast<int>(g_students.size())) {
                // Редактирование
                g_students[*pIndex].fio = fio;
                g_students[*pIndex].recordBookNumber = record;
                for (int i = 0; i < 5; i++) {
                    g_students[*pIndex].grades[i] = grades[i];
                }
            } else {
                // Добавление
                Student s;
                s.fio = fio;
                s.recordBookNumber = record;
                for (int i = 0; i < 5; i++) {
                    s.grades[i] = grades[i];
                }
                g_students.push_back(s);
                *pIndex = static_cast<int>(g_students.size()) - 1;
            }
            
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        else if (LOWORD(wParam) == ID_BTN_CANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
        
    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    
    return FALSE;
}

// Показ диалога студента
void ShowStudentDialog(int index) {
    INT_PTR result = DialogBoxParam(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCE(ID_DIALOG_STUDENT),
        g_hMainWnd,
        StudentDlgProc,
        reinterpret_cast<LPARAM>(&index)
    );
    
    if (result == IDOK) {
        SaveDataToFile(g_dataFile); // Сохраняем после добавления/изменения
        UpdateListView();
    }
}

// Создание меню
HMENU CreateMainMenu() {
    HMENU hMenu = CreateMenu();
    
    HMENU hFile = CreatePopupMenu();
    AppendMenu(hFile, MF_STRING, ID_MENU_FILE_ADD, L"&Добавить\tIns");
    AppendMenu(hFile, MF_STRING, ID_MENU_FILE_EDIT, L"&Изменить\tEnter");
    AppendMenu(hFile, MF_STRING, ID_MENU_FILE_DELETE, L"&Удалить\tDel");
    AppendMenu(hFile, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hFile, MF_STRING, ID_MENU_FILE_EXIT, L"Выход\tEsc");
    
    HMENU hView = CreatePopupMenu();
    AppendMenu(hView, MF_STRING, ID_MENU_VIEW_TABLE, L"&Таблица");
    AppendMenu(hView, MF_STRING, ID_MENU_VIEW_CARDS, L"&Карточки");
    
    HMENU hSort = CreatePopupMenu();
    AppendMenu(hSort, MF_STRING, ID_MENU_SORT_NONE, L"Без сортировки");
    AppendMenu(hSort, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hSort, MF_STRING, ID_MENU_SORT_DESC_AVG, L"По ср.баллу (убывание)");
    AppendMenu(hSort, MF_STRING, ID_MENU_SORT_ASC_AVG, L"По ср.баллу (возрастание)");
    AppendMenu(hSort, MF_STRING, ID_MENU_SORT_ALPHA, L"По алфавиту");
    
    AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFile), L"&Файл");
    AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hView), L"&Вид");
    AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSort), L"&Сортировка");
    
    return hMenu;
}

// Точка входа программы
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"StudentManagerClass";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    
    RegisterClassEx(&wc);
    
    g_hMainWnd = CreateWindowEx(
        0,
        L"StudentManagerClass",
        L"Менеджер студентов",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr,
        CreateMainMenu(),
        hInstance,
        nullptr
    );
    
    if (!g_hMainWnd) {
        return 1;
    }
    
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);
    
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return static_cast<int>(msg.wParam);
}

// Процедура окна
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hStatusBar = CreateWindowEx(
            0, STATUSCLASSNAME, nullptr,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            hWnd, nullptr, GetModuleHandle(hWnd), nullptr
        );
        
        // Загружаем данные из файла при запуске
        LoadDataFromFile(g_dataFile);
        
        InitListView(hWnd);
        UpdateListView();
        return 0;
    }
    
    case WM_SIZE: {
        RECT rcClient, rcStatus;
        GetClientRect(hWnd, &rcClient);
        GetWindowRect(g_hStatusBar, &rcStatus);
        
        int statusHeight = rcStatus.bottom - rcStatus.top;
        int listHeight = rcClient.bottom - statusHeight;
        
        MoveWindow(g_hListView, 0, 0, rcClient.right, listHeight, TRUE);
        MoveWindow(g_hStatusBar, 0, listHeight, rcClient.right, statusHeight, TRUE);
        
        // Обновляем колонки статусной строки
        int parts[1];
        parts[0] = -1;
        SendMessage(g_hStatusBar, SB_SETPARTS, 1, reinterpret_cast<LPARAM>(parts));
        
        return 0;
    }
    
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_MENU_FILE_EXIT:
            DestroyWindow(hWnd);
            break;
            
        case ID_MENU_FILE_ADD: {
            int index = -1;
            ShowStudentDialog(index);
            break;
        }
        
        case ID_MENU_FILE_EDIT: {
            int selected = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
            if (selected != -1) {
                LVITEM lvi = {};
                lvi.mask = LVIF_PARAM;
                lvi.iItem = selected;
                ListView_GetItem(g_hListView, &lvi);
                int index = static_cast<int>(lvi.lParam);
                ShowStudentDialog(index);
            } else {
                MessageBox(hWnd, L"Выберите студента для редактирования", L"Информация", MB_ICONINFORMATION);
            }
            break;
        }
        
        case ID_MENU_FILE_DELETE: {
            int selected = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
            if (selected != -1) {
                LVITEM lvi = {};
                lvi.mask = LVIF_PARAM;
                lvi.iItem = selected;
                ListView_GetItem(g_hListView, &lvi);
                int index = static_cast<int>(lvi.lParam);
                
                if (MessageBox(hWnd, L"Удалить выбранного студента?", L"Подтверждение", 
                              MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    g_students.erase(g_students.begin() + index);
                    SaveDataToFile(g_dataFile); // Сохраняем после удаления
                    UpdateListView();
                }
            } else {
                MessageBox(hWnd, L"Выберите студента для удаления", L"Информация", MB_ICONINFORMATION);
            }
            break;
        }
        
        case ID_MENU_VIEW_TABLE:
            g_viewMode = true;
            ListView_SetView(g_hListView, LV_VIEW_REPORT);
            break;
            
        case ID_MENU_VIEW_CARDS:
            g_viewMode = false;
            // В режиме карточек можно использовать LV_VIEW_LIST или кастомное отображение
            ListView_SetView(g_hListView, LV_VIEW_LIST);
            break;
            
        case ID_MENU_SORT_DESC_AVG:
            SortByAverageNonRecursive(false); // убывание
            g_sortOrder = 1;
            UpdateListView();
            break;
            
        case ID_MENU_SORT_ASC_AVG:
            SortByAverageNonRecursive(true); // возрастание
            g_sortOrder = 2;
            UpdateListView();
            break;
            
        case ID_MENU_SORT_ALPHA:
            SortByNameRecursive(0, static_cast<int>(g_students.size()) - 1);
            g_sortOrder = 3;
            UpdateListView();
            break;
            
        case ID_MENU_SORT_NONE:
            g_sortOrder = 0;
            UpdateListView();
            break;
        }
        return 0;
        
    case WM_NOTIFY: {
        NMHDR* pNMHDR = reinterpret_cast<NMHDR*>(lParam);
        if (pNMHDR->code == NM_DBLCLK && pNMHDR->hwndFrom == g_hListView) {
            int selected = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
            if (selected != -1) {
                LVITEM lvi = {};
                lvi.mask = LVIF_PARAM;
                lvi.iItem = selected;
                ListView_GetItem(g_hListView, &lvi);
                int index = static_cast<int>(lvi.lParam);
                ShowStudentDialog(index);
            }
        }
        return 0;
    }
    
    case WM_KEYDOWN:
        if (wParam == VK_INSERT) {
            int index = -1;
            ShowStudentDialog(index);
        } else if (wParam == VK_RETURN) {
            int selected = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
            if (selected != -1) {
                LVITEM lvi = {};
                lvi.mask = LVIF_PARAM;
                lvi.iItem = selected;
                ListView_GetItem(g_hListView, &lvi);
                int index = static_cast<int>(lvi.lParam);
                ShowStudentDialog(index);
            }
        } else if (wParam == VK_DELETE) {
            SendMessage(hWnd, WM_COMMAND, MAKELONG(ID_MENU_FILE_DELETE, 0), 0);
        }
        break;
        
    case WM_DESTROY:
        SaveDataToFile(g_dataFile); // Сохраняем данные при закрытии
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
