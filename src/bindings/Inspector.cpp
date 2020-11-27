#include "../helpers/V8Class.h"
#include "../helpers/V8Helpers.h"
#include "../helpers/V8ResourceImpl.h"
#include "../CV8Resource.h"
#include "cpp-sdk/SDK.h"
#include "inspector/CV8InspectorClient.h"

static void SetInspectorCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_CHECK_ARGS_LEN(1);
	V8_ARG_TO_FUNCTION(1, callback);

	auto res = static_cast<CV8ResourceImpl*>(CV8ResourceImpl::Get(ctx));
	res->GetInspector()->SetCallback(callback, isolate);

	V8_RETURN(v8::Undefined(isolate));
}

extern V8Class v8Inspector("Inspector", nullptr, [](v8::Local<v8::FunctionTemplate> tpl) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();

	V8::SetStaticMethod(isolate, tpl, "setCallback", &SetInspectorCallback);
	V8::SetStaticMethod(isolate, tpl, "sendMessage", &CV8InspectorClient::SendInspectorMessage);
});
