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

enum JsonValueType
{
    JsonValue_Null,         // data = NULL
    JsonValue_String,       // data = Str*
    JsonValue_Number,       // data = f64
    JsonValue_Bool,         // data = bool
    JsonValue_Object,       // data = JsonObject*
    JsonValue_Array,        // data = List<JsonValue>*
};

struct JsonValue;
struct JsonObject;
typedef DArray<JsonValue> JsonArray;

struct JsonValue
{
    JsonValueType type = JsonValue_Null;
    void* data = NULL;

    String AsString();
    f64 AsNumber();
    bool AsBool();
    JsonObject* AsObject();
    JsonArray* AsArray();
};

struct JsonObject
{
    // Implementing using two lists, key/value pairs are inserted in order.
    // Technically, these are unordered, but this makes it easier to implement.
    // If this implies in any performance concern later, change to hash map.
    DArray<String> keys;
    DArray<JsonValue> values;
    
    JsonValue* GetValue(const String& key);

    String      GetStringValue(const String& key);
    f64         GetNumberValue(const String& key);
    bool        GetBoolValue(const String& key);
    JsonObject* GetObjectValue(const String& key);
    JsonArray*  GetArrayValue(const String& key);

    bool GetStringValue(const String& key, String* out);
    bool GetObjectValue(const String& key, JsonObject* out);
    bool GetArrayValue(const String& key, JsonArray* out);
    bool GetNumberValue(const String& key, f32* out);
    bool GetNumberValue(const String& key, f64* out);
    bool GetNumberValue(const String& key, u32* out);
    bool GetNumberValue(const String& key, u64* out);
    bool GetNumberValue(const String& key, i32* out);
    bool GetNumberValue(const String& key, i64* out);
};

byte* JsonSkipWhitespace(byte* p);
byte* JsonParseNumber(byte* p, f64* out);
byte* JsonParseString(mem::Arena* arena, byte* p, String* out);
byte* JsonParseValue(mem::Arena* arena, byte* p, JsonValue* out);
byte* JsonParseArray(mem::Arena* arena, byte* p, JsonArray* out);
byte* JsonParseObject(mem::Arena* arena, byte* p, JsonObject* out);

JsonObject* MakeJson(mem::Arena* arena, byte* jsonBuffer);
JsonObject* MakeJsonFromStr(mem::Arena* arena, String jsonStr);
JsonObject* MakeJsonFromFile(mem::Arena* arena, String jsonPath);

}
}
