
#include "CV8InspectorClient.h"
#include "CV8InspectorChannel.h"

CV8InspectorClient::CV8InspectorClient(v8::Local<v8::Context> context)
{
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
        return v8::Local<v8::Promise>();
    }

    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    auto id = CV8InspectorClient::GetNextMessageId();

    auto promise = v8::Promise::Resolver::New(ctx);
    v8::Local<v8::Promise::Resolver> resolver;
    if (!promise.ToLocal(&resolver))
        return v8::Local<v8::Promise>();

    promises.emplace(id, v8::Global<v8::Promise::Resolver>(isolate, resolver));

    V8_NEW_OBJECT(messageObject);
    V8_OBJECT_SET_INTEGER(messageObject, "id", id);
    V8_OBJECT_SET_STRING(messageObject, "method", method);
    messageObject->Set(ctx, V8_NEW_STRING("params"), params);

    v8::Local<v8::String> message;
    if(!v8::JSON::Stringify(ctx, messageObject).ToLocal(&message))
    {
        Log::Error << "[V8] Failed to parse inspector message" << Log::Endl;
        return v8::Local<v8::Promise>();
    }

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

uint32_t CV8InspectorClient::SendInspectorMessage(v8::Isolate* isolate, const std::string& messageRaw)
{
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    auto id = CV8InspectorClient::GetNextMessageId();
    CV8ResourceImpl* resource = static_cast<CV8ResourceImpl*>(V8ResourceImpl::Get(ctx));
    v8_inspector::V8InspectorSession* session = resource->GetInspector()->GetSession();

    v8::Local<v8::String> message = v8::String::NewFromUtf8(isolate, messageRaw.c_str()).ToLocalChecked();

    std::unique_ptr<uint16_t[]> buffer(new uint16_t[message->Length()]);
    message->Write(isolate, buffer.get(), 0, message->Length());

    v8_inspector::StringView message_view(buffer.get(), message->Length());
    {
        v8::SealHandleScope seal_handle_scope(isolate);
        session->dispatchProtocolMessage(message_view);
    }

    return id;
}

uint32_t CV8InspectorClient::lastMessageId = 0;
uint32_t CV8InspectorClient::GetNextMessageId()
{
    return ++lastMessageId;
}

std::unordered_map<uint32_t, v8::Global<v8::Promise::Resolver>> CV8InspectorClient::promises;
std::unordered_map<uint32_t, ix::WebSocket*> CV8InspectorClient::pendingMessages;

void CV8InspectorClient::CreateWebSocketServer(uint32_t port)
{
    if(_wsServer != nullptr)
    {
        Log::Error << "[V8] The inspector websocket server is already running" << Log::Endl;
        return;
    }

    _wsServer = new ix::WebSocketServer(port);
    _wsServer->setOnClientMessageCallback([this](std::shared_ptr<ix::ConnectionState> connectionState, ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg)
    {
        if(msg->type == ix::WebSocketMessageType::Open)
        {
            Log::Info << "[V8] New connection to inspector websocket server" << Log::Endl;
        }
        else if(msg->type == ix::WebSocketMessageType::Message)
        {
            Log::Warning << msg->str << Log::Endl;
            auto id = CV8InspectorClient::SendInspectorMessage(_isolate, msg->str);
            pendingMessages.emplace(id, &webSocket);
        }
        else if(msg->type == ix::WebSocketMessageType::Error)
        {
            Log::Error << "[V8] Inspector websocket server received error: " << msg->errorInfo.reason << Log::Endl;
        }
    });

    auto result = _wsServer->listen();
    if(!result.first)
    {
        Log::Error << "[V8] Error while starting inspector websocket server: " << result.second << Log::Endl;
        return;
    }
    _wsServer->start();

    Log::Info << "[V8] Started inspector websocket server on port " << std::to_string(port) << Log::Endl;
}
