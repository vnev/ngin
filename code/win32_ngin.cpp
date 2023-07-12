#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define BYTES_PER_PIXEL 4

struct offscreen_buffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Pitch;
    int Height;
    int BytesPerPixel;
};

global_variable offscreen_buffer GlobalBackbuffer;
global_variable bool Running;

internal void RenderGradient(offscreen_buffer Buffer, int BlueOffset, int GreenOffset)
{
    uint8_t *Row = (uint8_t *)Buffer.Memory;
    
    for (int y = 0; y < Buffer.Height; y++)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for (int x = 0; x < Buffer.Width; x++)
        {
            uint8_t Blue = (x + BlueOffset);
            uint8_t Green = (y + GreenOffset);
            
            *Pixel++ = ((Green << 8) | Blue);
        }
        
        Row += Buffer.Pitch;
    }
    
}

internal void ResizeDIBSection(offscreen_buffer *Buffer, int Width, int Height)
{
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }
    
    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = BYTES_PER_PIXEL;
    
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    // this is so that StretchDIBits uses a top-down approach to drawing the row of pixels
    // if its negative, top-down, if positive- bottom-up
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    int MemoryNeeded = (Buffer->BytesPerPixel * Width * Height);
    Buffer->Memory = VirtualAlloc(0, MemoryNeeded, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Buffer->BytesPerPixel * Width;
    
    //RenderGradient();
}

internal void UnloadBufferToWindow(HDC DeviceContext, RECT *WindowRect, offscreen_buffer Buffer, int X, int Y, int Width, int Height)
{
    int WindowWidth = WindowRect->right - WindowRect->left;
    int WindowHeight = WindowRect->bottom - WindowRect->top;
    StretchDIBits(DeviceContext, 0, 0, Buffer.Width, Buffer.Height, 0, 0, WindowWidth, WindowHeight, Buffer.Memory, &Buffer.Info, DIB_RGB_COLORS, SRCCOPY);
    //StretchDIBits(DeviceContext, X, Y, Width, Height, X, Y, Width, Height, BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result;
    switch (uMsg)
    {
        case WM_SIZE:
        {
            RECT ClientRect;
            GetClientRect(hWnd, &ClientRect);
            int width = ClientRect.right - ClientRect.left;
            int height = ClientRect.bottom - ClientRect.top;
            ResizeDIBSection(&GlobalBackbuffer, width, height);
            
            OutputDebugStringA("WM_SIZE\n");
        } break;
        
        case WM_DESTROY:
        {
            Running = false;
            OutputDebugStringA("WM_DESTOY\n");
        } break;
        
        case WM_CLOSE:
        {
            Running = false;
            //PostQuitMessage(0);
            OutputDebugStringA("WM_CLOSE\n");
        } break;
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_PAINT:
        {
            OutputDebugStringA("Painting...\n");
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            int minX = ps.rcPaint.left;
            int maxX = ps.rcPaint.right;
            int minY = ps.rcPaint.top;
            int maxY = ps.rcPaint.bottom;
            int width = maxX - minX;
            int height = maxY - minY;
            
            RECT ClientRect;
            GetClientRect(hWnd, &ClientRect);
            
            UnloadBufferToWindow(hdc, &ClientRect, GlobalBackbuffer, minX, minY, width, height);
            EndPaint(hWnd, &ps);
        } break;
        
        default:
        {
            Result = DefWindowProcA(hWnd, uMsg, wParam, lParam);
        } break;
    }
    
    return Result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSA WindowClass = {};
    
    //TODO: check if still relevant or if there's something more updated we can use
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = hInstance;
    WindowClass.lpszClassName = "NginWindowClass";
    
    if(RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "Ngin",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT, CW_USEDEFAULT, 0,
                                      0, hInstance, 0);
        
        if (Window)
        {
            MSG Message;
            Running = true;
            int XOffset = 0;
            int YOffset = 0;
            while (Running)
            {
                //MSG Message;
                //BOOL Result = PeekMessageA(&Message, 0, 0, 0, PM_REMOVE);
                
                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        Running = false;
                    }
                    // do window related thingies
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                
                RenderGradient(GlobalBackbuffer, XOffset, YOffset);
                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);
                int WindowWidth = ClientRect.right - ClientRect.left;
                int WindowHeight = ClientRect.bottom - ClientRect.top;
                UnloadBufferToWindow(DeviceContext, &ClientRect, GlobalBackbuffer, 0, 0, WindowWidth, WindowHeight);
                ReleaseDC(Window, DeviceContext);
                ++XOffset;
                --YOffset;
            }
        }
        else
        {
            OutputDebugStringA("Failed to create window!\n");
        }
    }
    else
    {
        OutputDebugStringA("Failed to register window class!\n");
    }
    
    return (0);
}