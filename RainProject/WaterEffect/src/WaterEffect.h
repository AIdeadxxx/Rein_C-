#pragma once

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <dwmapi.h>
#include <vector>
#include <memory>

// Структура для хранения информации о волне
struct Wave {
    float x;           // Координата X центра волны
    float y;           // Координата Y центра волны
    float radius;      // Текущий радиус волны
    float maxRadius;   // Максимальный радиус волны
    float opacity;     // Текущая прозрачность волны (1.0f - непрозрачная, 0.0f - полностью прозрачная)
    float speed;       // Скорость расширения волны
};

class WaterEffect {
public:
    WaterEffect();
    ~WaterEffect();

    // Инициализация окна и графического контекста
    bool Initialize(HINSTANCE hInstance);
    
    // Запуск цикла обработки сообщений
    int Run();

    // Создание новой волны в указанной точке
    void CreateWave(float x, float y);

private:
    // Регистрация класса окна
    bool RegisterWindowClass(HINSTANCE hInstance);
    
    // Создание окна
    bool CreateAppWindow();
    
    // Инициализация Direct2D
    bool InitializeDirect2D();
    
    // Создание графических ресурсов
    bool CreateGraphicsResources();
    
    // Освобождение графических ресурсов
    void DiscardGraphicsResources();
    
    // Обновление анимации
    void Update();
    
    // Отрисовка сцены
    void Render();
    
    // Статическая функция для обработки сообщений окна
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Обработчик сообщений для текущего экземпляра
    LRESULT HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_hwnd;                               // Дескриптор окна
    ID2D1Factory* m_pD2DFactory;               // Фабрика Direct2D
    ID2D1HwndRenderTarget* m_pRenderTarget;    // Цель рендеринга
    ID2D1SolidColorBrush* m_pBrush;            // Кисть для рисования

    std::vector<Wave> m_waves;                 // Список активных волн
    
    // Размеры экрана
    int m_screenWidth;
    int m_screenHeight;
    
    // Параметры волн
    static constexpr float MAX_WAVE_RADIUS = 300.0f;    // Максимальный радиус волны
    static constexpr float WAVE_SPEED = 150.0f;         // Скорость расширения волны (пикселей в секунду)
    static constexpr float WAVE_FADE_SPEED = 0.8f;      // Скорость затухания волны
    
    // Частота обновления анимации (мс)
    static constexpr int UPDATE_INTERVAL = 16;          // ~60 FPS
    
    // Флаг для отслеживания активности таймера
    bool m_timerActive;
}; 