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
typedef List<JsonValue> JsonArray;

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
    List<String> keys;
    List<JsonValue> values;
    
    JsonValue* GetValue(const String& key);

    String      GetStringValue(const String& key);
    f64         GetNumberValue(const String& key);
    bool        GetBoolValue(const String& key);
    JsonObject* GetObjectValue(const String& key);
    JsonArray*  GetArrayValue(const String& key);

    bool GetStringValue(const String& key, String* out);
    bool GetObjectValue(const String& key, JsonObject* out);
    bool GetNumberValue(const String& key, f32* out);
    bool GetNumberValue(const String& key, f64* out);
    bool GetNumberValue(const String& key, u32* out);
    bool GetNumberValue(const String& key, u64* out);
    bool GetNumberValue(const String& key, i32* out);
    bool GetNumberValue(const String& key, i64* out);
};

u8* JsonSkipWhitespace(u8* p);
u8* JsonParseNumber(u8* p, f64* out);
u8* JsonParseString(u8* p, String* out);
u8* JsonParseValue(u8* p, JsonValue* out);
u8* JsonParseArray(u8* p, JsonArray* out);
u8* JsonParseObject(u8* p, JsonObject* out);

void JsonFreeValue(JsonValue* value);
void JsonFreeArray(JsonArray* jsonArray);
void JsonFreeObject(JsonObject* jsonObject);

JsonObject* MakeJson(String jsonStr);
JsonObject* MakeJson(file::Path jsonPath);
void DestroyJson(JsonObject* jsonObject);

}
}
