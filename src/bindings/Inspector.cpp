#include "../helpers/V8Class.h"
#include "../helpers/V8Helpers.h"
#include "../helpers/V8ResourceImpl.h"
#include "../CV8Resource.h"
#include "cpp-sdk/SDK.h"

#include "../inspector/CV8InspectorClient.h"

extern class CV8ResourceImpl;

/*static void SetInspectorCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_CHECK_ARGS_LEN(1);
	V8_ARG_TO_FUNCTION(1, callback);

	auto res = static_cast<CV8ResourceImpl*>(CV8ResourceImpl::Get(ctx));
	res->GetInspector()->SetCallback(callback, isolate);

	V8_RETURN(v8::Undefined(isolate));
}*/

static void SendInspectorMessage(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_CHECK_ARGS_LEN2(1, 2);
	V8_ARG_TO_STRING(1, method);
	v8::Local<v8::Object> params = v8::Object::New(isolate);
	if (info.Length() == 2) 
	{
		bool result = V8::SafeToObject(info[2 - 1], ctx, params);
		V8_CHECK(result, "Failed to convert argument 2 to object")
	}

	auto promise = CV8InspectorClient::SendInspectorMessage(isolate, method, params);

	info.GetReturnValue().Set(promise);
}

extern V8Class v8Inspector("Inspector", nullptr, [](v8::Local<v8::FunctionTemplate> tpl) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();

	//V8::SetStaticMethod(isolate, tpl, "setCallback", &SetInspectorCallback);
	V8::SetStaticMethod(isolate, tpl, "sendMessage", &SendInspectorMessage);
});
