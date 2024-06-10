#include <windows.h>
#include <stdio.h>

// Определение идентификаторов для кнопок и статического текста
#define IDC_STATIC (-1) // Определение IDC_STATIC для статического текста
#define IDC_STATIC_GLOBAL_MEMORY (8) // Определение IDC_STATIC для статического текста
#define IDC_STATIC_STACK_USAGE (9) // Определение IDC_STATIC для статического текста
#define ID_ALLOCATE_HEAP_BUTTON 1
#define ID_FREE_HEAP_BUTTON 2
#define ID_ALLOC_VIRT_MEM_BUTTON 3
#define ID_BUTTON_FREE_VIRT_MEM 4
#define ID_BUTTON_5 5
#define ID_BUTTON_ALLOC_STATIC 6
#define ID_BUTTON_REFRESH_DATA 7

int wasStaticButtPressed = 0;

// Функция для получения общего объема памяти, используемой в куче
SIZE_T get_heap_memory_usage(HANDLE heap) {
    SIZE_T totalSize = 0; // Переменная для хранения общего объема памяти
    PROCESS_HEAP_ENTRY entry; // Структура для хранения информации о блоках памяти в куче
    entry.lpData = NULL; // Инициализация указателя на данные

    // Обход всех блоков памяти в куче с помощью HeapWalk
    while (HeapWalk(heap, &entry)) {
        // Если блок занят, добавляем его размер к общему объему памяти
        if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) {
            totalSize += entry.cbData;
        }
    }

    // Проверка на наличие ошибок после завершения обхода
    DWORD error = GetLastError();
    if (error != ERROR_NO_MORE_ITEMS) {
        fprintf(stderr, "HeapWalk failed with error %lu\n", error);
    }

    return totalSize; // Возвращаем общий объем памяти
}

// Функция для обновления отображаемого объема памяти, используемой в куче
void update_memory_usage(HWND hwnd, HANDLE heap) {
    SIZE_T memoryUsage = get_heap_memory_usage(heap); // Получаем текущий объем памяти
    char buffer[256]; // Буфер для строки с информацией
    sprintf(buffer, "Current HEAP memory usage: %zu bytes (%zu KB)", memoryUsage, memoryUsage / 1024); // Форматируем строку
    SetWindowText(GetDlgItem(hwnd, IDC_STATIC), buffer); // Устанавливаем текст статического элемента управления
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void update_global_memory_status(HWND hwnd) {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);

    char buffer[512];
    sprintf(buffer, "Global Memory Status:\nTotal Physical Memory: %llu KB (%.3f GB)\nAvailable Physical Memory: %llu KB (%.3f GB)\nTotal Virtual Memory: %llu KB (%.3f GB)\nAvailable Virtual Memory: %llu KB (%.3f GB)",
             statex.ullTotalPhys / 1024, (double)(statex.ullTotalPhys / (1024*1024*1024)),
             statex.ullAvailPhys / 1024, (double)(statex.ullAvailPhys / (1024*1024*1024)),
             statex.ullTotalVirtual / 1024, (double)(statex.ullTotalVirtual / (1024*1024*1024)),
             statex.ullAvailVirtual / 1024, (double)(statex.ullAvailVirtual / (1024*1024*1024))
            );

    SetWindowText(GetDlgItem(hwnd, IDC_STATIC_GLOBAL_MEMORY), buffer);
}

void update_stack_usage(HWND hwnd) {
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery(&mbi, &mbi, sizeof(mbi));
    SIZE_T stackUsage = (char*)mbi.BaseAddress + mbi.RegionSize - (char*)&mbi;

    char buffer[256];
    sprintf(buffer, "Current stack usage: %zu bytes (%zu KB)", stackUsage, stackUsage / 1024);
    SetWindowText(GetDlgItem(hwnd, IDC_STATIC_STACK_USAGE), buffer);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* В функции virtualMemoryAlloc переменная lpvBase используется для хранения адреса выделенной виртуальной памяти.
Если выделение памяти прошло успешно, то lpvBase будет указывать на начало выделенного региона.
В противном случае, lpvBase будет иметь значение NULL. */
LPVOID lpvBase = NULL;

void virtualMemoryAlloc(HWND hwnd) {
    if (lpvBase) {
        MessageBox(hwnd, "First release last reserved memory", "Warning", MB_OK | MB_ICONERROR);
    } else {
        lpvBase = VirtualAlloc(NULL,
                               1024*16 * sizeof(int), // 64 KB - гранулярность 
                               MEM_COMMIT, // резервируем для процесса и выделяем в физической
                               PAGE_READWRITE);
        if (lpvBase) {
            int* pInt = (int*)lpvBase;
            for (int i = 0; i < 1024*16; ++i) {
                pInt[i] = i;
            }
        }
    }
}

void virtualMemoryFree(HWND hwnd) {
    if (!lpvBase) {
        MessageBox(hwnd, "Error! Failed to release not reserved memory", "Error", MB_OK | MB_ICONERROR);
    } else {
        VirtualFree(lpvBase, 0, MEM_RELEASE);
        lpvBase = NULL;
    }
}    
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




// Основная оконная процедура для обработки сообщений окна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static int *array = NULL; // Указатель на выделенный массив (статический для сохранения состояния между вызовами)
    static HANDLE heap = NULL; // Дескриптор кучи (статический для сохранения состояния между вызовами)

    switch (uMsg) {
        case WM_CREATE:
            heap = GetProcessHeap(); // Получаем дескриптор текущей кучи процесса
            // Создаем кнопку для выделения памяти
            CreateWindow("BUTTON", "Allocate 4 KB in HEAP", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         10, 10, 200, 30, hwnd, (HMENU)ID_ALLOCATE_HEAP_BUTTON, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            // Создаем кнопку для освобождения памяти
            CreateWindow("BUTTON", "Free 4KB in HEAP", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         10, 50, 200, 30, hwnd, (HMENU)ID_FREE_HEAP_BUTTON, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            CreateWindow("BUTTON", "Allocate 64KB of VIRT mem", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         220, 10, 200, 30, hwnd, (HMENU)ID_ALLOC_VIRT_MEM_BUTTON, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            CreateWindow("BUTTON", "Release 64KB of VIRT mem", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         220, 50, 200, 30, hwnd, (HMENU)ID_BUTTON_FREE_VIRT_MEM, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            //CreateWindow("BUTTON", "Button 5", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                //         330, 10, 100, 30, hwnd, (HMENU)ID_BUTTON_5, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            CreateWindow("BUTTON", "Allocate 4KB STATIC mem", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         430, 10, 200, 30, hwnd, (HMENU)ID_BUTTON_ALLOC_STATIC, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            CreateWindow("BUTTON", "REFRESH", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         10, 280, 100, 30, hwnd, (HMENU)ID_BUTTON_REFRESH_DATA, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
                    // Создаем статический элемент управления для отображения информации о памяти
            CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD,
                         10, 90, 300, 40, hwnd, (HMENU)IDC_STATIC, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

                        // Static text fields for memory information
            CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD,
                         10, 140, 580, 90, hwnd, (HMENU)IDC_STATIC_GLOBAL_MEMORY, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
           /// CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD,
                    ///     10, 240, 580, 30, hwnd, (HMENU)IDC_STATIC_STACK_USAGE, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

            
            update_global_memory_status(hwnd);
            update_stack_usage(hwnd);
            update_memory_usage(hwnd, heap); // Обновляем информацию о памяти при создании окна
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_ALLOCATE_HEAP_BUTTON:
                    if (array == NULL) {
                        array = (int *)HeapAlloc(heap, 0, 1024 * sizeof(int)); // Выделяем память в куче
                        if (array == NULL) {
                            MessageBox(hwnd, "Failed to allocate memory", "Error", MB_OK | MB_ICONERROR); // Сообщение об ошибке при неудачном выделении памяти
                        } else {
                            printf("Allocated 4 KB at address %p\n", array); // Вывод адреса выделенной памяти в консоль
                        }
                        update_memory_usage(hwnd, heap); // Обновляем информацию о памяти после выделения
                        update_global_memory_status(hwnd);
                        update_stack_usage(hwnd);
                    } else {
                        MessageBox(hwnd, "The memory has been already allocated", "Info", MB_OK | MB_ICONINFORMATION); // Сообщение о повторном выделении памяти
                    }
                    break;

                case ID_FREE_HEAP_BUTTON:
                    if (array != NULL) {
                        HeapFree(heap, 0, array); // Освобождаем выделенную память в куче
                        printf("Freed memory at address %p\n", array); // Вывод адреса освобожденной памяти в консоль
                        array = NULL; // Сбрасываем указатель на массив
                        update_memory_usage(hwnd, heap); // Обновляем информацию о памяти после освобождения
                        update_global_memory_status(hwnd);
                        update_stack_usage(hwnd);
                    } else {
                        MessageBox(hwnd, "No memory allocated to be freed", "Error", MB_OK | MB_ICONERROR); // Сообщение об ошибке при попытке освободить невыделенную память
                    }
                    break;
                
                case ID_ALLOC_VIRT_MEM_BUTTON:
                   virtualMemoryAlloc(hwnd);

                   update_global_memory_status(hwnd);
                   update_stack_usage(hwnd);
                   update_memory_usage(hwnd, heap);
                    break;

                case ID_BUTTON_FREE_VIRT_MEM:
                   virtualMemoryFree(hwnd);

                   update_global_memory_status(hwnd);
                   update_stack_usage(hwnd);
                   update_memory_usage(hwnd, heap);
                    break;
                case ID_BUTTON_5:
                     // Handle button press
                    break;
                case ID_BUTTON_ALLOC_STATIC:
                   if (wasStaticButtPressed==0)
                        {
                            static int staticArr[1024];
                            int i; for (i=0;i<1024;++i) staticArr[i]= i; 
                            wasStaticButtPressed = 1;
                        }
                   else
                        MessageBox(hwnd, "Static array has already been created.", "Warning", MB_OK | MB_ICONERROR);

                   update_global_memory_status(hwnd);
                   update_stack_usage(hwnd);
                   update_memory_usage(hwnd, heap);
                    break;

                case ID_BUTTON_REFRESH_DATA:
                    update_global_memory_status(hwnd); 
                    update_memory_usage(hwnd, heap);
                    update_stack_usage(hwnd);
                    break;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0); // Завершаем приложение при закрытии окна
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam); // Обработка всех остальных сообщений по умолчанию
    }
    return 0;
}

// Главная функция приложения WinMain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "Sample Window Class"; // Имя класса окна

    WNDCLASS wc = { }; // Структура для регистрации класса окна
    wc.lpfnWndProc = WindowProc; // Указатель на оконную процедуру
    wc.hInstance = hInstance; // Дескриптор экземпляра приложения
    wc.lpszClassName = CLASS_NAME; // Имя класса окна

    RegisterClass(&wc); // Регистрация класса окна

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Memory infromation and management",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 400,
        NULL,
        NULL,
        hInstance,
        NULL
);

    if (hwnd == NULL) {
        return 0; // Завершаем приложение при ошибке создания окна
    }

    ShowWindow(hwnd, nCmdShow); // Отображаем окно

    MSG msg = { }; // Структура для хранения сообщений

    while (GetMessage(&msg, NULL, 0, 0)) { // Цикл обработки сообщений
        TranslateMessage(&msg); // Преобразование сообщений клавиатуры
        DispatchMessage(&msg); // Отправка сообщений оконной процедуре
    }

    return 0; // Завершаем приложение при выходе из цикла обработки сообщений
}