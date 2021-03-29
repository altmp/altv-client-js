#include "../CV8Resource.h"
#include "../helpers/V8Helpers.h"
#include <chrono>

// Helper function
static int64_t GetTimestamp()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

static void Start(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();

    auto timestamp = GetTimestamp();
    // Start
    info.This()->SetInternalField(0, v8::BigInt::New(isolate, timestamp));
    // End
    info.This()->SetInternalField(1, v8::BigInt::New(isolate, timestamp));
}

static void Stop(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();

    auto end = GetTimestamp();
    // End
    info.This()->SetInternalField(1, v8::BigInt::New(isolate, end));
}

static void GetResult(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_ARG_TO_INTEGER(1, precision);

    auto start = info.This()->GetInternalField(0).As<v8::BigInt>()->Int64Value();
    auto end = info.This()->GetInternalField(1).As<v8::BigInt>()->Int64Value();
    auto duration = end - start;

    switch(precision)
    {
        // Milliseconds
        case 0:
        {
            V8_RETURN_INTEGER((duration / 1000) / 1000);
            break;
        }
        // Microseconds
        case 1:
        {
            V8_RETURN_INTEGER(duration / 1000);
            break;
        }
        // Nanoseconds
        case 2:
        {
            V8_RETURN_INTEGER(duration);
            break;
        }
    }
}

static void Constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_CHECK_CONSTRUCTOR();
    V8_ARG_TO_BOOLEAN_OPT(1, start, true);

    if(start) Start(info);
}

extern V8Class v8Benchmark("Benchmark", Constructor, [](v8::Local<v8::FunctionTemplate> tpl) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();

    tpl->InstanceTemplate()->SetInternalFieldCount(2);

    V8::SetMethod(isolate, tpl, "start", Start);
    V8::SetMethod(isolate, tpl, "stop", Stop);

    V8::SetMethod(isolate, tpl, "getResult", GetResult);
});
