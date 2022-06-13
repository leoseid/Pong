INTERNAL void
os_print(String8 to_print)
{
    ASSERT(to_print.data && to_print.length && to_print.length <= U32_MAX);
    
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if(stdout_handle)
    {
        DWORD written;
        ASSERT_FUNCTION(WriteConsoleA(stdout_handle, to_print.data, (u32)to_print.length, &written, NULL));
        ASSERT(written == (u32)to_print.length);
    }
    
    OutputDebugStringA(to_print.data);
}
