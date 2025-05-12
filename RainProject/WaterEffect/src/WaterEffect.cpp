#include "WaterEffect.h"
#include <chrono>
#include <algorithm>
#include <windowsx.h>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include <random>

// Идентификатор таймера анимации
constexpr int TIMER_ID = 1;

// Идентификатор таймера для тестовой волны
constexpr int TEST_WAVE_TIMER_ID = 2;

// Путь к лог-файлу
const std::string LOG_FILE_PATH = "./water_effect_log.txt";

// Конструктор
WaterEffect::WaterEffect() : 
    m_hwnd(nullptr),
    m_pD2DFactory(nullptr),
    m_pRenderTarget(nullptr),
    m_pBrush(nullptr),
    m_screenWidth(0),
    m_screenHeight(0),
    m_timerActive(false)
{
    // Инициализируем генератор случайных чисел
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    
    // Открываем файл для логов
    std::ofstream logFile(LOG_FILE_PATH, std::ios::out);
    if (logFile.is_open()) {
        logFile << "WaterEffect инициализирован" << std::endl;
        logFile.close();
    }
    
    // Удаляем блокирующий диалог
    // MessageBoxW(nullptr, L"WaterEffect инициализирован", L"Статус", MB_OK);
}

// Деструктор
WaterEffect::~WaterEffect()
{
    // Удаляем таймер, если он активен
    if (m_timerActive && m_hwnd) {
        KillTimer(m_hwnd, TIMER_ID);
        KillTimer(m_hwnd, TEST_WAVE_TIMER_ID);
        m_timerActive = false;
    }

    // Освобождаем ресурсы Direct2D
    DiscardGraphicsResources();

    // Освобождаем фабрику Direct2D
    if (m_pD2DFactory) {
        m_pD2DFactory->Release();
        m_pD2DFactory = nullptr;
    }
}

// Инициализация приложения
bool WaterEffect::Initialize(HINSTANCE hInstance)
{
    // Регистрируем класс окна
    if (!RegisterWindowClass(hInstance)) {
        MessageBoxW(nullptr, L"Не удалось зарегистрировать класс окна", L"Ошибка", MB_OK | MB_ICONERROR);
        return false;
    }

    // Получаем размеры экрана
    m_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    m_screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Создаем окно
    if (!CreateAppWindow()) {
        MessageBoxW(nullptr, L"Не удалось создать окно", L"Ошибка", MB_OK | MB_ICONERROR);
        return false;
    }

    // Инициализируем Direct2D
    if (!InitializeDirect2D()) {
        MessageBoxW(nullptr, L"Не удалось инициализировать Direct2D", L"Ошибка", MB_OK | MB_ICONERROR);
        return false;
    }

    // Регистрируем устройство для получения Raw Input
    RAWINPUTDEVICE rid[1];
    rid[0].usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
    rid[0].usUsage = 0x02;              // HID_USAGE_GENERIC_MOUSE
    rid[0].dwFlags = RIDEV_INPUTSINK;   // Получать сообщения даже когда окно не активно
    rid[0].hwndTarget = m_hwnd;         // Окно, которое будет получать сообщения

    if (!RegisterRawInputDevices(rid, 1, sizeof(rid[0]))) {
        std::ofstream logFile(LOG_FILE_PATH, std::ios::app);
        if (logFile.is_open()) {
            logFile << "Не удалось зарегистрировать устройство Raw Input. Код ошибки: " << GetLastError() << std::endl;
            logFile.close();
        }
    } else {
        std::ofstream logFile(LOG_FILE_PATH, std::ios::app);
        if (logFile.is_open()) {
            logFile << "Устройство Raw Input зарегистрировано успешно" << std::endl;
            logFile.close();
        }
    }

    return true;
}

// Запуск цикла обработки сообщений
int WaterEffect::Run()
{
    // Показываем окно и обновляем его
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    // Запускаем таймер для анимации
    if (SetTimer(m_hwnd, TIMER_ID, UPDATE_INTERVAL, nullptr) == 0) {
        MessageBoxW(nullptr, L"Не удалось запустить таймер анимации", L"Ошибка", MB_OK | MB_ICONERROR);
    }
    
    // Запускаем таймер для создания тестовых волн - уменьшаем интервал до 1 секунды
    if (SetTimer(m_hwnd, TEST_WAVE_TIMER_ID, 1000, nullptr) == 0) {
        MessageBoxW(nullptr, L"Не удалось запустить таймер тестовых волн", L"Ошибка", MB_OK | MB_ICONERROR);
    }
    
    m_timerActive = true;

    // Записываем в лог
    std::ofstream logFile(LOG_FILE_PATH, std::ios::app);
    if (logFile.is_open()) {
        logFile << "Приложение запущено и окно показано" << std::endl;
        logFile.close();
    }

    // Добавляем ручное создание первой волны для тестирования
    CreateWave(static_cast<float>(m_screenWidth) / 2, static_cast<float>(m_screenHeight) / 2);
    
    // Удаляем блокирующий диалог
    // MessageBoxW(nullptr, L"Первая волна создана", L"Статус", MB_OK);

    // Стандартный цикл обработки сообщений
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Останавливаем таймер, если он был запущен
    if (m_timerActive) {
        KillTimer(m_hwnd, TIMER_ID);
        KillTimer(m_hwnd, TEST_WAVE_TIMER_ID);
        m_timerActive = false;
    }

    return static_cast<int>(msg.wParam);
}

// Регистрация класса окна
bool WaterEffect::RegisterWindowClass(HINSTANCE hInstance)
{
    // Определяем класс окна
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WaterEffect::WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr; // Прозрачный фон
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"WaterEffectWindowClass";
    wcex.hIconSm = nullptr;

    // Регистрируем класс окна
    return RegisterClassExW(&wcex) != 0;
}

// Создание окна
bool WaterEffect::CreateAppWindow()
{
    // Создаем окно с расширенными стилями для прозрачности
    m_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, // Добавляем WS_EX_TOOLWINDOW
        L"WaterEffectWindowClass",     // Имя класса
        L"Water Effect",               // Заголовок окна
        WS_POPUP,                      // Стиль окна (без рамки)
        0, 0,                          // Позиция (x, y)
        m_screenWidth, m_screenHeight, // Размеры (ширина, высота)
        nullptr,                       // Родительское окно
        nullptr,                       // Меню
        GetModuleHandle(nullptr),      // Экземпляр приложения
        this                           // Указатель на класс для WindowProc
    );

    if (!m_hwnd) {
        // Оставляем сообщение об ошибке, так как это критично
        MessageBoxW(nullptr, L"Не удалось создать окно", L"Ошибка", MB_OK | MB_ICONERROR);
        std::ofstream logFile(LOG_FILE_PATH, std::ios::app);
        if (logFile.is_open()) {
            logFile << "Не удалось создать окно. Код ошибки: " << GetLastError() << std::endl;
            logFile.close();
        }
        return false;
    }

    // Устанавливаем прозрачность окна - увеличиваем до 10 для лучшей видимости
    SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 10, LWA_COLORKEY);

    // Удаляем блокирующий диалог
    // MessageBoxW(nullptr, L"Окно создано успешно", L"Статус", MB_OK);
    
    // Записываем в лог
    std::ofstream logFile(LOG_FILE_PATH, std::ios::app);
    if (logFile.is_open()) {
        logFile << "Окно создано успешно" << std::endl;
        logFile.close();
    }

    return true;
}

// Инициализация Direct2D
bool WaterEffect::InitializeDirect2D()
{
    // Создаем фабрику Direct2D
    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        &m_pD2DFactory
    );

    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Не удалось создать фабрику Direct2D", L"Ошибка", MB_OK | MB_ICONERROR);
        return false;
    }

    // Сразу создаем графические ресурсы
    if (!CreateGraphicsResources()) {
        MessageBoxW(nullptr, L"Не удалось создать графические ресурсы", L"Ошибка", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

// Создание графических ресурсов
bool WaterEffect::CreateGraphicsResources()
{
    HRESULT hr = S_OK;

    // Если цель рендеринга еще не создана
    if (!m_pRenderTarget) {
        // Определяем размеры окна
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

        // Создаем цель рендеринга для окна
        hr = m_pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            ),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &m_pRenderTarget
        );

        // Создаем кисть для рисования
        if (SUCCEEDED(hr)) {
            hr = m_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White),
                &m_pBrush
            );
        }
    }

    return SUCCEEDED(hr);
}

// Освобождение графических ресурсов
void WaterEffect::DiscardGraphicsResources()
{
    // Освобождаем кисть
    if (m_pBrush) {
        m_pBrush->Release();
        m_pBrush = nullptr;
    }

    // Освобождаем цель рендеринга
    if (m_pRenderTarget) {
        m_pRenderTarget->Release();
        m_pRenderTarget = nullptr;
    }
}

// Обновление анимации
void WaterEffect::Update()
{
    // Записываем в лог
    std::ofstream logFile(LOG_FILE_PATH, std::ios::app);
    if (logFile.is_open()) {
        logFile << "Update: волн = " << m_waves.size() << std::endl;
    }

    // Рассчитываем время, прошедшее с предыдущего кадра
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - lastTime).count();
    lastTime = currentTime;

    // Ограничиваем deltaTime для предотвращения скачков при отладке
    deltaTime = std::min(deltaTime, 0.1f);

    // Обновляем все активные волны
    for (auto it = m_waves.begin(); it != m_waves.end();) {
        // Увеличиваем радиус волны
        it->radius += it->speed * deltaTime;

        // Обновляем прозрачность по мере увеличения радиуса
        it->opacity = 1.0f - (it->radius / it->maxRadius);

        // Удаляем волны, которые стали полностью прозрачными
        if (it->opacity <= 0.0f || it->radius >= it->maxRadius) {
            it = m_waves.erase(it);
            if (logFile.is_open()) {
                logFile << "Волна удалена" << std::endl;
            }
        } else {
            ++it;
        }
    }

    // Перерисовываем сцену
    InvalidateRect(m_hwnd, nullptr, FALSE);

    if (logFile.is_open()) {
        logFile << "InvalidateRect вызван" << std::endl;
        logFile.close();
    }
}

// Отрисовка сцены
void WaterEffect::Render()
{
    // Записываем в лог
    std::ofstream logFile(LOG_FILE_PATH, std::ios::app);
    if (logFile.is_open()) {
        logFile << "Render: начало отрисовки" << std::endl;
    }

    // Создаем графические ресурсы, если они еще не созданы
    if (!m_pRenderTarget) {
        HRESULT hr = CreateGraphicsResources();
        if (FAILED(hr)) {
            MessageBoxW(nullptr, L"Не удалось создать ресурсы рендеринга", L"Ошибка", MB_OK);
            return;
        }
    }

    // Если ресурсы созданы успешно
    HRESULT hr = S_OK;

    // Начинаем отрисовку
    m_pRenderTarget->BeginDraw();
    
    // Очищаем фон (полностью прозрачный)
    m_pRenderTarget->Clear(D2D1::ColorF(0, 0, 0, 0));

    // Отрисовываем все активные волны
    if (logFile.is_open()) {
        logFile << "Отрисовка " << m_waves.size() << " волн" << std::endl;
    }

    for (const auto& wave : m_waves) {
        // Убираем обводку - просто заполняем круг полупрозрачным голубым цветом
        m_pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::DeepSkyBlue, wave.opacity * 0.3f));
        m_pRenderTarget->FillEllipse(
            D2D1::Ellipse(D2D1::Point2F(wave.x, wave.y), wave.radius, wave.radius),
            m_pBrush
        );
        
        // Дополнительно рисуем более яркий голубой круг в центре для эффекта глубины
        m_pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::LightBlue, wave.opacity * 0.5f));
        m_pRenderTarget->FillEllipse(
            D2D1::Ellipse(D2D1::Point2F(wave.x, wave.y), wave.radius * 0.6f, wave.radius * 0.6f),
            m_pBrush
        );
    }

    // Завершаем отрисовку
    hr = m_pRenderTarget->EndDraw();
    
    if (logFile.is_open()) {
        logFile << "EndDraw: " << (SUCCEEDED(hr) ? "успешно" : "ошибка") << std::endl;
        logFile.close();
    }
    
    // Если произошла ошибка, освобождаем ресурсы
    if (FAILED(hr) && hr == (HRESULT)D2DERR_RECREATE_TARGET) {
        DiscardGraphicsResources();
    }
}

// Создание новой волны в указанной точке
void WaterEffect::CreateWave(float x, float y)
{
    // Записываем в лог
    std::ofstream logFile(LOG_FILE_PATH, std::ios::app);
    if (logFile.is_open()) {
        logFile << "Создание волны в точке: X=" << x << ", Y=" << y << std::endl;
        logFile.close();
    }

    // Создаем новую волну
    Wave wave;
    wave.x = x;
    wave.y = y;
    wave.radius = 0.0f;
    wave.maxRadius = MAX_WAVE_RADIUS;
    wave.opacity = 1.0f;
    wave.speed = WAVE_SPEED * 1.5f; // Увеличиваем скорость для большей заметности

    // Добавляем волну в список
    m_waves.push_back(wave);
    
    // Принудительно вызываем перерисовку
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

// Статическая функция обработки сообщений окна
LRESULT CALLBACK WaterEffect::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WaterEffect* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        // Получаем указатель на экземпляр класса из данных создания окна
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<WaterEffect*>(pCreate->lpCreateParams);
        
        // Сохраняем указатель в пользовательских данных окна
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        // Получаем указатель из пользовательских данных окна
        pThis = reinterpret_cast<WaterEffect*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    // Проверяем наличие экземпляра класса
    if (pThis) {
        // Передаем сообщение экземпляру класса
        return pThis->HandleMessage(hwnd, uMsg, wParam, lParam);
    } else {
        // Если экземпляр не найден, обрабатываем сообщение стандартным образом
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

// Обработчик сообщений для текущего экземпляра
LRESULT WaterEffect::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Открываем лог для каждого сообщения
    std::ofstream logFile(LOG_FILE_PATH, std::ios::app);
    if (logFile.is_open()) {
        logFile << "Получено сообщение: " << uMsg;
        
        // Добавляем имя сообщения для удобства отладки
        switch(uMsg) {
            case WM_PAINT: logFile << " (WM_PAINT)"; break;
            case WM_LBUTTONDOWN: logFile << " (WM_LBUTTONDOWN)"; break;
            case WM_MOUSEMOVE: logFile << " (WM_MOUSEMOVE)"; break;
            case WM_TIMER: logFile << " (WM_TIMER)"; break;
            case WM_DESTROY: logFile << " (WM_DESTROY)"; break;
            case WM_INPUT: logFile << " (WM_INPUT)"; break;
        }
        
        logFile << std::endl;
    }

    switch (uMsg) {
        // Обработка перерисовки
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            Render();
            EndPaint(hwnd, &ps);
            
            if (logFile.is_open()) {
                logFile << "Выполнена отрисовка" << std::endl;
            }
            return 0;
        }

        // Обработка клика мыши через стандартное сообщение
        case WM_LBUTTONDOWN: {
            // Получаем координаты клика
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            
            if (logFile.is_open()) {
                logFile << "WM_LBUTTONDOWN: x=" << xPos << ", y=" << yPos << std::endl;
            }
            
            // Создаем волну в точке клика
            CreateWave(static_cast<float>(xPos), static_cast<float>(yPos));
            
            // Передаем сообщение дальше
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }

        // Обработка Raw Input сообщений (для перехвата событий мыши глобально)
        case WM_INPUT: {
            UINT dwSize = 0;
            
            // Сначала получаем размер структуры
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
            
            if (dwSize > 0) {
                std::vector<BYTE> rawdata(dwSize);
                
                // Теперь получаем саму структуру
                if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawdata.data(), &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
                    RAWINPUT* raw = (RAWINPUT*)rawdata.data();
                    
                    // Проверяем, что это события от мыши и что нажата левая кнопка
                    if (raw->header.dwType == RIM_TYPEMOUSE && 
                        raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
                        
                        // Получаем координаты курсора
                        POINT pt;
                        GetCursorPos(&pt);
                        
                        // Записываем в лог
                        if (logFile.is_open()) {
                            logFile << "WM_INPUT: Клик мыши в точке x=" << pt.x << ", y=" << pt.y << std::endl;
                        }
                        
                        // Создаем волну в точке клика
                        CreateWave(static_cast<float>(pt.x), static_cast<float>(pt.y));
                    }
                }
            }
            
            // Передаем сообщение дальше
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }

        // Обработка таймера для анимации
        case WM_TIMER: {
            if (wParam == TIMER_ID) {
                // Обновляем анимацию
                Update();
                
                if (logFile.is_open()) {
                    logFile << "Обновление анимации по таймеру" << std::endl;
                }
            } else if (wParam == TEST_WAVE_TIMER_ID) {
                // Создаем тестовую волну в случайной точке экрана
                float x = static_cast<float>(std::rand() % m_screenWidth);
                float y = static_cast<float>(std::rand() % m_screenHeight);
                
                if (logFile.is_open()) {
                    logFile << "Создание тестовой волны по таймеру x=" << x << ", y=" << y << std::endl;
                }
                
                CreateWave(x, y);
                
                // Обновляем окно, чтобы сразу отобразить волну
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        // Обработка клавиши Escape для выхода
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hwnd);
                return 0;
            }
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);

        // Обработка уничтожения окна
        case WM_DESTROY: {
            if (logFile.is_open()) {
                logFile << "Окно уничтожено" << std::endl;
            }
            
            // Останавливаем таймер
            if (m_timerActive) {
                KillTimer(hwnd, TIMER_ID);
                KillTimer(hwnd, TEST_WAVE_TIMER_ID);
                m_timerActive = false;
            }
            
            // Освобождаем ресурсы Direct2D
            DiscardGraphicsResources();
            
            // Уведомляем систему о завершении работы
            PostQuitMessage(0);
            return 0;
        }

        default:
            if (logFile.is_open()) {
                logFile << "Сообщение обработано по умолчанию" << std::endl;
            }
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
} 