
#include "CV8InspectorClient.h"
#include "CV8InspectorChannel.h"

CV8InspectorClient::CV8InspectorClient(v8::Local<v8::Context> context, bool connect)
{
    if (!connect) return;

    _isolate = context->GetIsolate();
    auto isolate = _isolate;

    _channel.reset(new CV8InspectorChannel(context, this));
    _inspector = v8_inspector::V8Inspector::create(isolate, this);
    _session = _inspector->connect(1, _channel.get(), v8_inspector::StringView());
    _inspector->contextCreated(v8_inspector::V8ContextInfo(context, 1, v8_inspector::StringView()));

    _context.Reset(isolate, context);
}

v8::Local<v8::Promise> CV8InspectorClient::SendInspectorMessage(v8::Isolate* isolate, alt::String method, v8::Local<v8::Object> params)
{
    v8::HandleScope handle_scope(isolate);

    std::unique_ptr<uint16_t[]> method_buffer(new uint16_t[method.GetSize()]);
    v8::Local<v8::String> methodString = v8::String::NewFromUtf8(isolate, method.CStr()).ToLocalChecked();
    methodString->Write(isolate, method_buffer.get(), 0, method.GetSize());
    v8_inspector::StringView method_view(method_buffer.get(), method.GetSize());
    if (!v8_inspector::V8InspectorSession::canDispatchMethod(method_view)) {
        V8Helpers::Throw(isolate, "Invalid protocol method passed");
        return;
    }

    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    auto id = CV8InspectorClient::GetNextMessageId();

    auto promise = v8::Promise::Resolver::New(ctx);
    v8::Local<v8::Promise::Resolver> resolver;
    if (!promise.ToLocal(&resolver))
        return v8::Local<v8::Promise>();

    promises.emplace(id, v8::Global<v8::Promise::Resolver>(isolate, resolver));

    auto paramsStringified = v8::JSON::Stringify(ctx, params).ToLocalChecked();
    v8::String::Utf8Value paramsUtf(isolate, paramsStringified);
    std::string paramsString(*paramsUtf);

    // This is a bad solution, too bad!
    char paramsChar[1024];
    sprintf_s(paramsChar, "{ \"id\": %d, \"method\": \"%s\", \"params\": %s }", id, method.CStr(), paramsString.c_str());
    v8::Local<v8::String> message = v8::String::NewFromUtf8(isolate, paramsChar).ToLocalChecked();

    std::unique_ptr<uint16_t[]> buffer(new uint16_t[message->Length()]);
    message->Write(isolate, buffer.get(), 0, message->Length());

    v8_inspector::StringView message_view(buffer.get(), message->Length());
    {
        v8::SealHandleScope seal_handle_scope(isolate);

        CV8ResourceImpl* resource = static_cast<CV8ResourceImpl*>(V8ResourceImpl::Get(ctx));
        v8_inspector::V8InspectorSession* session = resource->GetInspector()->GetSession();
        session->dispatchProtocolMessage(message_view);
    }
    return resolver->GetPromise();
}

v8::Local<v8::Promise> CV8InspectorClient::SendInspectorMessage(v8::Isolate* isolate, alt::String messageRaw)
{
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    auto id = CV8InspectorClient::GetNextMessageId();

    auto promise = v8::Promise::Resolver::New(ctx);
    v8::Local<v8::Promise::Resolver> resolver;
    if (!promise.ToLocal(&resolver))
        return v8::Local<v8::Promise>();

    promises.emplace(id, v8::Global<v8::Promise::Resolver>(isolate, resolver));

    v8::Local<v8::String> message = v8::String::NewFromUtf8(isolate, messageRaw.CStr()).ToLocalChecked();

    CV8ResourceImpl* resource = static_cast<CV8ResourceImpl*>(V8ResourceImpl::Get(ctx));
    v8_inspector::V8InspectorSession* session = resource->GetInspector()->GetSession();

    std::unique_ptr<uint16_t[]> buffer(new uint16_t[message->Length()]);
    message->Write(isolate, buffer.get(), 0, message->Length());

    v8_inspector::StringView message_view(buffer.get(), message->Length());
    {
        v8::SealHandleScope seal_handle_scope(isolate);
        session->dispatchProtocolMessage(message_view);
    }
    return resolver->GetPromise();
}

uint32_t CV8InspectorClient::lastMessageId = 0;
uint32_t CV8InspectorClient::GetNextMessageId()
{
    return ++lastMessageId;
}

std::unordered_map<uint32_t, v8::Global<v8::Promise::Resolver>> CV8InspectorClient::promises;