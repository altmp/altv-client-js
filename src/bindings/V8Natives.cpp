
#include "../helpers/V8Helpers.h"
#include "../helpers/V8Module.h"
#include "../helpers/Log.h"
#include "../helpers/V8ResourceImpl.h"

static uint64_t pointers[32];
static uint32_t pointersCount = 0;

static uint32_t returnsCount = 1;

static char *SaveString(const char *str)
{
	static char *stringValues[256] = {0};
	static int nextString = 0;

	if (stringValues[nextString])
		free(stringValues[nextString]);

	char *_str = _strdup(str);
	stringValues[nextString] = _str;
	nextString = (nextString + 1) % 256;

	return _str;
}

template <class T>
static T *SavePointer(T val)
{
	T *ptr = reinterpret_cast<T *>(&pointers[pointersCount++]);
	*ptr = val;
	return ptr;
}

template <>
static alt::INative::Vector3 *SavePointer(alt::INative::Vector3 val)
{
	alt::INative::Vector3 *ptr = reinterpret_cast<alt::INative::Vector3 *>(&pointers[pointersCount]);
	pointersCount += 3;
	*ptr = val;
	return ptr;
}

static void *ToMemoryBuffer(v8::Local<v8::Value> val, v8::Local<v8::Context> ctx)
{
	if (val->IsObject())
	{
		v8::Local<v8::Object> obj = val.As<v8::Object>();

		if (obj->InternalFieldCount() == 2)
		{
			void *memory = obj->GetAlignedPointerFromInternalField(0);
			uint32_t size = obj->GetInternalField(0)->Uint32Value(ctx).ToChecked();

			if (size > 0)
				return memory;
		}
	}

	return nullptr;
}

static const char* GetNativeTypeName(alt::INative::Type type)
{
	using Type = alt::INative::Type;
	switch(type)
	{
		case Type::ARG_BOOL: 
		case Type::ARG_BOOL_PTR:
			return "bool";
		case Type::ARG_INT32:
		case Type::ARG_INT32_PTR:
			return "int32";
		case Type::ARG_UINT32:
		case Type::ARG_UINT32_PTR:
			return "uint32";
		case Type::ARG_FLOAT:
		case Type::ARG_FLOAT_PTR:
			return "float";
		case Type::ARG_VECTOR3:
		case Type::ARG_VECTOR3_PTR:
			return "vector3";
		case Type::ARG_STRING:
			return "string";
		case Type::ARG_STRUCT:
			return "struct";
		case Type::ARG_VOID:
			return "void";
	}
	return "unknown";
}

static void PushArg(alt::Ref<alt::INative::Context> scrCtx, alt::INative* native, alt::INative::Type argType, v8::Isolate *isolate, V8ResourceImpl* resource, v8::Local<v8::Value> val)
{
	using ArgType = alt::INative::Type;

	v8::Local<v8::Context> v8Ctx = isolate->GetEnteredOrMicrotaskContext();
	
	switch (argType)
	{
	case alt::INative::Type::ARG_BOOL:
		scrCtx->Push((int32_t)val->ToBoolean(isolate)->Value());
		break;
	case alt::INative::Type::ARG_BOOL_PTR:
		++returnsCount;
		scrCtx->Push(SavePointer((int32_t)val->ToBoolean(isolate)->Value()));
		break;
	case alt::INative::Type::ARG_INT32:
	{
		if (val->IsNumber())
		{
			v8::Local<v8::Integer> value;
			if (val->ToInteger(v8Ctx).ToLocal(&value))
			{
				scrCtx->Push((int32_t)value->Value());
			}
			else
			{
				Log::Error << "Native argument " << "(" << V8::GetJSValueTypeName(val) << ")" << " could not be parsed to type " << GetNativeTypeName(argType) << " (" << native->GetName() << ")" << Log::Endl;
				scrCtx->Push(0);
			}
		}
		else if (val->IsBigInt())
		{
			v8::Local<v8::BigInt> value;
			if (val->ToBigInt(v8Ctx).ToLocal(&value))
			{
				scrCtx->Push((int32_t)value->Int64Value());
			}
			else
			{
				Log::Error << "Native argument " << "(" << V8::GetJSValueTypeName(val) << ")" << " could not be parsed to type " << GetNativeTypeName(argType) << " (" << native->GetName() << ")" << Log::Endl;
				scrCtx->Push(0);
			}
		}
		else if (val->IsObject())
		{
			auto ent = V8Entity::Get(val);
			if(ent != nullptr) scrCtx->Push(ent->GetHandle().As<alt::IEntity>()->GetScriptGuid());
			else scrCtx->Push(0);
		}
		else
		{
			Log::Error << "Native argument " << "(" << V8::GetJSValueTypeName(val) << ")" << " could not be parsed to type " << GetNativeTypeName(argType) << " (" << native->GetName() << ")" << Log::Endl;
			scrCtx->Push(0);
		}
		break;
	}
	case alt::INative::Type::ARG_INT32_PTR:
		++returnsCount;
		scrCtx->Push(SavePointer((int32_t)val->ToInteger(v8Ctx).ToLocalChecked()->Value()));
		break;
	case alt::INative::Type::ARG_UINT32:
	{
		if (val->IsNumber())
		{
			v8::Local<v8::Integer> value;
			if (val->ToInteger(v8Ctx).ToLocal(&value))
			{
				scrCtx->Push((uint32_t)value->Value());
			}
			else
			{
				Log::Error << "Native argument " << "(" << V8::GetJSValueTypeName(val) << ")" << " could not be parsed to type " << GetNativeTypeName(argType) << " (" << native->GetName() << ")" << Log::Endl;
				scrCtx->Push(0);
			}
		}
		else if (val->IsBigInt())
		{
			v8::Local<v8::BigInt> value;
			if (val->ToBigInt(v8Ctx).ToLocal(&value))
			{
				scrCtx->Push((uint32_t)value->Int64Value());
			}
			else
			{
				Log::Error << "Native argument " << "(" << V8::GetJSValueTypeName(val) << ")" << " could not be parsed to type " << GetNativeTypeName(argType) << " (" << native->GetName() << ")" << Log::Endl;
				scrCtx->Push(0);
			}
		}
		else
		{
			Log::Error << "Native argument " << "(" << V8::GetJSValueTypeName(val) << ")" << " could not be parsed to type " << GetNativeTypeName(argType) << " (" << native->GetName() << ")" << Log::Endl;
			scrCtx->Push(0);
		}
		break;
	}
	case alt::INative::Type::ARG_UINT32_PTR:
		++returnsCount;
		scrCtx->Push(SavePointer((uint32_t)val->ToInteger(v8Ctx).ToLocalChecked()->Value()));
		break;
	case alt::INative::Type::ARG_FLOAT:
	{
		if (val->IsNumber())
		{
			v8::Local<v8::Number> value;
			if (val->ToNumber(v8Ctx).ToLocal(&value))
			{
				scrCtx->Push((float)value->Value());
			}
			else
			{
				Log::Error << "Native argument " << "(" << V8::GetJSValueTypeName(val) << ")" << " could not be parsed to type " << GetNativeTypeName(argType) << " (" << native->GetName() << ")" << Log::Endl;
				scrCtx->Push(0.f);
			}
		}
		else
		{
			Log::Error << "Native argument " << "(" << V8::GetJSValueTypeName(val) << ")" << " could not be parsed to type " << GetNativeTypeName(argType) << " (" << native->GetName() << ")" << Log::Endl;
			scrCtx->Push(0.f);
		}
		break;
	}
	case alt::INative::Type::ARG_FLOAT_PTR:
		++returnsCount;
		scrCtx->Push(SavePointer((float)val->ToNumber(v8Ctx).ToLocalChecked()->Value()));
		break;
	case alt::INative::Type::ARG_VECTOR3_PTR:
		++returnsCount;
		scrCtx->Push(SavePointer(alt::INative::Vector3{})); // TODO: Add initializer
		break;
	case alt::INative::Type::ARG_STRING:
		if (val->IsString())
			scrCtx->Push(SaveString(*v8::String::Utf8Value(isolate, val->ToString(v8Ctx).ToLocalChecked())));
		else
			scrCtx->Push((char *)nullptr);
		break;
	case alt::INative::Type::ARG_STRUCT:
	{
		auto buffer = ToMemoryBuffer(val, v8Ctx);
		if(buffer != nullptr) scrCtx->Push(buffer);
		else
		{
			Log::Error << "Native argument " << "(" << V8::GetJSValueTypeName(val) << ")" << " could not be parsed to type " << GetNativeTypeName(argType) << " (" << native->GetName() << ")" << Log::Endl;
			scrCtx->Push((void*)nullptr);
		}
		break;
	}
	default:
		Log::Error << "Unknown native arg type " << (int)argType << " (" << native->GetName() << ")" << Log::Endl;
	}
}

static void PushPointerReturn(alt::INative::Type argType, v8::Local<v8::Array> retns, v8::Isolate *isolate, v8::Local<v8::Context> ctx)
{
	using ArgType = alt::INative::Type;

	switch (argType)
	{
	case alt::INative::Type::ARG_BOOL_PTR:
		retns->Set(ctx, returnsCount++, v8::Boolean::New(isolate, *reinterpret_cast<int32_t *>(&pointers[pointersCount++])));
		break;
	case alt::INative::Type::ARG_INT32_PTR:
		retns->Set(ctx, returnsCount++, v8::Integer::New(isolate, *reinterpret_cast<int32_t *>(&pointers[pointersCount++])));
		break;
	case alt::INative::Type::ARG_UINT32_PTR:
		retns->Set(ctx, returnsCount++, v8::Integer::NewFromUnsigned(isolate, *reinterpret_cast<uint32_t *>(&pointers[pointersCount++])));
		break;
	case alt::INative::Type::ARG_FLOAT_PTR:
		retns->Set(ctx, returnsCount++, v8::Number::New(isolate, *reinterpret_cast<float *>(&pointers[pointersCount++])));
		break;
	case alt::INative::Type::ARG_VECTOR3_PTR:
	{
		alt::INative::Vector3 *val = reinterpret_cast<alt::INative::Vector3 *>(&pointers[pointersCount]);
		pointersCount += 3;

		v8::Local<v8::Context> v8Ctx = isolate->GetEnteredOrMicrotaskContext();
		V8ResourceImpl* resource = V8ResourceImpl::Get(v8Ctx);
		auto vector = resource->CreateVector3({ val->x, val->y, val->z }).As<v8::Object>();

		retns->Set(ctx, returnsCount++, vector);
		break;
	}
	}
}

static v8::Local<v8::Value> GetReturn(alt::Ref<alt::INative::Context> scrCtx, alt::INative* native, alt::INative::Type retnType, v8::Isolate *isolate)
{
	using ArgType = alt::INative::Type;

	v8::Local<v8::Context> v8Ctx = isolate->GetEnteredOrMicrotaskContext();

	switch (retnType)
	{
	case alt::INative::Type::ARG_BOOL:
		return v8::Boolean::New(isolate, scrCtx->ResultBool());
	case alt::INative::Type::ARG_INT32:
		return v8::Integer::New(isolate, scrCtx->ResultInt());
	case alt::INative::Type::ARG_UINT32:
		return v8::Integer::NewFromUnsigned(isolate, scrCtx->ResultUint());
	case alt::INative::Type::ARG_FLOAT:
		return v8::Number::New(isolate, scrCtx->ResultFloat());
	case alt::INative::Type::ARG_VECTOR3:
	{
		alt::INative::Vector3 val = scrCtx->ResultVector3();
		V8ResourceImpl* resource = V8ResourceImpl::Get(v8Ctx);
		auto vector = resource->CreateVector3({ val.x, val.y, val.z }).As<v8::Object>();
		return vector;
	}
	case alt::INative::Type::ARG_STRING:
		if (!scrCtx->ResultString())
			return v8::Null(isolate);

		return v8::String::NewFromUtf8(isolate, scrCtx->ResultString()).ToLocalChecked();
	case alt::INative::Type::ARG_VOID:
		return v8::Undefined(isolate);
	default:
		Log::Error << "Unknown native return type " << (int)retnType << " (" << native->GetName() << ")" << Log::Endl;
		return v8::Undefined(isolate);
	}
}

static void InvokeNative(const v8::FunctionCallbackInfo<v8::Value> &info)
{
	static auto ctx = alt::ICore::Instance().CreateNativesContext();

	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> v8Ctx = isolate->GetCurrentContext();

	auto native = static_cast<alt::INative *>(info.Data().As<v8::External>()->Value());

	if (!native->IsValid())
	{
		info.GetReturnValue().Set(v8::Boolean::New(isolate, false));
		return;
	}

	auto args = native->GetArgTypes();
	uint32_t argsSize = args.GetSize();

	ctx->Reset();
	pointersCount = 0;
	returnsCount = 1;

	auto resource = V8ResourceImpl::Get(v8Ctx);
	for (uint32_t i = 0; i < argsSize; ++i)
		PushArg(ctx, native, args[i], isolate, resource, info[i]);

	if (!native->Invoke(ctx))
	{
		V8Helpers::Throw(isolate, "Native call failed");
		return;
	}

	if (returnsCount == 1)
	{
		info.GetReturnValue().Set(GetReturn(ctx, native, native->GetRetnType(), isolate));
	}
	else
	{
		v8::Local<v8::Array> retns = v8::Array::New(isolate, returnsCount);
		retns->Set(v8Ctx, 0, GetReturn(ctx, native, native->GetRetnType(), isolate));

		pointersCount = 0;
		returnsCount = 1;

		for (uint32_t i = 0; i < argsSize; ++i)
			PushPointerReturn(args[i], retns, isolate, v8Ctx);

		info.GetReturnValue().Set(retns);
	}
}

static void RegisterNatives(v8::Local<v8::Context> ctx, v8::Local<v8::Object> exports)
{
	v8::Isolate *isolate = v8::Isolate::GetCurrent();

	for (auto native : alt::ICore::Instance().GetAllNatives())
	{
		V8::SetFunction(isolate, ctx, exports, native->GetName().CStr(), InvokeNative, native);
	}
}

extern V8Module nativesModule("natives", {}, RegisterNatives);
