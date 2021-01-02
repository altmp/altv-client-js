
#include "v8-inspector.h"
#include "../helpers/V8Helpers.h"
#include "CV8InspectorChannel.h"
#include "CV8InspectorClient.h"

void CV8InspectorChannel::Send(int id, const v8_inspector::StringView& string)
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

    auto it = CV8InspectorClient::promises.find(static_cast<uint32_t>(id));
    if(it != CV8InspectorClient::promises.end())
    {
        auto promise = it->second.Get(isolate);
        v8::TryCatch try_catch(isolate);
        v8::Local<v8::Context> context = isolate->GetEnteredContext();

        v8::Local<v8::Object> result = v8::JSON::Parse(context, message).ToLocalChecked().As<v8::Object>();
        v8::MaybeLocal<v8::Value> error = result->Get(context, v8::String::NewFromUtf8(isolate, "error").ToLocalChecked());
        v8::Local<v8::Value> errorObj;
        if (error.ToLocal(&errorObj) && errorObj.As<v8::Object>()->HasOwnProperty(context, v8::String::NewFromUtf8(isolate, "code").ToLocalChecked()).ToChecked())
            promise->Reject(context, errorObj);
        else
            promise->Resolve(context, result);
        CV8InspectorClient::promises.erase(static_cast<uint32_t>(id));
    }

    auto msg = CV8InspectorClient::pendingMessages.find(static_cast<uint32_t>(id));
    if(msg != CV8InspectorClient::pendingMessages.end())
    {
        v8::String::Utf8Value utf8(isolate, message);
        std::string str(*utf8);
        auto socket = msg->second;
        socket->send(str);
        CV8InspectorClient::pendingMessages.erase(static_cast<uint32_t>(id));
    }
}

void CV8InspectorChannel::SendEvent(const v8_inspector::StringView& string)
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

    v8::Local<v8::Context> context = isolate->GetEnteredContext();

    CV8ResourceImpl* resource = static_cast<CV8ResourceImpl*>(V8ResourceImpl::Get(context));
    auto callback = resource->GetInspector()->GetCallback(isolate);

    if(!callback.IsEmpty())
    {
        v8::TryCatch try_catch(isolate);

        v8::Local<v8::Object> result = v8::JSON::Parse(context, message).ToLocalChecked().As<v8::Object>();
        v8::Local<v8::Value> args[] = { message };
        callback->Call(context, v8::Undefined(isolate), 1, args);
    }

    v8::String::Utf8Value utf8(isolate, message);
    std::string str(*utf8);

    auto server = _client->GetWebSocketServer();
    if(server == nullptr) return;
    auto clients = server->getClients();
    for(auto& client : clients)
    {
        client->send(str);
    }
}