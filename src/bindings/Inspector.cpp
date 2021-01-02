#include "../helpers/V8Class.h"
#include "../helpers/V8Helpers.h"
#include "../helpers/V8ResourceImpl.h"
#include "../CV8Resource.h"
#include "cpp-sdk/SDK.h"

#include "../inspector/CV8InspectorClient.h"
#include "../CV8Resource.h"

static void SetInspectorCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_CHECK(alt::ICore::Instance().IsDebug(), "The inspector is only available in debug mode");
	V8_CHECK_ARGS_LEN(1);
	V8_ARG_TO_FUNCTION(1, callback);

	auto res = static_cast<CV8ResourceImpl*>(CV8ResourceImpl::Get(ctx));
	res->GetInspector()->SetCallback(isolate, callback);
}

static void SendInspectorMessage(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_CHECK(alt::ICore::Instance().IsDebug(), "The inspector is only available in debug mode");
	V8_CHECK_ARGS_LEN2(1, 2);
	V8_ARG_TO_STRING(1, method);

	v8::Local<v8::Object> params = v8::Object::New(isolate);
	if (info.Length() == 2) 
	{
		bool result = V8::SafeToObject(info[2 - 1], ctx, params);
		V8_CHECK(result, "Failed to convert argument 2 to object")
	}

	auto promise = CV8InspectorClient::SendInspectorMessage(isolate, method, params);

	V8_RETURN(promise);
}

static void SendInspectorMessageRaw(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_CHECK(alt::ICore::Instance().IsDebug(), "The inspector is only available in debug mode");
	V8_CHECK_ARGS_LEN(1);
	V8_ARG_TO_STRING(1, message);

	auto promise = CV8InspectorClient::SendInspectorMessage(isolate, message);

	info.GetReturnValue().Set(promise);
}

static void SetupWebSocketServer(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_CHECK(alt::ICore::Instance().IsDebug(), "The inspector is only available in debug mode");
	V8_CHECK_ARGS_LEN(1);
	V8_ARG_TO_UINT32(1, port);

	auto res = static_cast<CV8ResourceImpl*>(CV8ResourceImpl::Get(ctx));
	res->GetInspector()->CreateWebSocketServer(port);
}

extern V8Class v8Inspector("Inspector", nullptr, [](v8::Local<v8::FunctionTemplate> tpl) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();

	V8::SetStaticMethod(isolate, tpl, "setEventCallback", &SetInspectorCallback);
	V8::SetStaticMethod(isolate, tpl, "sendMessage", &SendInspectorMessage);
	V8::SetStaticMethod(isolate, tpl, "sendMessageRaw", &SendInspectorMessageRaw);
	V8::SetStaticMethod(isolate, tpl, "startWebSocketServer", &SetupWebSocketServer);
});
