
#include "CV8InspectorClient.h"
#include "CV8InspectorChannel.h"

CV8InspectorClient::CV8InspectorClient(v8::Local<v8::Context> context, bool connect)
{
    if (!connect) return;

    _isolate = context->GetIsolate();
    auto isolate = _isolate;
    _channel.reset(new CV8InspectorChannel(context, this));
    _inspector = v8_inspector::V8Inspector::create(isolate, this);
    _session =
        _inspector->connect(1, _channel.get(), v8_inspector::StringView());
    context->SetAlignedPointerInEmbedderData(2, this);
    _inspector->contextCreated(v8_inspector::V8ContextInfo(
        context, 1, v8_inspector::StringView()));
    /*v8::Local<v8::Value> function =
        v8::FunctionTemplate::New(_isolate, CV8InspectorClient::SendInspectorMessage)
        ->GetFunction(context)
        .ToLocalChecked();
    bool isSet =
        context->Global()->Set(
            context,
            v8::String::NewFromUtf8(isolate, "__send").ToLocalChecked(),
            function
        ).FromJust();
    if (!isSet) {
        Log::Error << "Could not set global inspector send function" << Log::Endl;
        return;
    }*/

    _context.Reset(isolate, context);
}

static void CV8InspectorClient::SendInspectorMessage(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Isolate* isolate = info.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    V8_RETURN(v8::Undefined(isolate));

    v8::Local<v8::String> message = info[0]->ToString(context).ToLocalChecked();

    v8_inspector::V8InspectorSession* session =
        CV8InspectorClient::GetSession(context);
    std::unique_ptr<uint16_t[]> buffer(new uint16_t[message->Length()]);
    message->Write(isolate, buffer.get(), 0, message->Length());
    v8_inspector::StringView message_view(buffer.get(), message->Length());
    {
        v8::SealHandleScope seal_handle_scope(isolate);
        session->dispatchProtocolMessage(message_view);
    }
    V8_RETURN_BOOLEAN(true);
}
