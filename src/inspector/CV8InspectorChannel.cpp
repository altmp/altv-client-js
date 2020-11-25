
#include "v8-inspector.h";
#include "../helpers/V8Helpers.h";
#include "CV8InspectorChannel.h";

void CV8InspectorChannel::Send(const v8_inspector::StringView& string)
{
    auto isolate = _isolate;
    v8::Isolate::AllowJavascriptExecutionScope allow_script(isolate);
    v8::HandleScope handle_scope(isolate);

    int length = static_cast<int>(string.length());
    V8_CHECK(length < v8::String::kMaxLength, "Message too long");

    v8::Local<v8::String> message =
        (string.is8Bit()
            ? v8::String::NewFromOneByte(
                isolate,
                reinterpret_cast<const uint8_t*>(string.characters8()),
                v8::NewStringType::kNormal, length)
            : v8::String::NewFromTwoByte(
                isolate,
                reinterpret_cast<const uint16_t*>(string.characters16()),
                v8::NewStringType::kNormal, length))
        .ToLocalChecked();

    v8::Local<v8::Context> context = _context.Get(isolate);
    v8::Local<v8::Function> callback = _client->GetCallback(isolate);
    auto type = callback->TypeOf(isolate);
    v8::String::Utf8Value utfValue(isolate, type);
    std::string name(*utfValue);

    if (callback->IsFunction()) {
        v8::TryCatch try_catch(isolate);
        v8::Local<v8::Value> args[] = { message };
        v8::Local<v8::Function>::Cast(callback)->Call(context, v8::Undefined(isolate), 1, args);
    }
}