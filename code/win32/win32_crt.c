#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif

int _fltused = 0x9875; // NOTE(leo): Value gathered from CRT source file 'fltused.cpp' at MSVC installation path.

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#if defined(MSVC) // NOTE(leo): MSVC must be defined as compiler option (-D) or before this file.
#pragma function(memset)
#endif
extern void *
memset(void *_Dst, int _Val, size_t _Size)
{
    unsigned char  value_to_set = *(unsigned char *)&_Val;
    unsigned char *byte_to_set  =  (unsigned char *)_Dst;
    
    while(_Size--)
    {
        *byte_to_set++ = value_to_set;
    }
    
    return byte_to_set;
}

#if defined(MSVC)
#pragma function(memcpy)
#endif
extern void *
memcpy(void *_Dst, const void *_Src, size_t _Size)
{
    char *dest = (char *)_Dst;
    const char *src = (const char *)_Src;
    
    while(_Size--)
    {
        *dest++ = *src++;
    }
    
    return dest;
}
