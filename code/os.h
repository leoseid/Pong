INTERNAL void os_print(String8 to_print);

INTERNAL void
os_printf(String8 format, ...)
{
    va_list args;
    va_start(args, format);
    
    char formated[1024];
    u32 formated_len = str_format_va(formated, ARRAY_COUNT(formated), format, args);
    va_end(args);
    
    // NOTE(leo): In case that 'formated' buffer is not large enough for the result string, when DEVELOPMENT is defined, an INVALID_CODE_PATH will be triggered, making
    // us know that we need to increase its size. When DEVELOPMENT is not defined, the str_format_va will return the number of characters written to the point before it
    // realized that is not enough space. Then we will print this amount of characters.
    
    os_print((String8){formated, formated_len});
}
