#include "../asset/json.hpp"

namespace ty
{
namespace asset
{

JsonValue* JsonObject::GetValue(const String& key)
{
    for(i32 i = 0; i < keys.count; i++)
    {
        if(key == keys[i])
        {
            return &values[i];
        }
    }
    return NULL;
}

String& JsonObject::GetString(const String& key)
{
    JsonValue* value = GetValue(key);
    ASSERT(value && value->type == JsonValue_String);
    return *(String*)(value->data);
}

f64 JsonObject::GetNumber(const String& key)
{
    JsonValue* value = GetValue(key);
    ASSERT(value && value->type == JsonValue_Number);
    f64 result = *(f64*)&(value->data);
    return result;
}

bool JsonObject::GetBool(const String& key)
{
    JsonValue* value = GetValue(key);
    ASSERT(value && value->type == JsonValue_Bool);
    return (bool)(value->data);
}

JsonObject* JsonObject::GetObject(const String& key)
{
    JsonValue* value = GetValue(key);
    ASSERT(value && value->type == JsonValue_Object);
    return (JsonObject*)(value->data);
}

List<JsonValue>* JsonObject::GetArray(const String& key)
{
    JsonValue* value = GetValue(key);
    ASSERT(value && value->type == JsonValue_Array);
    return (List<JsonValue>*)(value->data);
}

u8* JsonSkipWhitespace(u8* p)
{
    while(  *p == 0x20       // Space 
        ||  *p == 0x0a       // Line feed 
        ||  *p == 0x0d       // Carriage return
        ||  *p == 0x09)      // Horizontal tab
    {
        p++;
    }

    return p;
}

u8* JsonParseNumber(u8* p, f64* out)
{
    i32 sign = 1;
    if(*p == '-')
    {
        sign = -1;
        p++;
    }
    
    f64 result = 0;
    while(*p >= '0' && *p <= '9')
    {
        i32 digit = *p - '0';
        result = result * 10 + digit;
        p++;
    }

    // Fractional
    if(*p == '.')
    {
        p++;
        f64 fraction = 0;
        i32 decimal = 1;
        while(*p >= '0' && *p <= '9')
        {
            i32 digit = *p - '0';
            fraction = fraction * 10 + digit;
            decimal *= 10;
            p++;
        }
        result += (fraction / decimal);
    }

    // Exponent
    if(*p == 'e' || *p == 'E')
    {
        p++;
        int expoSign = 1;
        f64 exponent = 0;
        if(*p == '+') p++;
        else if(*p == '-')
        {
            expoSign = -1;
            p++;
        }
        while(*p >= '0' && *p <= '9')
        {
            i32 digit = *p - '0';
            exponent = exponent * 10 + digit;
            p++;
        }
        exponent *= expoSign;
        result *= powf(10, exponent);
    }

    result = sign * result;

    *out = result;
    return p;
}

u8* JsonParseString(u8* p, String* out)
{
    ASSERT(*p == '"');
    p++;

    // First find out capacity of string
    u64 size = 0;
    while(true)
    {
        if(*(p + size) == '"' && *(p + size - 1) != '\\') break;

        size++;
    }

    // Then construct the string with the right size
    if(!size)
    {
        p++;
        *out = IStr("");
        return p;
    }
    MStr(result, size + 1);
    while(true)
    {
        if(*p == '"') break;

        char nextChar[2];
        nextChar[0] = *p;
        nextChar[1] = 0;
        if(*p == '\\')
        {
            char special = *(p + 1);
            if(special == '"')
            {
                nextChar[0] = 0x22;     // Quotation mark
            }
            else if(special == '\\')
            {
                nextChar[0] = 0x5C;     // Reverse solidus
            }
            else if(special == '/')
            {
                nextChar[0] = 0x2F;     // Solidus
            }
            else if(special == 'b')
            {
                nextChar[0] = 0x08;     // Backspace
            }
            else if(special == 'f')
            {
                nextChar[0] = 0x0C;     // Form feed
            }
            else if(special == 'n')
            {
                nextChar[0] = 0x0A;     // Line feed
            }
            else if(special == 'r')
            {
                nextChar[0] = 0x0D;     // Carriage return
            }
            else if(special == 't')
            {
                nextChar[0] = 0x09;     // Horizontal tab
            }
            else if(special == 'u')
            {
                ASSERT(0);              // Not supported yet
            }

            p += 2;
        }
        else
        {
            p++;
        }

        str::Append(result, nextChar);
    }

    *out = result;
    p++;
    return p;
}

u8* JsonParseValue(u8* p, JsonValue* out)
{
    JsonValue result = {};

    p = JsonSkipWhitespace(p);

    if(*p == '"')
    {
        // String
        result.type = JsonValue_String;
        String dataStr;
        p = JsonParseString(p, &dataStr);
        result.data = &dataStr;
    }
    else if(*p == 't')
    {
        // Bool (true)
        result.type = JsonValue_Bool;
        result.data = (void*)1;
        p += 4; // true
    }
    else if(*p == 'f')
    {
        // Bool (false)
        result.type = JsonValue_Bool;
        result.data = (void*)0;
        p += 5; // false
    }
    else if(*p == 'n')
    {
        // Null
        result.type = JsonValue_Null;
        result.data = NULL;
        p += 4; // null
    }
    else if(*p == '{')
    {
        // Object
        result.type = JsonValue_Object;
        JsonObject resultObject;
        p = JsonParseObject(p, &resultObject);
        JsonObject* pResult = (JsonObject*)mem::Alloc(sizeof(JsonObject));
        memcpy(pResult, &resultObject, sizeof(JsonObject));
        result.data = pResult;

    }
    else if(*p == '[')
    {
        // Array
        result.type = JsonValue_Array;
        List<JsonValue> resultArray;
        p = JsonParseArray(p, &resultArray);
        List<JsonValue>* pResult = (List<JsonValue>*)mem::Alloc(sizeof(List<JsonValue>));
        memcpy(pResult, &resultArray, sizeof(List<JsonValue>));
        result.data = pResult;
    }
    else
    {
        // Number
        result.type = JsonValue_Number;
        f64 dataNumber;
        p = JsonParseNumber(p, &dataNumber);
        result.data = *(void**)&dataNumber;
    }

    p = JsonSkipWhitespace(p);

    *out = result;
    return p;
}

u8* JsonParseArray(u8* p, List<JsonValue>* out)
{
    ASSERT(*p == '[');
    p++;

    List<JsonValue> result = MakeList<JsonValue>();
    p = JsonSkipWhitespace(p);
    if(*p == ']')
    {
        *out = result;
        p++;
        return p;
    }

    while(true)
    {
        JsonValue element;
        p = JsonParseValue(p, &element);
        result.Push(element);
        if(*p == ']') break;
        p++;    // Comma separating elements
    }

    *out = result;
    p++;
    return p;
}

u8* JsonParseObject(u8* p, JsonObject* out)
{
    ASSERT(*p == '{');
    p++;

    JsonObject result = {};
    result.keys = MakeList<String>();
    result.values = MakeList<JsonValue>();

    p = JsonSkipWhitespace(p);
    if(*p == '}')
    {
        *out = result;
        p++;
        return p;
    }

    while(true)
    {
        String key;
        p = JsonParseString(p, &key);
        result.keys.Push(key);
        p = JsonSkipWhitespace(p);
        p++;    // Semicolon pairing key with value
        JsonValue value;
        p = JsonParseValue(p, &value);
        result.values.Push(value);
        if(*p == '}') break;
        p++;    // Comma separating object key/value pairs
        p = JsonSkipWhitespace(p);
    }

    *out = result;
    p++;
    return p;
}

}
}
