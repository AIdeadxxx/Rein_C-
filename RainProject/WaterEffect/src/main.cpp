#include "WaterEffect.h"
#include <windows.h>
#include <windowsx.h>

// Точка входа в приложение
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Предотвращаем предупреждения компилятора о неиспользуемых параметрах
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Создаем экземпляр класса эффекта воды
    WaterEffect waterEffect;

    // Инициализируем приложение
    if (!waterEffect.Initialize(hInstance)) {
        MessageBoxW(nullptr, L"Не удалось инициализировать приложение", L"Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Запускаем цикл обработки сообщений
    return waterEffect.Run();
} 