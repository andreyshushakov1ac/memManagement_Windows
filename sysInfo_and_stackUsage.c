#include <windows.h>  // Подключаем заголовочный файл Windows API
#include <stdio.h>    // Подключаем заголовочный файл для функций ввода-вывода
#include <malloc.h>   // Подключаем заголовочный файл для функции _alloca
#include <psapi.h> // для GetProcessMemoryInfo : исп-мый объём памяти для конкретного процесса
#include <stdlib.h>

// Функция для вывода текущего использования стека
void print_stack_usage() {
    MEMORY_BASIC_INFORMATION mbi;  // Создаем структуру для хранения информации о памяти
    VirtualQuery(&mbi, &mbi, sizeof(mbi));  // Получаем информацию о текущем состоянии памяти
    // Выводим текущее использование стека в байтах
    printf("Текущее использование стэка: %zu bytes (%.3f KB)\n", (double)((char*)mbi.BaseAddress + mbi.RegionSize - (char*)&mbi), (double)(((char*)mbi.BaseAddress + mbi.RegionSize - (char*)&mbi)/1024));
}

int main() {

 printf("\n\n");

  // Get system information
  SYSTEM_INFO si;
  GetSystemInfo(&si);

  // Print system and memory information
  printf("System Information:\n");
  printf("Processor Architecture: %d\n", si.wProcessorArchitecture);
  printf("Page Size: %d bytes\n", si.dwPageSize);
  printf("Minimum Application Address: %p\n", si.lpMinimumApplicationAddress);
  printf("Maximum Application Address: %p\n", si.lpMaximumApplicationAddress);
  printf("Active Processor Mask: 0x%08X\n", si.dwActiveProcessorMask);
  printf("Number of Processors: %d\n", si.dwNumberOfProcessors);
  printf("Processor Type: %d\n", si.dwProcessorType);
  printf("Allocation Granularity: %d bytes\n", si.dwAllocationGranularity);
  printf("Processor Level: %d\n", si.wProcessorLevel);
  printf("Processor Revision: %d\n", si.wProcessorRevision);



 printf("\n\n");


    print_stack_usage();  // Вызываем функцию для вывода текущего использования стека

    int N =1024;
    // Создаем массив из 1024 целых чисел на стеке с помощью _alloca
    int* array = (int*)_alloca(N * sizeof(int));
    (void)array;  // Используем переменную, чтобы избежать предупреждений компилятора о неиспользуемой переменной

    int k =sizeof(array[0]) ;
    printf("Создаём (_alloca) на стеке массив %s на %d элементов (=>%d байт)\n",k == 1 ? "char" : k == 2 ? "short" : k == 4 ? "int" : k == 8 ? "long long" : "unknown",N ,k*N);

    print_stack_usage();  // Снова вызываем функцию для вывода текущего использования стека


    return 0;  // Завершаем выполнение программы
}

