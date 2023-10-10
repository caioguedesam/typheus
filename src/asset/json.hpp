// ========================================================
// JSON
// Functionality for parsing JSON files.
// @Caio Guedes, 2023
// ========================================================
#pragma once
#include "../core/base.hpp"
#include "../core/file.hpp"
#include "../core/math.hpp"
#include "../core/ds.hpp"
#include "../core/string.hpp"

namespace ty
{
namespace asset
{

enum JsonValueType : u8
{
    JsonValue_Null,         // data = NULL
    JsonValue_String,       // data = Str*
    JsonValue_Number,       // data = f64
    JsonValue_Bool,         // data = bool
    JsonValue_Object,       // data = JsonObject*
    JsonValue_Array,        // data = List<JsonValue>*
};

struct JsonValue
{
    JsonValueType type = JsonValue_Null;
    void* data = NULL;
};

struct JsonObject
{
    // Implementing using two lists, key/value pairs are inserted in order.
    // Technically, these are unordered, but this makes it easier to implement.
    // If this implies in any performance concern later, change to hash map.
    List<String> keys;
    List<JsonValue> values;
    
    JsonValue* GetValue(const String& key);

    String& GetString(const String& key);
    f64 GetNumber(const String& key);
    bool GetBool(const String& key);
    JsonObject* GetObject(const String& key);
    List<JsonValue>* GetArray(const String& key);
};

//void JsonSkipWhitespace(u8* p);
//f64 JsonParseNumber(u8* p);
//String JsonParseString(u8* p);
//JsonValue JsonParseValue(u8* p);
//List<JsonValue> JsonParseArray(u8* p);
//JsonObject JsonParseObject(u8* p);

u8* JsonSkipWhitespace(u8* p);
u8* JsonParseNumber(u8* p, f64* out);
u8* JsonParseString(u8* p, String* out);
u8* JsonParseValue(u8* p, JsonValue* out);
u8* JsonParseArray(u8* p, List<JsonValue>* out);
u8* JsonParseObject(u8* p, JsonObject* out);

// TODO(caio): CONTINUE
//  - Test json parse*
//  - Implement freeing json memory
//  - Test freeing json memory

}
}
