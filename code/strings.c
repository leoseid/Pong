typedef struct
{
    char *data;
    u64   length;
    
} String8;

#define STRING8_LITERAL(literal) ((String8){literal, ARRAY_COUNT(literal)-1})

INTERNAL u32
str_format_va(char *result, u64 result_capacity, String8 format, va_list args)
{
    ASSERT(result && result_capacity && format.data && args);
    
    u32 chars_written = 0;
    
    char *character = format.data;
    
#define CHECK_BOUNDS(to_write_count) if(chars_written + to_write_count >= result_capacity) { INVALID_CODE_PATH; goto end; }
    
    for(;;)
    {
        if(character >= format.data + format.length)
        {
            break;
        }
        
        if(*character == '%')
        {
            character++;
            
            b32 commas_in_thousands = false;
            char *digits_string = "0123456789";
            u32 int_base = 10;
            b32 float_precision_specified = false;
            
            // MUST be zero here, otherwise the while loop bellow will calculate it wrong.
            int float_precision = 0;
            
            if(*character == '\'')
            {
                character++;
                commas_in_thousands = true;
            }
            else if(*character == 'x' || *character == 'X')
            {
                digits_string = *character == 'x' ? "0123456789abcdef" : "0123456789ABCDEF";
                int_base = 16;
                character++;
            }
            else if(*character == '.')
            {
                character++;
                float_precision_specified = true;
                
                ASSERT(*character >= '0' && *character <= '9');
                while(*character >= '0' && *character <= '9')
                {
                    float_precision *= 10;
                    float_precision += *character++ - '0';
                }
            }
            
            if(!float_precision_specified)
            {
                float_precision = 6;
            }
            
            switch(*character)
            {
                case 'S':
                {
                    String8 string = *va_arg(args, String8 *);
                    CHECK_BOUNDS(string.length);
                    
                    for(u64 i = 0;
                        i < string.length;
                        ++i)
                    {
                        result[chars_written++] = string.data[i];
                    }
                } break;
                
                case 'a':
                {
                    char *null_term_string = va_arg(args, char *);
                    
                    for(;
                        *null_term_string;
                        ++null_term_string)
                    {
                        CHECK_BOUNDS(1);
                        result[chars_written++] = *null_term_string;
                    }
                } break;
                
                case 'w':
                {
                    wchar_t *null_term_wide_string = va_arg(args, wchar_t *);
                    
                    for(;
                        *null_term_wide_string;
                        ++null_term_wide_string)
                    {
                        ASSERT(*((char *)null_term_wide_string + 1) == 0);
                        CHECK_BOUNDS(1);
                        result[chars_written++] = *((char *)null_term_wide_string);
                    }
                } break;
                
                case 's':
                case 'u':
                {
                    u64 value;
                    
                    if(*character++ == 's')
                    {
                        s64 s_value;
                        
                        switch(*character)
                        {
                            case '8':
                            {
#ifdef __clang__
                                s_value = va_arg(args, s32);
#else
                                s_value = va_arg(args, s8);
#endif
                            } break;
                            
                            case '1':
                            {
                                character++;
#ifdef __clang__
                                s_value = va_arg(args, s32);
#else
                                s_value = va_arg(args, s16);
#endif
                            } break;
                            
                            case '3':
                            {
                                character++;
                                s_value = va_arg(args, s32);
                            } break;
                            
                            case '6':
                            {
                                character++;
                                s_value = va_arg(args, s64);
                            } break;
                            
                            default:
                            {
                                s_value = 0; // NOTE(leo): Just to shut up MSVC.
                                INVALID_CODE_PATH;
                            } break;
                        }
                        
                        if(s_value < 0)
                        {
                            CHECK_BOUNDS(1);
                            result[chars_written++] = '-';
                            s_value = -s_value;
                        }
                        
                        value = (u64)s_value;
                    }
                    else
                    {
                        switch(*character)
                        {
                            case '8':
                            {
#ifdef __clang__
                                value = va_arg(args, u32);
#else
                                value = va_arg(args, u8);
#endif
                            } break;
                            
                            case '1':
                            {
                                character++;
#ifdef __clang__
                                value = va_arg(args, u32);
#else
                                value = va_arg(args, u16);
#endif
                            } break;
                            
                            case '3':
                            {
                                character++;
                                value = va_arg(args, u32);
                            } break;
                            
                            case '6':
                            {
                                character++;
                                value = va_arg(args, u64);
                            } break;
                            
                            default:
                            {
                                value = 0; // NOTE(leo): Just to shut up MSVC.
                                INVALID_CODE_PATH;
                            } break;
                        }
                    }
                    
                    if(int_base == 16)
                    {
                        CHECK_BOUNDS(2);
                        result[chars_written++] = '0';
                        result[chars_written++] = 'x';
                    }
                    
                    if(value == 0)
                    {
                        CHECK_BOUNDS(1);
                        result[chars_written++] = '0';
                    }
                    else
                    {
                        u32 digits_count = 0;
                        u64 temp_value = value;
                        while(temp_value)
                        {
                            digits_count++;
                            temp_value /= int_base;
                        }
                        
                        u32 chars_to_write = digits_count;
                        
                        if(commas_in_thousands)
                        {
                            ASSERT(int_base == 10);
                            
                            chars_to_write += digits_count / 3;
                            if(digits_count % 3 == 0)
                            {
                                chars_to_write--;
                            }
                        }
                        
                        CHECK_BOUNDS(chars_to_write);
                        
                        u32 chars_to_write_temp = chars_to_write;
                        
                        u32 count = 0;
                        while(value)
                        {
                            u32 digit_index = value % int_base;
                            result[chars_written + --chars_to_write] = digits_string[digit_index];
                            if(++count == 3 && commas_in_thousands)
                            {
                                count = 0;
                                result[chars_written + --chars_to_write] = ',';
                            }
                            value /= int_base;
                        }
                        
                        chars_written += chars_to_write_temp;
                    }
                } break;
                
                case 'f':
                {
                    f64 value = va_arg(args, f64);
                    
                    int temp_written = d2fixed_buffered_n(value, (u32)float_precision, result + chars_written);
                    ASSERT(temp_written > 0);
                    
                    // Same as CHECK_BOUNDS
                    if(chars_written + (u32)temp_written >= result_capacity)
                    {
                        INVALID_CODE_PATH;
                        // NOTE(leo): Here we can add temp_written to chars_written even though we pass the bounds, because since the code got here, it didn't crash,
                        // and so why not print the complete float string.
                        chars_written += (u32)temp_written;
                        goto end;
                    }
                    
                    chars_written += (u32)temp_written;
                    
                } break;
                
                case '%':
                {
                    CHECK_BOUNDS(1);
                    result[chars_written++] = '%';
                } break;
                
                case 'b':
                {
                    character++;
                    
                    b32 bool_value;
                    
                    switch(*character)
                    {
                        case '8':
                        {
#ifdef __clang__
                            bool_value = va_arg(args, b32);
#else
                            bool_value = va_arg(args, b8);
#endif
                        } break;
                        
                        case '3':
                        {
                            bool_value = va_arg(args, b32);
                            character++;
                        } break;
                        
                        default:
                        {
                            bool_value = 0; // NOTE(leo): Just to shut up the compiler.
                            INVALID_CODE_PATH;
                        } break;
                    }
                    
                    String8 bool_str;
                    
                    if(bool_value)
                    {
                        bool_str = STRING8_LITERAL("true");
                    }
                    else
                    {
                        bool_str = STRING8_LITERAL("false");
                    }
                    
                    CHECK_BOUNDS(bool_str.length);
                    for(u64 i = 0;
                        i < bool_str.length;
                        ++i)
                    {
                        result[chars_written++] = bool_str.data[i];
                    }
                    
                } break;
                
                default:
                {
                    INVALID_CODE_PATH;
                } break;
            }
        }
        else
        {
            CHECK_BOUNDS(1);
            result[chars_written++] = *character;
        }
        
        character++;
    }
    
#undef CHECK_BOUNDS
    
    end:
    result[chars_written] = '\0';
    return chars_written;
}
