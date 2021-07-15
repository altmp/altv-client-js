#include "../CV8Resource.h"
#include "../helpers/V8Helpers.h"
#include "cpp-sdk/script-objects/ICheckpoint.h"

static void Constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT_RESOURCE();
	V8_CHECK_CONSTRUCTOR();
	if(info.Length() == 6)
	{
		V8_ARG_TO_INT(1, type);
		V8_ARG_TO_OBJECT(2, pos);
		V8_ARG_TO_OBJECT(3, nextPos);
		V8_ARG_TO_NUMBER(4, radius);
		V8_ARG_TO_NUMBER(5, height);
		V8_ARG_TO_OBJECT(6, color);

		V8_OBJECT_GET_NUMBER(pos, "x", x);
		V8_OBJECT_GET_NUMBER(pos, "y", y);
		V8_OBJECT_GET_NUMBER(pos, "z", z);

		V8_OBJECT_GET_NUMBER(nextPos, "x", x2);
		V8_OBJECT_GET_NUMBER(nextPos, "y", y2);
		V8_OBJECT_GET_NUMBER(nextPos, "z", z2);

		V8_OBJECT_GET_INTEGER(color, "r", r);
		V8_OBJECT_GET_INTEGER(color, "g", g);
		V8_OBJECT_GET_INTEGER(color, "b", b);
		V8_OBJECT_GET_INTEGER(color, "a", a);

		alt::Ref<alt::ICheckpoint> cp = alt::ICore::Instance().CreateCheckpoint(type, { x, y, z }, { x2, y2, z2 }, radius, height, { (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a });
		V8_BIND_BASE_OBJECT(cp, "Failed to create Checkpoint");
	}
	else if(info.Length() == 10)
	{
		V8_ARG_TO_INT(1, type);
		V8_ARG_TO_NUMBER(2, x);
		V8_ARG_TO_NUMBER(3, y);
		V8_ARG_TO_NUMBER(4, z);
		V8_ARG_TO_NUMBER(5, x2);
		V8_ARG_TO_NUMBER(6, y2);
		V8_ARG_TO_NUMBER(7, z2);
		V8_ARG_TO_NUMBER(8, radius);
		V8_ARG_TO_NUMBER(9, height);
		V8_ARG_TO_OBJECT(10, color);

		V8_OBJECT_GET_INTEGER(color, "r", r);
		V8_OBJECT_GET_INTEGER(color, "g", g);
		V8_OBJECT_GET_INTEGER(color, "b", b);
		V8_OBJECT_GET_INTEGER(color, "a", a);

		alt::Ref<alt::ICheckpoint> cp = alt::ICore::Instance().CreateCheckpoint(type, { x, y, z }, { x2, y2, z2 }, radius, height, { (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a });
		V8_BIND_BASE_OBJECT(cp, "Failed to create Checkpoint");
	}
	else V8Helpers::Throw(isolate, "6 or 10 arguments expected");
}

static void TypeGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_GET_THIS_BASE_OBJECT(cp, alt::ICheckpoint);
	V8_RETURN_INT(cp->GetCheckpointType());
}

static void TypeSetter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_GET_THIS_BASE_OBJECT(cp, alt::ICheckpoint);
	V8_TO_INTEGER(value, val);
	cp->SetCheckpointType(val);
}

static void RadiusGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_GET_THIS_BASE_OBJECT(cp, alt::ICheckpoint);
	V8_RETURN_NUMBER(cp->GetRadius());
}

static void RadiusSetter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_GET_THIS_BASE_OBJECT(cp, alt::ICheckpoint);
	V8_TO_NUMBER(value, val);
	cp->SetRadius(val);
}

static void HeightGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_GET_THIS_BASE_OBJECT(cp, alt::ICheckpoint);
	V8_RETURN_NUMBER(cp->GetHeight());
}

static void HeightSetter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_GET_THIS_BASE_OBJECT(cp, alt::ICheckpoint);
	V8_TO_NUMBER(value, val);
	cp->SetHeight(val);
}

static void ColorGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT_RESOURCE();
	V8_GET_THIS_BASE_OBJECT(cp, alt::ICheckpoint);
	V8_RETURN(resource->CreateRGBA(cp->GetColor()));
}

static void ColorSetter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_GET_THIS_BASE_OBJECT(cp, alt::ICheckpoint);

	V8_TO_OBJECT(value, color);
	V8_OBJECT_GET_INTEGER(color, "r", r);
	V8_OBJECT_GET_INTEGER(color, "g", g);
	V8_OBJECT_GET_INTEGER(color, "b", b);
	V8_OBJECT_GET_INTEGER(color, "a", a);

	cp->SetColor({ (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a });
}

static void NextPosGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT_RESOURCE();
	V8_GET_THIS_BASE_OBJECT(cp, alt::ICheckpoint);

	V8_RETURN(resource->CreateVector3(cp->GetNextPosition()));
}

static void NextPosSetter(v8::Local<v8::String>, v8::Local<v8::Value> val, const v8::PropertyCallbackInfo<void>& info)
{
	V8_GET_ISOLATE_CONTEXT();
	V8_GET_THIS_BASE_OBJECT(cp, alt::ICheckpoint);

	V8_TO_OBJECT(val, pos);
	V8_OBJECT_GET_NUMBER(pos, "x", x);
	V8_OBJECT_GET_NUMBER(pos, "y", y);
	V8_OBJECT_GET_NUMBER(pos, "z", z);

	cp->SetNextPosition({ x, y, z });
}

static void IsEntityIn(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT_RESOURCE();
	V8_GET_THIS_BASE_OBJECT(cp, alt::ICheckpoint);

	V8_ARG_TO_BASE_OBJECT(1, ent, alt::IEntity, "IEntity");

	V8_RETURN_BOOLEAN(cp->IsEntityIn(ent));
}

static void IsPointIn(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT_RESOURCE();
	V8_GET_THIS_BASE_OBJECT(cp, alt::ICheckpoint);

	V8_ARG_TO_OBJECT(1, pos);
	V8_OBJECT_GET_NUMBER(pos, "x", x);
	V8_OBJECT_GET_NUMBER(pos, "y", y);
	V8_OBJECT_GET_NUMBER(pos, "z", z);

	V8_RETURN_BOOLEAN(cp->IsPointIn({ x, y, z}));
}

extern V8Class v8WorldObject;
extern V8Class v8Checkpoint("Checkpoint", v8WorldObject, Constructor, [](v8::Local<v8::FunctionTemplate> tpl) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();

	V8::SetAccessor(isolate, tpl, "checkpointType", &TypeGetter, &TypeSetter);
	V8::SetAccessor(isolate, tpl, "radius", &RadiusGetter, &RadiusSetter);
	V8::SetAccessor(isolate, tpl, "height", &HeightGetter, &HeightSetter);
	V8::SetAccessor(isolate, tpl, "color", &ColorGetter, &ColorSetter);
	V8::SetAccessor(isolate, tpl, "nextPos", &NextPosGetter, &NextPosSetter);

	V8::SetMethod(isolate, tpl, "isEntityIn", &IsEntityIn);
	V8::SetMethod(isolate, tpl, "isPointIn", &IsPointIn);
});
