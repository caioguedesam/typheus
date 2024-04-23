#include "../asset/json.hpp"
#include "src/core/ds.hpp"
#include "src/core/memory.hpp"
#include "src/core/string.hpp"

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

String JsonValue::AsString()
{
    ASSERT(type == JsonValue_String);
    String result = *(String*)data;
    return result;
}

f64 JsonValue::AsNumber()
{
    ASSERT(type == JsonValue_Number);
    f64 result = *(f64*)&(data);
    return result;
}

bool JsonValue::AsBool()
{
    ASSERT(type == JsonValue_Bool);
    return (bool)(data);
}

JsonObject* JsonValue::AsObject()
{
    ASSERT(type == JsonValue_Object);
    return (JsonObject*)(data);
}

JsonArray* JsonValue::AsArray()
{
    ASSERT(type == JsonValue_Array);
    return (JsonArray*)(data);
}

String JsonObject::GetStringValue(const String& key)
{
    JsonValue* value = GetValue(key);
    ASSERT(value);
    return value->AsString();
}

f64 JsonObject::GetNumberValue(const String& key)
{
    JsonValue* value = GetValue(key);
    ASSERT(value);
    return value->AsNumber();
}

bool JsonObject::GetBoolValue(const String& key)
{
    JsonValue* value = GetValue(key);
    ASSERT(value);
    return value->AsBool();
}

JsonObject* JsonObject::GetObjectValue(const String& key)
{
    JsonValue* value = GetValue(key);
    ASSERT(value);
    return value->AsObject();
}

JsonArray* JsonObject::GetArrayValue(const String& key)
{
    JsonValue* value = GetValue(key);
    ASSERT(value);
    return value->AsArray();
}

bool JsonObject::GetStringValue(const String& key, String* out)
{
    ASSERT(out);
    JsonValue* value = GetValue(key);
    if(!value) return false;

    *out = value->AsString();

    return true;
}

bool JsonObject::GetObjectValue(const String& key, JsonObject* out)
{
    ASSERT(out);
    JsonValue* value = GetValue(key);
    if(!value) return false;

    *out = *value->AsObject();

    return true;
}

bool JsonObject::GetArrayValue(const String& key, JsonArray* out)
{
    ASSERT(out);
    JsonValue* value = GetValue(key);
    if(!value) return false;

    *out = *value->AsArray();
    return true;
}

#define TY_JSON_SET_NUMBER_VALUE_BODY \
    ASSERT(out); \
    JsonValue* value = GetValue(key); \
    if(!value) return false; \
    *out = value->AsNumber(); \
    return true;

bool JsonObject::GetNumberValue(const String& key, f32* out)
{
    TY_JSON_SET_NUMBER_VALUE_BODY
}

bool JsonObject::GetNumberValue(const String& key, f64* out)
{
    TY_JSON_SET_NUMBER_VALUE_BODY
}

bool JsonObject::GetNumberValue(const String& key, u32* out)
{
    TY_JSON_SET_NUMBER_VALUE_BODY
}

bool JsonObject::GetNumberValue(const String& key, u64* out)
{
    TY_JSON_SET_NUMBER_VALUE_BODY
}

bool JsonObject::GetNumberValue(const String& key, i32* out)
{
    TY_JSON_SET_NUMBER_VALUE_BODY
}

bool JsonObject::GetNumberValue(const String& key, i64* out)
{
    TY_JSON_SET_NUMBER_VALUE_BODY
}

byte* JsonSkipWhitespace(byte* p)
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

byte* JsonParseNumber(byte* p, f64* out)
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

byte* JsonParseString(mem::Arena* arena, byte* p, String* out)
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
        *out = "";
        return p;
    }

    //MStr(result, size + 1);
    char* buf = (char*)mem::ArenaPushZero(arena, size + 1);
    u64 index = 0;
    while(true)
    {
        if(*p == '"') break;

        // char nextChar[2];
        // nextChar[0] = *p;
        // nextChar[1] = 0;
        char nextChar = *p;
        if(*p == '\\')
        {
            char special = *(p + 1);
            if(special == '"')
            {
                nextChar = 0x22;     // Quotation mark
            }
            else if(special == '\\')
            {
                nextChar = 0x5C;     // Reverse solidus
            }
            else if(special == '/')
            {
                nextChar = 0x2F;     // Solidus
            }
            else if(special == 'b')
            {
                nextChar = 0x08;     // Backspace
            }
            else if(special == 'f')
            {
                nextChar = 0x0C;     // Form feed
            }
            else if(special == 'n')
            {
                nextChar = 0x0A;     // Line feed
            }
            else if(special == 'r')
            {
                nextChar = 0x0D;     // Carriage return
            }
            else if(special == 't')
            {
                nextChar = 0x09;     // Horizontal tab
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

        //str::Append(result, nextChar);
        buf[index] = nextChar;
        index++;
    }

    *out = Str(buf);
    p++;
    return p;
}

byte* JsonParseValue(mem::Arena* arena, byte* p, JsonValue* out)
{
    JsonValue result = {};

    p = JsonSkipWhitespace(p);

    if(*p == '"')
    {
        // String
        result.type = JsonValue_String;
        String parsedStr;
        p = JsonParseString(arena, p, &parsedStr);
        String* dataStr = (String*)mem::ArenaPush(arena, sizeof(String));
        *dataStr = parsedStr;
        result.data = dataStr;
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
        p = JsonParseObject(arena, p, &resultObject);
        JsonObject* pResult = (JsonObject*)mem::ArenaPush(arena, sizeof(JsonObject));
        memcpy(pResult, &resultObject, sizeof(JsonObject));
        result.data = pResult;

    }
    else if(*p == '[')
    {
        // Array
        result.type = JsonValue_Array;
        JsonArray resultArray;
        p = JsonParseArray(arena, p, &resultArray);
        JsonArray* pResult = (JsonArray*)mem::ArenaPush(arena, sizeof(JsonArray));
        memcpy(pResult, &resultArray, sizeof(JsonArray));
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

byte* JsonParseArray(mem::Arena* arena, byte* p, JsonArray* out)
{
    ASSERT(*p == '[');
    p++;

    JsonArray result = MakeDArray<JsonValue>(arena);
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
        p = JsonParseValue(arena, p, &element);
        result.Push(element);
        if(*p == ']') break;
        p++;    // Comma separating elements
    }

    *out = result;
    p++;
    return p;
}

byte* JsonParseObject(mem::Arena* arena, byte* p, JsonObject* out)
{
    ASSERT(*p == '{');
    p++;

    JsonObject result = {};
    result.keys = MakeDArray<String>(arena);
    result.values = MakeDArray<JsonValue>(arena);

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
        p = JsonParseString(arena, p, &key);
        result.keys.Push(key);
        p = JsonSkipWhitespace(p);
        p++;    // Semicolon pairing key with value
        JsonValue value;
        p = JsonParseValue(arena, p, &value);
        result.values.Push(value);
        if(*p == '}') break;
        p++;    // Comma separating object key/value pairs
        p = JsonSkipWhitespace(p);
    }

    *out = result;
    p++;
    return p;
}

JsonObject* MakeJson(mem::Arena* arena, byte* jsonBuffer)
{
    JsonObject* result = (JsonObject*)mem::ArenaPush(arena, sizeof(JsonObject));
    byte* jsonCursor = jsonBuffer;
    JsonParseObject(arena, jsonCursor, result);
    return result;
}

JsonObject* MakeJsonFromStr(mem::Arena* arena, String jsonStr)
{
    JsonObject* result = (JsonObject*)mem::ArenaPush(arena, sizeof(JsonObject));
    byte* jsonCursor = (byte*)jsonStr.CStr();
    JsonParseObject(arena, jsonCursor, result);
    return result;
}

JsonObject* MakeJsonFromFile(mem::Arena* arena, String jsonPath)
{
    byte* jsonBuffer = file::ReadFileToBuffer(arena, jsonPath);
    return MakeJson(arena, jsonBuffer);
}

}   // namespace asset
}   // namespace ty
