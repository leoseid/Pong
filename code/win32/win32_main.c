/* TODO(leo):

 - Error logging system. Display a message box for fatal errors (ex. window creation failed), and a simple os_print for less important ones.
 - See if we can make the window not blink in black when we resize, due to the WM_PAINT message.

*/

#if !defined(_M_X64) && !defined(__x86_64__)
#error Only x64 target support.
#endif

// NOTE(leo): Just in case someone removes the /D MSVC from build.bat, since there is no pre-defined macro to identify the Microsoft Compiler, because _MSC_VER is also
// defined when using Clang on Windows.
#ifndef MSVC
#if !defined(__clang__) && !defined(__GNUC__) && defined(_MSC_VER)
#define MSVC
#endif
#endif

#include "win32_crt.c" // NOTE(leo): Windows CRT replacement since we aren't linking with it. First included so that we can use memset and memcpy anywhere.

#include "../includes.h"    // NOTE(leo): Cross-platform code.
#include "win32_includes.h" // NOTE(leo): Windows-specific code.

GLOBAL struct // NOTE(leo): This GLOBAL is only global because the way Windows handle window messages doesn't allow us to pass parameters. It uses a callback function.
{
    HWND    window_handle;
    HDC     window_dc;
    HBITMAP bitmap_handle;
    HDC     bitmap_dc;
    RECT    fullscreen_rect;
    RECT    windowed_rect;
    s32     blit_dest_x;
    s32     blit_dest_y;
    s32     last_client_width;
    s32     last_client_height;
    
} g_win32 = {0};

GLOBAL f32 g_cpu_ticks_per_second = 0.0f;

INTERNAL void
win32_resize_graphics(s32 new_width, s32 new_height)
{
    if((new_width != g_back_buffer.width) || (new_height != g_back_buffer.height))
    {
        BITMAPINFO bitmap_info = {0};
        
        bitmap_info.bmiHeader.biSize        =  sizeof(bitmap_info.bmiHeader);
        bitmap_info.bmiHeader.biWidth       =  new_width;
        bitmap_info.bmiHeader.biHeight      = -new_height; // NOTE(leo): Negative for top-down
        bitmap_info.bmiHeader.biPlanes      =  1;
        bitmap_info.bmiHeader.biBitCount    =  32;
        bitmap_info.bmiHeader.biCompression =  BI_RGB;
        // NOTE(leo): There are more members that are beeing set to 0 and NULL.
        
        HDC temp_bitmap_dc = NULL;
        
        void *temp_pixels;
        HBITMAP temp_bitmap = CreateDIBSection(g_win32.window_dc, &bitmap_info, DIB_RGB_COLORS, &temp_pixels, NULL, 0);
        
        if(temp_bitmap)
        {
            temp_bitmap_dc = CreateCompatibleDC(g_win32.window_dc);
            
            if(temp_bitmap_dc)
            {
                if(SelectObject(temp_bitmap_dc, temp_bitmap))
                {
                    if(g_win32.bitmap_handle)
                    {
                        ASSERT_FUNCTION(DeleteObject(g_win32.bitmap_handle));
                    }
                    if(g_win32.bitmap_dc)
                    {
                        ASSERT_FUNCTION(DeleteDC(g_win32.bitmap_dc));
                    }
                    
                    g_win32.bitmap_handle = temp_bitmap;
                    g_win32.bitmap_dc     = temp_bitmap_dc;
                    g_back_buffer.pixels  = temp_pixels;
                    
                    g_back_buffer.width        = new_width;
                    g_back_buffer.height       = new_height;
                    g_back_buffer.pixels_count = new_width * new_height;
                    g_back_buffer.aspect_ratio = (f32)new_width / (f32)new_height;
                    
#define ASPECT_RATIO_EPSILON 0.03f
                    ASSERT((g_back_buffer.aspect_ratio >= (TARGET_ASPECT_RATIO - ASPECT_RATIO_EPSILON)) &&
                           (g_back_buffer.aspect_ratio <= (TARGET_ASPECT_RATIO + ASPECT_RATIO_EPSILON)));
#undef ASPECT_RATIO_EPSILON
                    
                    temp_bitmap    = NULL;
                    temp_bitmap_dc = NULL;
                    temp_pixels    = NULL;
                }
                else
                {
                    // TODO(leo): SelectObject failed.
                    INVALID_CODE_PATH;
                }
            }
            else
            {
                // TODO(leo): CreateCompatibleDC failed.
                INVALID_CODE_PATH;
            }
        }
        else
        {
            // TODO(leo): CreateDIBSection failed.
            INVALID_CODE_PATH;
        }
        
        if(temp_bitmap)
        {
            ASSERT_FUNCTION(DeleteObject(temp_bitmap));
        }
        if(temp_bitmap_dc)
        {
            ASSERT_FUNCTION(DeleteDC(temp_bitmap_dc));
        }
    }
}

INTERNAL void
win32_toggle_fullscreen(void)
{
    RECT current_rect;
    if(GetWindowRect(g_win32.window_handle, &current_rect))
    {
        int x, y, w, h;
        
        if((current_rect.left  == g_win32.fullscreen_rect.left)  && (current_rect.top  == g_win32.fullscreen_rect.top) &&
           (current_rect.right == g_win32.fullscreen_rect.right) && (current_rect.left == g_win32.fullscreen_rect.left))
        {
            x = g_win32.windowed_rect.left;
            y = g_win32.windowed_rect.top;
            w = g_win32.windowed_rect.right  - x;
            h = g_win32.windowed_rect.bottom - y;
        }
        else
        {
            x = g_win32.fullscreen_rect.left;
            y = g_win32.fullscreen_rect.top;
            w = g_win32.fullscreen_rect.right  - x;
            h = g_win32.fullscreen_rect.bottom - y;
        }
        
        if(MoveWindow(g_win32.window_handle, x, y, w, h, true))
        {
            g_win32.windowed_rect = current_rect;
        }
        else
        {
            os_print(STRING8_LITERAL("MoveWindow failed on win32_toggle_fullscreen.\n"));
            INVALID_CODE_PATH;
        }
    }
    else
    {
        os_print(STRING8_LITERAL("GetClientRect failed on win32_toggle_fullscreen.\n"));
        INVALID_CODE_PATH;
    }
}

INTERNAL LRESULT CALLBACK
win32_window_callback(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;
    
    switch(message)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        } break;
        
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *minmax_info = (MINMAXINFO *)l_param;
            // NOTE(leo): It seems that only the ptMaxTrackSize.y is less than the required window size for a fullscreen window, and it also seems that it is always 19 less,
            // so we can just set it to be 19 more than the current maximum, but let's keep an eye on it.
            minmax_info->ptMaxTrackSize.y = GetSystemMetrics(SM_CYMAXTRACK) + 19;
        } break;
        
        case WM_SIZE:
        {
            s32 client_width  = LOWORD(l_param);
            s32 client_height = HIWORD(l_param);
            
            if((client_width && client_height) && ((client_width != g_win32.last_client_width) || (client_height != g_win32.last_client_height)))
            {
                f32 aspect_ratio = (f32)client_width / (f32)client_height;
                
                s32 width_to_render  = client_width;
                s32 height_to_render = client_height;
                
                g_win32.blit_dest_x = 0;
                g_win32.blit_dest_y = 0;
                
                if(aspect_ratio > TARGET_ASPECT_RATIO)
                {
                    width_to_render = round_f32_to_s32_up(((f32)client_height * TARGET_ASPECT_RATIO_NUMERATOR) / TARGET_ASPECT_RATIO_DENOMINATOR);
                    g_win32.blit_dest_x = round_f32_to_s32_up(((f32)client_width - (f32)width_to_render) / 2.0f);
                }
                else if(aspect_ratio < TARGET_ASPECT_RATIO)
                {
                    height_to_render = round_f32_to_s32_up(((f32)client_width * TARGET_ASPECT_RATIO_DENOMINATOR) / TARGET_ASPECT_RATIO_NUMERATOR);
                    g_win32.blit_dest_y = round_f32_to_s32_up(((f32)client_height - (f32)height_to_render) / 2.0f);
                }
                
                win32_resize_graphics(width_to_render, height_to_render);
                
                g_win32.last_client_width  = client_width;
                g_win32.last_client_height = client_height;
            }
            
        } break;
        
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        {
            u32 vk_code     =  (u32)w_param;
            b32 is_down     = !CHECK_BIT(l_param, 31);
            //b32 was_down    =  CHECK_BIT(l_param, 30);
            b32 alt_is_down =  CHECK_BIT(l_param, 29);
            
            if(((vk_code == VK_F11) || (vk_code == VK_RETURN && alt_is_down)) && is_down)
            {
                win32_toggle_fullscreen();
            }
            else if(vk_code == 'W')
            {
                g_is_key_down[KEY_W]  = is_down;
            }
            else if(vk_code == 'S')
            {
                g_is_key_down[KEY_S]  = is_down;
            }
            else if(vk_code == VK_UP)
            {
                g_is_key_down[KEY_UP]  = is_down;
            }
            else if(vk_code == VK_DOWN)
            {
                g_is_key_down[KEY_DOWN]  = is_down;
            }
            else if(vk_code == VK_RETURN)
            {
                g_is_key_down[KEY_ENTER]  = is_down;
            }
            
        } break;
        
        case WM_SYSKEYUP:
        {
            u32 vk_code     = (u32)w_param;
            b32 alt_is_down = CHECK_BIT(l_param, 29);
            
            if(vk_code == VK_F4 && alt_is_down)
            {
                PostQuitMessage(0);
            }
            
        } break;
        
        case WM_MOUSEMOVE:
        {
            SetCursor(NULL);
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window_handle, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int w = paint.rcPaint.right - x;
            int h = paint.rcPaint.bottom - y;
            
            ASSERT_FUNCTION(PatBlt(device_context, x, y, w, h, BLACKNESS));
            ASSERT_FUNCTION(BitBlt(g_win32.window_dc,
                                   g_win32.blit_dest_x,
                                   g_win32.blit_dest_y,
                                   g_back_buffer.width,
                                   g_back_buffer.height,
                                   g_win32.bitmap_dc,
                                   0,
                                   0,
                                   SRCCOPY));
            
            EndPaint(window_handle, &paint);
        } break;
        
        default:
        {
            result = DefWindowProcA(window_handle, message, w_param, l_param);
        } break;
    }
    
    return result;
}

INTERNAL b32
win32_create_window(void)
{
    b32 result = false;
    
    if(!g_win32.window_handle && !g_win32.window_dc)
    {
        HINSTANCE exe_instance = GetModuleHandleA(NULL); // NOTE(leo): Same as the 'hInstance' parameter from WinMain.
        if(exe_instance)
        {
            WNDCLASSEXA window_class = {0};
            
            window_class.cbSize        = sizeof(window_class);
            window_class.style         = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
            window_class.lpfnWndProc   = win32_window_callback;
            window_class.cbClsExtra    = 0;
            window_class.cbWndExtra    = 0;
            window_class.hInstance     = exe_instance;
            window_class.hIcon         = NULL;
            window_class.hCursor       = NULL;
            window_class.hbrBackground = NULL;
            window_class.lpszMenuName  = NULL;
            window_class.lpszClassName = PROGRAM_NAME" Window Class";
            window_class.hIconSm       = NULL;
            
            if(RegisterClassExA(&window_class))
            {
                // NOTE(leo): Width and height of the primary screen;
                int screen_width_px  = GetSystemMetrics(SM_CXSCREEN);
                int screen_height_px = GetSystemMetrics(SM_CYSCREEN);
                
                if(screen_width_px && screen_height_px)
                {
                    s32 windowed_init_width  = 1280;
                    s32 windowed_init_height = 720;
                    
                    // NOTE(leo): Centralizing the window in the screen.
                    g_win32.windowed_rect.left   = (screen_width_px  / 2) - (windowed_init_width  / 2);
                    g_win32.windowed_rect.top    = (screen_height_px / 2) - (windowed_init_height / 2);
                    g_win32.windowed_rect.right  = g_win32.windowed_rect.left + windowed_init_width;
                    g_win32.windowed_rect.bottom = g_win32.windowed_rect.top  + windowed_init_height;
                    
                    g_win32.fullscreen_rect.left   = 0;
                    g_win32.fullscreen_rect.top    = 0;
                    g_win32.fullscreen_rect.right  = screen_width_px;
                    g_win32.fullscreen_rect.bottom = screen_height_px;
                    
                    DWORD style = WS_OVERLAPPEDWINDOW;
                    //DWORD style = WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
                    
                    if(AdjustWindowRectEx(&g_win32.windowed_rect, style, false, 0) && AdjustWindowRectEx(&g_win32.fullscreen_rect, style, false, 0))
                    {
#ifdef DEVELOPMENT
                        int x = g_win32.windowed_rect.left;
                        int y = g_win32.windowed_rect.top;
                        int w = g_win32.windowed_rect.right  - x;
                        int h = g_win32.windowed_rect.bottom - y;
#else
                        int x = g_win32.fullscreen_rect.left;
                        int y = g_win32.fullscreen_rect.top;
                        int w = g_win32.fullscreen_rect.right  - x;
                        int h = g_win32.fullscreen_rect.bottom - y;
#endif
                        g_win32.window_handle = CreateWindowExA(0,
                                                                window_class.lpszClassName,
                                                                PROGRAM_NAME,
                                                                style,
                                                                x,
                                                                y,
                                                                w,
                                                                h,
                                                                NULL,
                                                                NULL,
                                                                exe_instance,
                                                                NULL);
                        if(g_win32.window_handle)
                        {
                            g_win32.window_dc = GetDC(g_win32.window_handle);
                            if(g_win32.window_dc)
                            {
                                result = true;
                            }
                            else
                            {
                                ASSERT_FUNCTION(DestroyWindow(g_win32.window_handle));
                                g_win32.window_handle = NULL;
                                // TODO(leo): GetDC failed.
                            }
                        }
                        else
                        {
                            // TODO(leo): CreateWindowExA failed.
                        }
                    }
                    else
                    {
                        // TODO(leo): AdjustWindowRectEx failed.
                    }
                }
                else
                {
                    // TODO(leo): GetSystemMetrics failed.
                }
            }
            else
            {
                // TODO(leo): RegisterClassExA failed.
            }
        }
        else
        {
            // TODO(leo): GetModuleHandleA failed.
        }
    }
    else
    {
        // TODO(leo): Already created.
    }
    
    return result;
}

INTERNAL inline s64
win32_get_cpu_tick(void)
{
    LARGE_INTEGER li_counter;
    ASSERT_FUNCTION(QueryPerformanceCounter(&li_counter)); // NOTE(leo): According to MSDN, will never fail on Windows XP or later.
    return li_counter.QuadPart;
}

INTERNAL inline f32
win32_get_seconds_elapsed(s64 init_tick, s64 end_tick)
{
    s64 delta = end_tick - init_tick;
    ASSERT(delta > 0);
    return (f32)delta / g_cpu_ticks_per_second;
}

#ifdef __cplusplus
#define COM_FUNCTION(com_object, function, ...) com_object->function(__VA_ARGS__)
#else
#define COM_FUNCTION(com_object, function, ...) com_object->lpVtbl->function(com_object, __VA_ARGS__)
#endif

typedef struct
{
    u64 running_sample_index;
    u32 samples_per_second;
    u32 buffer_size;
    u32 buffer_seconds;
    u16 channels_count;
    u16 bytes_per_sample;
    
} SoundSpecs;

INTERNAL b32
win32_init_directsound(HWND window_handle, LPDIRECTSOUNDBUFFER *secondary_buffer, SoundSpecs *sound_specs)
{
    b32 result = false;
    
    sound_specs->running_sample_index = 0; // NOTE(leo): Just making sure.
    
    HMODULE dsound_dll = LoadLibraryA("dsound.dll");
    if(dsound_dll)
    {
        typedef HRESULT WINAPI DSoundCreate(LPGUID lpGuid, LPDIRECTSOUND8* ppDS, LPUNKNOWN pUnkOuter);
        DSoundCreate *dsound_create = (DSoundCreate *)GetProcAddress(dsound_dll, "DirectSoundCreate8");
        
        if(dsound_create)
        {
            if(SUCCEEDED(CoInitialize(NULL)) && sound_specs)
            {
                LPDIRECTSOUND8 dsound_interface;
                HRESULT ds_result = dsound_create(NULL, &dsound_interface, NULL);
                if(SUCCEEDED(ds_result))
                {
                    ds_result = COM_FUNCTION(dsound_interface, SetCooperativeLevel, window_handle, DSSCL_PRIORITY);
                    if(SUCCEEDED(ds_result))
                    {
                        DSBUFFERDESC primary_buffer_desc = {0};
                        
                        primary_buffer_desc.dwSize  = sizeof(primary_buffer_desc);
                        primary_buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
                        
                        LPDIRECTSOUNDBUFFER primary_buffer;
                        
                        ds_result = COM_FUNCTION(dsound_interface, CreateSoundBuffer, &primary_buffer_desc, &primary_buffer, NULL);
                        if(SUCCEEDED(ds_result))
                        {
                            WAVEFORMATEX wave_format = {0};
                            
                            wave_format.wFormatTag      = WAVE_FORMAT_PCM;
                            wave_format.nChannels       = sound_specs->channels_count;
                            wave_format.nSamplesPerSec  = sound_specs->samples_per_second;
                            wave_format.wBitsPerSample  = sound_specs->bytes_per_sample * 8;
                            wave_format.nBlockAlign     = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
                            wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
                            
                            ds_result = COM_FUNCTION(primary_buffer, SetFormat, &wave_format);
                            if(SUCCEEDED(ds_result))
                            {
                                DSBUFFERDESC secondary_buffer_desc = {0};
                                
                                secondary_buffer_desc.dwSize        = sizeof(secondary_buffer_desc);
                                secondary_buffer_desc.dwFlags       = DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_GLOBALFOCUS|DSBCAPS_LOCHARDWARE;
                                secondary_buffer_desc.dwBufferBytes = wave_format.nAvgBytesPerSec * sound_specs->buffer_seconds;
                                secondary_buffer_desc.lpwfxFormat   = &wave_format;
                                
                                sound_specs->buffer_size = secondary_buffer_desc.dwBufferBytes;
                                sound_specs->bytes_per_sample = wave_format.nBlockAlign;
                                
                                ds_result = COM_FUNCTION(dsound_interface, CreateSoundBuffer, &secondary_buffer_desc, secondary_buffer, NULL);
                                // NOTE(leo): Trying to get hardware mixing.
                                if(SUCCEEDED(ds_result))
                                {
                                    result = true;
                                }
                                else if(ds_result == DSERR_UNSUPPORTED)
                                {
                                    secondary_buffer_desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_GLOBALFOCUS;
                                    
                                    ds_result = COM_FUNCTION(dsound_interface, CreateSoundBuffer, &secondary_buffer_desc, secondary_buffer, NULL);
                                    if(SUCCEEDED(ds_result))
                                    {
                                        result = true;
                                    }
                                    else
                                    {
                                        // TODO(leo): CreateSoundBuffer failed.
                                    }
                                }
                                else
                                {
                                    // TODO(leo): CreateSoundBuffer failed.
                                }
                            }
                            else
                            {
                                // TODO(leo): SetFormat failed.
                            }
                        }
                        else
                        {
                            // TODO(leo): CreateSoundBuffer failed.
                        }
                    }
                    else
                    {
                        // TODO(leo): SetCooperativeLevel failed.
                    }
                }
                else
                {
                    // TODO(leo): DirectSoundCreate8 failed.
                }
            }
            else
            {
                // TODO(leo): CoInitialize failed.
            }
        }
        else
        {
            // TODO(leo): GetProcAddress failed.
        }
    }
    else
    {
        // TODO(leo): LoadLibraryA failed.
    }
    
    if(result == true)
    {
        ASSERT_FUNCTION(SUCCEEDED(COM_FUNCTION((*secondary_buffer), Play, 0, 0, DSBPLAY_LOOPING)));
    }
    
    return result;
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

__declspec(noreturn) void __stdcall
WinMainCRTStartup(void)
{
    UINT exit_code = 1; // NOTE(leo): Considering the worst-case scenario where something fails.
    
    LARGE_INTEGER li_frequency;
    if(QueryPerformanceFrequency(&li_frequency)) // NOTE(leo): According to MSDN, should never fail on Windows XP or later.
    {
        g_cpu_ticks_per_second = (f32)li_frequency.QuadPart;
        
        if(win32_create_window())
        {
            int refresh_rate = GetDeviceCaps(GetDC(NULL), VREFRESH);
            ASSERT(refresh_rate > 1);
            f32 target_frame_seconds = 1.0f / (f32)refresh_rate;
            
            SoundSpecs sound_specs = {0};
            
            sound_specs.channels_count     = 2;
            sound_specs.samples_per_second = 44100;
            sound_specs.bytes_per_sample   = 2; // per channel. after init, it will become for both channels (4 bytes).
            sound_specs.buffer_seconds     = 2;
            
            LPDIRECTSOUNDBUFFER dsound_buffer;
            if(win32_init_directsound(g_win32.window_handle, &dsound_buffer, &sound_specs))
            {
                GameState game_state;
                game_state_init(&game_state);
                
                f32 last_frame_time_seconds = 0.0f; // TODO(leo): Initialize this to a aproximate right value so that the first frame is less wrong.
                
                ShowWindow(g_win32.window_handle, SW_SHOW);
                SetFocus(g_win32.window_handle);
                
                SetErrorMode(SEM_FAILCRITICALERRORS); // TODO(leo): Should we use SetThreadErrorMode as MSDN suggests instead?
                
                if(timeBeginPeriod(1) == TIMERR_NOERROR)
                {
                    MSG message;
                    while(true) // NOTE(leo): Main loop.
                    {
                        s64 frame_begin_tick = win32_get_cpu_tick();
                        
                        while(PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
                        {
                            if(message.message == WM_QUIT)
                            {
                                // TODO(leo): Ask for user confirmation. Save.
                                exit_code = (UINT)message.wParam;
                                goto break_main_loop;
                            }
                            else
                            {
                                TranslateMessage(&message);
                                DispatchMessageA(&message);
                            }
                        }
                        
                        game_update_and_render(&game_state, last_frame_time_seconds);
                        
                        
                        
                        
                        
                        
                        s64 before_write_audio_tick = win32_get_cpu_tick();
                        f32 before_write_audio_seconds_elapsed = win32_get_seconds_elapsed(frame_begin_tick, before_write_audio_tick);
                        f32 seconds_until_blit = target_frame_seconds - before_write_audio_seconds_elapsed;
                        ASSERT(seconds_until_blit > 0.0f);
                        
                        os_printf(STRING8_LITERAL("last frame seconds: %f32 | before write audio seconds: %f32 | seconds until blit: %f32\n"),
                                  last_frame_time_seconds, before_write_audio_seconds_elapsed, seconds_until_blit);
                        
                        DWORD buffer_status;
                        ASSERT_FUNCTION(SUCCEEDED(COM_FUNCTION(dsound_buffer, GetStatus, &buffer_status)));
                        
                        // NOTE(leo): Making sure that the buffer is not lost.
                        ASSERT(!(buffer_status & DSBSTATUS_BUFFERLOST));
                        
                        DWORD play_cursor_byte, write_cursor_byte;
                        ASSERT_FUNCTION(SUCCEEDED(COM_FUNCTION(dsound_buffer, GetCurrentPosition, &play_cursor_byte, &write_cursor_byte)));
                        
                        DWORD cursors_diff = write_cursor_byte > play_cursor_byte ?
                            write_cursor_byte - play_cursor_byte : (sound_specs.buffer_size - play_cursor_byte) + write_cursor_byte;
                        
                        f32 cursors_latency_ms = ((f32)cursors_diff * 1000.0f) / ((f32)sound_specs.buffer_size / (f32)sound_specs.buffer_seconds);
                        
                        if(cursors_latency_ms < seconds_until_blit)
                        {
                            
                        }
                        else
                        {
                            
                        }
                        
                        DWORD byte_to_lock, bytes_to_write = 8000;
                        
                        if(sound_specs.running_sample_index)
                        {
                            byte_to_lock = (sound_specs.running_sample_index / sound_specs.bytes_per_sample) % sound_specs.buffer_size;
                        }
                        else
                        {
                            byte_to_lock = write_cursor_byte;
                        }
                        
                        void* region1;
                        void* region2;
                        DWORD region1_size;
                        DWORD region2_size;
                        
                        ASSERT_FUNCTION(SUCCEEDED(COM_FUNCTION(dsound_buffer, Lock, byte_to_lock, bytes_to_write, &region1, &region1_size, &region2, &region2_size, 0)));
                        
                        //
                        
                        ASSERT_FUNCTION(SUCCEEDED(COM_FUNCTION(dsound_buffer, Unlock, region1, region1_size, region2, region2_size)));
                        
                        
                        
                        
                        
                        
                        s64 frame_work_tick = win32_get_cpu_tick();
                        f32 frame_work_seconds = win32_get_seconds_elapsed(frame_begin_tick, frame_work_tick);
                        int ms_to_sleep = (int)((target_frame_seconds - frame_work_seconds) * 1000.0f);
                        if(ms_to_sleep > 1)
                        {
                            Sleep((DWORD)(ms_to_sleep - 1));
                        }
                        
                        ASSERT_FUNCTION(BitBlt(g_win32.window_dc,
                                               g_win32.blit_dest_x,
                                               g_win32.blit_dest_y,
                                               g_back_buffer.width,
                                               g_back_buffer.height,
                                               g_win32.bitmap_dc,
                                               0,
                                               0,
                                               SRCCOPY));
                        
                        s64 frame_end_tick = win32_get_cpu_tick();
                        last_frame_time_seconds = win32_get_seconds_elapsed(frame_begin_tick, frame_end_tick);
                        // NOTE(leo): Checking if the last frame seconds is 1.5 times larger than the target, in cases where the user resizes or move the window, or when we
                        // are debugging, and set it to the target, so that our update code doesn't gets crazy.
                        if(last_frame_time_seconds > target_frame_seconds * 1.5f) 
                        {
                            last_frame_time_seconds = target_frame_seconds;
                        }
                    }
                    
                    break_main_loop:
                    ASSERT_FUNCTION(DestroyWindow(g_win32.window_handle));
                }
                else
                {
                    // TODO(leo): timeBeginPeriod failed.
                }
            }
        }
    }
    else
    {
        // TODO(leo): QueryPerformanceFrequency failed.
    }
    
    ExitProcess(exit_code);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
