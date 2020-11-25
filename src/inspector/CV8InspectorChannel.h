#pragma once

#include "v8-inspector.h"
#include "../helpers/V8Helpers.h"
#include "CV8InspectorClient.h";

class CV8InspectorChannel : public v8_inspector::V8Inspector::Channel
{
public:
	explicit CV8InspectorChannel(v8::Local<v8::Context> context, CV8InspectorClient* client)
    {
		_isolate = context->GetIsolate();
		_context.Reset(_isolate, context);
        _client = client;
	}
	~CV8InspectorChannel() override = default;
    CV8InspectorClient* GetClient() 
    {
        return _client;
    }

private:
    void sendResponse(int callId, std::unique_ptr<v8_inspector::StringBuffer> message) override 
    {
        //Log::Info << __FUNCTION__ << " ID: " << callId << Log::Endl;;
        Send(message->string());
    }
    void sendNotification(std::unique_ptr<v8_inspector::StringBuffer> message) override 
    {
        //Log::Info << __FUNCTION__ << Log::Endl;
        Send(message->string());
    }
    void flushProtocolNotifications() override 
    {
        //Log::Info << __FUNCTION__ << Log::Endl;
    }
    void Send(const v8_inspector::StringView& string);

    v8::Isolate* _isolate;
    v8::Global<v8::Context> _context;
    CV8InspectorClient* _client;
};
