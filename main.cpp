#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include <stack>
#include <shlwapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shlwapi.lib")

// Структура студента
struct Student {
    wchar_t fullName[100];   // ФИО
    int recordBookNumber;    // Номер зачетки
    int grades[5];           // 5 оценок
    
    double getAverage() const {
        int sum = 0;
        for (int i = 0; i < 5; ++i) sum += grades[i];
        return (double)sum / 5.0;
    }
};

// Глобальные переменные
std::vector<Student> g_students;
HWND g_hMainWnd = NULL;
HWND g_hListView = NULL;
HWND g_hStatusBar = NULL;
HFONT g_hFont = NULL;
bool g_isTableView = true; // true - таблица, false - карточки
wchar_t g_currentFile[MAX_PATH] = L"students.csv";

// ID элементов управления
#define IDM_FILE_OPEN       1001
#define IDM_FILE_SAVE       1002
#define IDM_FILE_EXIT       1003
#define IDM_VIEW_TABLE      1004
#define IDM_VIEW_CARDS      1005
#define IDM_SORT_ALPHA_ASC  1006
#define IDM_SORT_ALPHA_DESC 1007
#define IDM_SORT_AVG_ASC    1008
#define IDM_SORT_AVG_DESC   1009
#define IDM_HELP_ABOUT      1010
#define IDI_APP_ICON        2001

#define IDC_LISTVIEW        3001
#define IDD_STUDENT_DIALOG  4001
#define IDC_EDIT_FIO        4002
#define IDC_EDIT_BOOK       4003
#define IDC_EDIT_G1         4004
#define IDC_EDIT_G2         4005
#define IDC_EDIT_G3         4006
#define IDC_EDIT_G4         4007
#define IDC_EDIT_G5         4008
#define IDOK_SAVE           4009
#define IDCANCEL_CLOSE      4010

// Объявления функций
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK StudentDlgProc(HWND, UINT, WPARAM, LPARAM);
void LoadStudentsFromFile(const wchar_t* filename);
void SaveStudentsToFile(const wchar_t* filename);
void UpdateListView();
void SortByNameRecursive(int left, int right, bool ascending);
void SortByAverageNonRecursive(bool ascending);
void ShowStudentDialog(int index);
void DeleteSelectedStudent();

// --- Быстрая сортировка (Рекурсивная) по ФИО ---
void SortByNameRecursive(int left, int right, bool ascending) {
    if (left >= right) return;

    int i = left, j = right;
    std::wstring pivot = g_students[(left + right) / 2].fullName;

    while (i <= j) {
        int cmp = wcscmp(g_students[i].fullName, pivot.c_str());
        if ((ascending && cmp < 0) || (!ascending && cmp > 0)) { i++; }
        else {
            cmp = wcscmp(g_students[j].fullName, pivot.c_str());
            if ((ascending && cmp > 0) || (!ascending && cmp < 0)) { j--; }
            else {
                std::swap(g_students[i], g_students[j]);
                i++; j--;
            }
        }
    }

    if (left < j) SortByNameRecursive(left, j, ascending);
    if (i < right) SortByNameRecursive(i, right, ascending);
}

// --- Быстрая сортировка (Нерекурсивная) по Среднему баллу ---
void SortByAverageNonRecursive(bool ascending) {
    if (g_students.empty()) return;

    std::stack<std::pair<int, int>> stack;
    stack.push({0, (int)g_students.size() - 1});

    while (!stack.empty()) {
        int left = stack.top().first;
        int right = stack.top().second;
        stack.pop();

        if (left >= right) continue;

        int i = left, j = right;
        double pivot = g_students[(left + right) / 2].getAverage();

        while (i <= j) {
            if ((ascending && g_students[i].getAverage() < pivot) || 
                (!ascending && g_students[i].getAverage() > pivot)) { i++; }
            else {
                if ((ascending && g_students[j].getAverage() > pivot) || 
                    (!ascending && g_students[j].getAverage() < pivot)) { j--; }
                else {
                    std::swap(g_students[i], g_students[j]);
                    i++; j--;
                }
            }
        }

        if (left < j) stack.push({left, j});
        if (i < right) stack.push({i, right});
    }
}

// --- Загрузка из CSV ---
void LoadStudentsFromFile(const wchar_t* filename) {
    FILE* f = _wfopen(filename, L"r, ccs=UTF-8");
    if (!f) {
        // Если файла нет, это не ошибка, просто пустой список
        if (GetFileAttributes(filename) == INVALID_FILE_ATTRIBUTES) {
            g_students.clear();
            UpdateListView();
            return;
        }
        MessageBoxW(g_hMainWnd, L"Не удалось открыть файл для чтения.", L"Ошибка", MB_ICONERROR);
        return;
    }

    g_students.clear();
    wchar_t line[512];
    
    // Пропускаем заголовок, если есть
    // fgets не совсем корректно работает с Unicode в MSVC, используем fgetws
    if (fgetws(line, 512, f) != NULL) {
        // Проверим, не является ли первая строка данными (простая эвристика: если вторая ячейка число)
        // Для простоты считаем, что первая строка может быть заголовком или данными.
        // Попробуем распарсить первую строку. Если не выйдет - читаем дальше.
        rewind(f); 
    }

    while (fgetws(line, 512, f) != NULL) {
        // Убираем \n \r
        size_t len = wcslen(line);
        while (len > 0 && (line[len-1] == L'\n' || line[len-1] == L'\r')) line[--len] = 0;
        if (len == 0) continue;

        Student s;
        // Формат: ФИО;Зачетка;О1;О2;О3;О4;О5
        // Используем wcstok_s для парсинга
        wchar_t* next_token = NULL;
        wchar_t* token = wcstok_s(line, L";", &next_token);
        if (!token) continue;
        wcsncpy_s(s.fullName, token, _TRUNCATE);

        token = wcstok_s(NULL, L";", &next_token);
        if (!token) continue;
        s.recordBookNumber = _wtoi(token);

        for (int i = 0; i < 5; ++i) {
            token = wcstok_s(NULL, L";", &next_token);
            if (token) s.grades[i] = _wtoi(token);
            else s.grades[i] = 0;
        }
        g_students.push_back(s);
    }

    fclose(f);
    wcscpy_s(g_currentFile, filename);
    UpdateListView();
    
    wchar_t msg[256];
    swprintf(msg, 256, L"Загружено студентов: %d", (int)g_students.size());
    SetWindowTextW(g_hStatusBar, msg);
}

// --- Сохранение в CSV ---
void SaveStudentsToFile(const wchar_t* filename) {
    FILE* f = _wfopen(filename, L"w, ccs=UTF-8");
    if (!f) {
        MessageBoxW(g_hMainWnd, L"Не удалось сохранить файл.", L"Ошибка", MB_ICONERROR);
        return;
    }

    // Заголовок
    fwprintf(f, L"ФИО;Зачетка;О1;О2;О3;О4;О5\n");

    for (const auto& s : g_students) {
        fwprintf(f, L"%s;%d;%d;%d;%d;%d;%d\n", 
            s.fullName, s.recordBookNumber,
            s.grades[0], s.grades[1], s.grades[2], s.grades[3], s.grades[4]);
    }

    fclose(f);
    wcscpy_s(g_currentFile, filename);
    
    wchar_t msg[256];
    swprintf(msg, 256, L"Сохранено: %s", filename);
    SetWindowTextW(g_hStatusBar, msg);
    MessageBoxW(g_hMainWnd, L"Данные успешно сохранены!", L"Успех", MB_ICONINFORMATION);
}

// --- Обновление ListView ---
void UpdateListView() {
    if (!g_hListView) return;

    ListView_DeleteAllItems(g_hListView);

    if (!g_isTableView) {
        // В режиме карточек скрываем заголовки и меняем стиль, но для простоты
        // оставим список, но будем отображать данные иначе или просто заполним
        // В данном примере мы просто заполняем список, а режим "карточки" 
        // можно реализовать через OwnerData или просто текстовое представление в одной колонке
        // Для упрощения WinAPI кода без CustomDraw сложностей, сделаем так:
        // В режиме карточек - одна колонка с подробной инфо.
        LVCOLUMNW lvc = {0};
        lvc.mask = LVCF_WIDTH | LVCF_TEXT;
        lvc.cx = 400; // Широкая колонка
        lvc.pszText = (wchar_t*)L"Карточка студента";
        ListView_InsertColumn(g_hListView, 0, &lvc);
        
        // Удаляем лишние колонки если были
        int colCount = (int)SendMessage(g_hListView, LVM_GETCOLUMNCOUNT, 0, 0);
        for (int i = colCount - 1; i > 0; --i) {
            ListView_DeleteColumn(g_hListView, i);
        }

        for (size_t i = 0; i < g_students.size(); ++i) {
            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT | LVIF_PARAM;
            lvi.iItem = (int)i;
            
            wchar_t buffer[512];
            swprintf(buffer, 512, L"ФИО: %s\nЗачетка: %d\nОценки: %d %d %d %d %d\nСредний балл: %.2f",
                g_students[i].fullName,
                g_students[i].recordBookNumber,
                g_students[i].grades[0], g_students[i].grades[1], g_students[i].grades[2],
                g_students[i].grades[3], g_students[i].grades[4],
                g_students[i].getAverage());
            
            lvi.pszText = buffer;
            ListView_InsertItem(g_hListView, &lvi);
        }
    } else {
        // Табличный режим
        // Настройка колонок
        const wchar_t* cols[] = { L"№", L"ФИО", L"Зачетка", L"О1", L"О2", L"О3", L"О4", L"О5", L"Ср. балл" };
        int widths[] = { 40, 200, 80, 40, 40, 40, 40, 40, 70 };

        // Очищаем старые колонки
        int colCount2 = (int)SendMessage(g_hListView, LVM_GETCOLUMNCOUNT, 0, 0);
        while (colCount2 > 0) {
            ListView_DeleteColumn(g_hListView, 0);
            colCount2--;
        }

        for (int i = 0; i < 9; ++i) {
            LVCOLUMNW lvc = {0};
            lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.iSubItem = i;
            lvc.cx = widths[i];
            lvc.pszText = (wchar_t*)cols[i];
            ListView_InsertColumn(g_hListView, i, &lvc);
        }

        for (size_t i = 0; i < g_students.size(); ++i) {
            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT | LVIF_PARAM;
            lvi.iItem = (int)i;
            lvi.lParam = (LPARAM)i; // Индекс в векторе

            wchar_t buffer[50];
            
            // Колонка 0: № п/п
            swprintf(buffer, 50, L"%d", (int)i + 1);
            lvi.pszText = buffer;
            lvi.iSubItem = 0;
            ListView_InsertItem(g_hListView, &lvi);

            // Колонка 1: ФИО
            lvi.mask = LVIF_TEXT;
            lvi.pszText = g_students[i].fullName;
            lvi.iSubItem = 1;
            ListView_SetItem(g_hListView, &lvi);

            // Колонка 2: Зачетка
            swprintf(buffer, 50, L"%d", g_students[i].recordBookNumber);
            lvi.pszText = buffer;
            lvi.iSubItem = 2;
            ListView_SetItem(g_hListView, &lvi);

            // Оценки
            for (int g = 0; g < 5; ++g) {
                swprintf(buffer, 50, L"%d", g_students[i].grades[g]);
                lvi.pszText = buffer;
                lvi.iSubItem = 3 + g;
                ListView_SetItem(g_hListView, &lvi);
            }

            // Средний балл
            swprintf(buffer, 50, L"%.2f", g_students[i].getAverage());
            lvi.pszText = buffer;
            lvi.iSubItem = 8;
            ListView_SetItem(g_hListView, &lvi);
        }
    }
    
    wchar_t msg[256];
    swprintf(msg, 256, L"Всего студентов: %d | Режим: %s", (int)g_students.size(), g_isTableView ? L"Таблица" : L"Карточки");
    SetWindowTextW(g_hStatusBar, msg);
}

// --- Диалог добавления/редактирования ---
INT_PTR CALLBACK StudentDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static int editIndex; // -1 если новый
    switch (message) {
    case WM_INITDIALOG:
        editIndex = (int)lParam;
        {
            HWND hFio = GetDlgItem(hDlg, IDC_EDIT_FIO);
            HWND hBook = GetDlgItem(hDlg, IDC_EDIT_BOOK);
            HWND hG[5];
            hG[0] = GetDlgItem(hDlg, IDC_EDIT_G1);
            hG[1] = GetDlgItem(hDlg, IDC_EDIT_G2);
            hG[2] = GetDlgItem(hDlg, IDC_EDIT_G3);
            hG[3] = GetDlgItem(hDlg, IDC_EDIT_G4);
            hG[4] = GetDlgItem(hDlg, IDC_EDIT_G5);

            if (editIndex != -1 && editIndex < (int)g_students.size()) {
                SetWindowTextW(hFio, g_students[editIndex].fullName);
                
                wchar_t buf[20];
                swprintf(buf, 20, L"%d", g_students[editIndex].recordBookNumber);
                SetWindowTextW(hBook, buf);

                for (int i = 0; i < 5; ++i) {
                    swprintf(buf, 20, L"%d", g_students[editIndex].grades[i]);
                    SetWindowTextW(hG[i], buf);
                }
                SetWindowTextW(GetDlgItem(hDlg, IDOK_SAVE), L"Изменить");
            } else {
                SetWindowTextW(GetDlgItem(hDlg, IDOK_SAVE), L"Добавить");
                // Фокус на ФИО
                SetFocus(hFio);
                return FALSE; 
            }
        }
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK_SAVE) {
            wchar_t buf[256];
            Student s;
            
            GetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_FIO), s.fullName, 100);
            if (wcslen(s.fullName) == 0) {
                MessageBoxW(hDlg, L"Введите ФИО", L"Ошибка", MB_ICONWARNING);
                return TRUE;
            }

            GetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_BOOK), buf, 20);
            s.recordBookNumber = _wtoi(buf);

            bool ok = true;
            for (int i = 0; i < 5; ++i) {
                GetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_G1 + i), buf, 10);
                int val = _wtoi(buf);
                if (val < 2 || val > 5) {
                    MessageBoxW(hDlg, L"Оценки должны быть от 2 до 5", L"Ошибка", MB_ICONWARNING);
                    ok = false; break;
                }
                s.grades[i] = val;
            }

            if (ok) {
                if (editIndex != -1 && editIndex < (int)g_students.size()) {
                    g_students[editIndex] = s;
                } else {
                    g_students.push_back(s);
                }
                SaveStudentsToFile(g_currentFile); // Автосохранение
                UpdateListView();
                EndDialog(hDlg, TRUE);
            }
            return TRUE;
        } else if (LOWORD(wParam) == IDCANCEL_CLOSE || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, FALSE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Глобальные переменные для диалога
HWND g_hDlgWnd = NULL;
int g_dlgTargetIndex = -1;

LRESULT CALLBACK DlgWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowExW(0, L"STATIC", L"ФИО:", WS_CHILD | WS_VISIBLE, 10, 10, 50, 20, hWnd, NULL, NULL, NULL);
        HWND hFio = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 70, 10, 200, 25, hWnd, (HMENU)IDC_EDIT_FIO, NULL, NULL);
        
        CreateWindowExW(0, L"STATIC", L"Зачетка:", WS_CHILD | WS_VISIBLE, 10, 45, 60, 20, hWnd, NULL, NULL, NULL);
        HWND hBook = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 70, 45, 100, 25, hWnd, (HMENU)IDC_EDIT_BOOK, NULL, NULL);

        const wchar_t* labels[] = {L"О1:", L"О2:", L"О3:", L"О4:", L"О5:"};
        for(int i=0; i<5; ++i) {
            CreateWindowExW(0, L"STATIC", labels[i], WS_CHILD | WS_VISIBLE, 10 + (i*60), 80, 30, 20, hWnd, NULL, NULL, NULL);
            CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_NUMBER, 
                40 + (i*60), 80, 40, 25, hWnd, (HMENU)(IDC_EDIT_G1 + i), NULL, NULL);
        }

        CreateWindowExW(0, L"BUTTON", L"Сохранить", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 
            100, 130, 100, 30, hWnd, (HMENU)IDOK_SAVE, NULL, NULL);
        CreateWindowExW(0, L"BUTTON", L"Отмена", WS_CHILD | WS_VISIBLE, 
            220, 130, 100, 30, hWnd, (HMENU)IDCANCEL_CLOSE, NULL, NULL);

        // Заполнение данными
        if (g_dlgTargetIndex != -1 && g_dlgTargetIndex < (int)g_students.size()) {
            Student& s = g_students[g_dlgTargetIndex];
            SetWindowTextW(hFio, s.fullName);
            wchar_t buf[20];
            swprintf(buf, 20, L"%d", s.recordBookNumber);
            SetWindowTextW(hBook, buf);
            for(int i=0; i<5; ++i) {
                swprintf(buf, 20, L"%d", s.grades[i]);
                SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_G1+i), buf);
            }
            SetWindowTextW(GetDlgItem(hWnd, IDOK_SAVE), L"Изменить");
        } else {
             SetWindowTextW(GetDlgItem(hWnd, IDOK_SAVE), L"Добавить");
             SetFocus(hFio);
        }
        SendMessage(hFio, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK_SAVE) {
            wchar_t buf[256];
            Student s;
            HWND hFio = GetDlgItem(hWnd, IDC_EDIT_FIO);
            GetWindowTextW(hFio, s.fullName, 100);
            if (wcslen(s.fullName) == 0) { MessageBoxW(hWnd, L"Введите ФИО", L"Ошибка", MB_ICONWARNING); return 0; }
            
            GetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_BOOK), buf, 20);
            s.recordBookNumber = _wtoi(buf);

            bool ok = true;
            for (int i = 0; i < 5; ++i) {
                GetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_G1 + i), buf, 10);
                int val = _wtoi(buf);
                if (val < 2 || val > 5) { MessageBoxW(hWnd, L"Оценки 2-5", L"Ошибка", MB_ICONWARNING); ok = false; break; }
                s.grades[i] = val;
            }
            if (ok) {
                if (g_dlgTargetIndex != -1 && g_dlgTargetIndex < (int)g_students.size()) g_students[g_dlgTargetIndex] = s;
                else g_students.push_back(s);
                SaveStudentsToFile(g_currentFile);
                UpdateListView();
                DestroyWindow(hWnd);
            }
            return 0;
        } else if (LOWORD(wParam) == IDCANCEL_CLOSE) {
            DestroyWindow(hWnd);
            return 0;
        }
        break;
    case WM_DESTROY:
        g_hDlgWnd = NULL;
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void ShowStudentDialog(int index) {
    HINSTANCE hInst = GetModuleHandle(NULL);
    
    // Регистрация класса для диалога
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = DlgWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"StudentDlgClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClassExW(&wc);

    g_dlgTargetIndex = index;
    
    // Создаем модальное окно
    HWND hParent = g_hMainWnd;
    EnableWindow(hParent, FALSE);
    
    g_hDlgWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"StudentDlgClass",
        index == -1 ? L"Добавление студента" : L"Редактирование студента",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 180,
        hParent, NULL, hInst, NULL
    );
    
    ShowWindow(g_hDlgWnd, SW_SHOW);
    UpdateWindow(g_hDlgWnd);

    // Модальный цикл сообщений
    MSG msg;
    BOOL bRet;
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (bRet == -1) break;
        if (msg.hwnd == g_hDlgWnd || IsChild(g_hDlgWnd, msg.hwnd)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Если окно диалога уничтожено, выходим
        if (g_hDlgWnd == NULL) break;
    }
    
    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
    UnregisterClassW(L"StudentDlgClass", hInst);
}

void DeleteSelectedStudent() {
    int sel = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
    if (sel == -1) {
        MessageBoxW(g_hMainWnd, L"Выберите студента для удаления", L"Внимание", MB_ICONWARNING);
        return;
    }
    
    // Получаем индекс из lParam (если таблица) или просто используем порядок (если карточки и нет пересортировки в UI)
    // В нашем коде lParam хранит оригинальный индекс? Нет, мы вставляем по порядку.
    // При сортировке вектор меняется, а ListView перестраивается. Значит, порядок в ListView соответствует вектору.
    
    if (sel >= 0 && sel < (int)g_students.size()) {
        wchar_t confirm[256];
        swprintf(confirm, L"Удалить студента: %s?", g_students[sel].fullName);
        if (MessageBoxW(g_hMainWnd, confirm, L"Подтверждение", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            g_students.erase(g_students.begin() + sel);
            SaveStudentsToFile(g_currentFile);
            UpdateListView();
        }
    }
}

// --- Основная процедура окна ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        // Список
        g_hListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            0, 0, 0, 0, hwnd, (HMENU)IDC_LISTVIEW, NULL, NULL);
        ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        SendMessage(g_hListView, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Статус бар
        g_hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, L"Готово",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hwnd, NULL, NULL, NULL);
        SendMessage(g_hStatusBar, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Загрузка данных при старте
        LoadStudentsFromFile(g_currentFile);
        return 0;

    case WM_SIZE:
        if (g_hListView) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            // Резервируем место под статус бар (примерно 30 пикселей)
            MoveWindow(g_hListView, 0, 0, rc.right, rc.bottom - 30, TRUE);
            
            int parts[2] = { rc.right - 100, -1 };
            SendMessage(g_hStatusBar, WM_SIZE, 0, 0);
            SendMessage(g_hStatusBar, SB_SETPARTS, 2, (LPARAM)parts);
            MoveWindow(g_hStatusBar, 0, rc.bottom - 30, rc.right, 30, TRUE);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_FILE_OPEN: {
            OPENFILENAMEW ofn = {0};
            wchar_t szFile[MAX_PATH] = L"";
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
            ofn.lpstrDefExt = L"csv";
            if (GetOpenFileNameW(&ofn)) {
                LoadStudentsFromFile(szFile);
            }
            break;
        }
        case IDM_FILE_SAVE: {
            OPENFILENAMEW ofn = {0};
            wchar_t szFile[MAX_PATH] = L"students.csv";
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
            ofn.lpstrDefExt = L"csv";
            if (GetSaveFileNameW(&ofn)) {
                SaveStudentsToFile(szFile);
            }
            break;
        }
        case IDM_FILE_EXIT:
            DestroyWindow(hwnd);
            break;
            
        case IDM_VIEW_TABLE:
            g_isTableView = true;
            UpdateListView();
            break;
        case IDM_VIEW_CARDS:
            g_isTableView = false;
            UpdateListView();
            break;

        case IDM_SORT_ALPHA_ASC:
            SortByNameRecursive(0, (int)g_students.size() - 1, true);
            UpdateListView();
            SaveStudentsToFile(g_currentFile);
            break;
        case IDM_SORT_ALPHA_DESC:
            SortByNameRecursive(0, (int)g_students.size() - 1, false);
            UpdateListView();
            SaveStudentsToFile(g_currentFile);
            break;
        case IDM_SORT_AVG_ASC:
            SortByAverageNonRecursive(true);
            UpdateListView();
            SaveStudentsToFile(g_currentFile);
            break;
        case IDM_SORT_AVG_DESC:
            SortByAverageNonRecursive(false);
            UpdateListView();
            SaveStudentsToFile(g_currentFile);
            break;

        case IDM_HELP_ABOUT:
            MessageBoxW(hwnd, L"Студенты Manager v1.0\nWin32 API + CSV", L"О программе", MB_ICONINFORMATION);
            break;
        }
        return 0;

    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->idFrom == IDC_LISTVIEW) {
            NMHDR* pnmh = (LPNMHDR)lParam;
            if (pnmh->code == NM_DBLCLK) {
                // Двойной клик - редактировать
                int sel = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
                if (sel != -1) ShowStudentDialog(sel);
            }
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_INSERT) {
            ShowStudentDialog(-1); // Добавить нового
        } else if (wParam == VK_DELETE) {
            DeleteSelectedStudent();
        } else if (wParam == VK_RETURN) {
            int sel = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
            if (sel != -1) ShowStudentDialog(sel);
        }
        break;

    case WM_DESTROY:
        SaveStudentsToFile(g_currentFile); // Финальное сохранение
        if (g_hFont) DeleteObject(g_hFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// --- Точка входа ---
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = L"StudentManagerClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) return 0;

    // Создание меню
    HMENU hMenu = CreateMenu();
    HMENU hFile = CreatePopupMenu();
    AppendMenuW(hFile, MF_STRING, IDM_FILE_OPEN, L"&Открыть...\tCtrl+O");
    AppendMenuW(hFile, MF_STRING, IDM_FILE_SAVE, L"&Сохранить как...\tCtrl+S");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, IDM_FILE_EXIT, L"Выход\tAlt+F4");

    HMENU hView = CreatePopupMenu();
    AppendMenuW(hView, MF_STRING, IDM_VIEW_TABLE, L"&Таблица");
    AppendMenuW(hView, MF_STRING, IDM_VIEW_CARDS, L"&Карточки");
    AppendMenuW(hView, MF_SEPARATOR, 0, NULL);
    
    HMENU hSort = CreatePopupMenu();
    AppendMenuW(hSort, MF_STRING, IDM_SORT_ALPHA_ASC, L"По ФИО (А-Я)");
    AppendMenuW(hSort, MF_STRING, IDM_SORT_ALPHA_DESC, L"По ФИО (Я-А)");
    AppendMenuW(hSort, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hSort, MF_STRING, IDM_SORT_AVG_ASC, L"По среднему баллу (возр.)");
    AppendMenuW(hSort, MF_STRING, IDM_SORT_AVG_DESC, L"По среднему баллу (убыв.)");

    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFile, L"&Файл");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hView, L"&Вид");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSort, L"&Сортировка");
    AppendMenuW(hMenu, MF_STRING, IDM_HELP_ABOUT, L"?");

    g_hMainWnd = CreateWindowExW(0, L"StudentManagerClass", L"Менеджер студентов (WinAPI)",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, hMenu, hInstance, NULL);

    if (!g_hMainWnd) return 0;

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
