#include "../helpers/V8Helpers.h"
#include "../helpers/V8Class.h"
#include "../helpers/V8Entity.h"
#include "../helpers/V8ResourceImpl.h"
#include "../CV8Resource.h"
#include "v8-profiler.h"

// *** Heap Sampling ***

static void StartHeapSampling(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_INTERNAL_FIELD_EXTERNAL(1, profiler, v8::HeapProfiler);

    auto result = profiler->StartSamplingHeapProfiler();
    V8_CHECK(result, "Heap sampling is already active");
}

static void StopHeapSampling(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_INTERNAL_FIELD_EXTERNAL(1, profiler, v8::HeapProfiler);

    profiler->StopSamplingHeapProfiler();
}

static v8::Local<v8::Object> SerializeHeapSampleNode(v8::Isolate* isolate, v8::Local<v8::Context> ctx, v8::AllocationProfile::Node* node, std::vector<v8::AllocationProfile::Sample> samples)
{
    V8_NEW_OBJECT(data);
    data->Set(ctx, V8_NEW_STRING("function"), node->name);
    data->Set(ctx, V8_NEW_STRING("script"), node->script_name);
    data->Set(ctx, V8_NEW_STRING("line"), v8::Integer::New(isolate, node->line_number));

    auto children = node->children;
    auto childrenArray = v8::Array::New(isolate, children.size());
    for(auto child : children)
    {
        childrenArray->Set(ctx, childrenArray->Length(), SerializeHeapSampleNode(isolate, ctx, child, samples));
    }
    data->Set(ctx, V8_NEW_STRING("children"), childrenArray);

    v8::AllocationProfile::Sample* bestSample = nullptr;
    for(auto& sample : samples)
    {
        if(sample.node_id == node->node_id && (bestSample == nullptr || bestSample->sample_id < sample.sample_id))
        {
            bestSample = &sample;
        }
    }
    if(bestSample != nullptr)
    {
        V8_NEW_OBJECT(sampleObj);
        V8_OBJECT_SET_INTEGER(sampleObj, "size", bestSample->size);
        V8_OBJECT_SET_INTEGER(sampleObj, "count", bestSample->count);
        data->Set(ctx, V8_NEW_STRING("latestSample"), sampleObj);
    }

    auto allocations = node->allocations;
    auto allocationsArray = v8::Array::New(isolate, allocations.size());
    for(auto& allocation : allocations)
    {
        V8_NEW_OBJECT(allocationObj);
        V8_OBJECT_SET_INTEGER(allocationObj, "size", allocation.size);
        V8_OBJECT_SET_INTEGER(allocationObj, "count", allocation.count);

        allocationsArray->Set(ctx, allocationsArray->Length(), allocationObj);
    }

    return data;
}

static void GetHeapSample(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_INTERNAL_FIELD_EXTERNAL(1, profiler, v8::HeapProfiler);

    // Get the heap sample
    auto profile = profiler->GetAllocationProfile();
    auto samples = profile->GetSamples();

    V8_NEW_OBJECT(data);
    auto arr = v8::Array::New(isolate, 1);
    auto root = profile->GetRootNode();
    auto obj = SerializeHeapSampleNode(isolate, ctx, root, samples);
    data->Set(ctx, V8_NEW_STRING("rootNode"), obj);

    V8_OBJECT_SET_INTEGER(data, "sampleCount", samples.size());

    V8_RETURN(data);
}

// *******

// *** CPU Profiling ***

static void StartCpuProfiling(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_INTERNAL_FIELD_EXTERNAL(2, profiler, v8::CpuProfiler);
    V8_CHECK_ARGS_LEN(1);

    V8_ARG_TO_STRING(1, profileName);
    profiler->StartProfiling(V8_NEW_STRING(profileName.CStr()));
}

static v8::Local<v8::Object> SerializeCpuProfileNode(v8::Isolate* isolate, v8::Local<v8::Context> ctx, const v8::CpuProfileNode* node)
{
    V8_NEW_OBJECT(data);
    data->Set(ctx, V8_NEW_STRING("function"), node->GetFunctionName());
    data->Set(ctx, V8_NEW_STRING("script"), node->GetScriptResourceName());
    data->Set(ctx, V8_NEW_STRING("line"), v8::Integer::New(isolate, node->GetLineNumber()));
    data->Set(ctx, V8_NEW_STRING("source"), v8::Integer::New(isolate, node->GetSourceType()));

    auto childrenCount = node->GetChildrenCount();
    auto childrenArray = v8::Array::New(isolate, childrenCount);
    for(int i = 0; i < childrenCount; i++)
    {
        auto child = node->GetChild(i);
        auto childObj = SerializeCpuProfileNode(isolate, ctx, child);
        childrenArray->Set(ctx, childrenArray->Length(), childObj);
    }

    return data;
}

static void StopCpuProfiling(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_INTERNAL_FIELD_EXTERNAL(2, profiler, v8::CpuProfiler);
    V8_CHECK_ARGS_LEN(1);

    V8_ARG_TO_STRING(1, profileName);
    auto profile = profiler->StopProfiling(V8_NEW_STRING(profileName.CStr()));
    V8_CHECK(profile != nullptr, "You have not started cpu profiling");

    V8_NEW_OBJECT(data);
    auto root = profile->GetTopDownRoot();
    data->Set(ctx, V8_NEW_STRING("rootNode"), SerializeCpuProfileNode(isolate, ctx, root));
    V8_OBJECT_SET_INTEGER(data, "startTimestamp", profile->GetStartTime() / 1000);
    V8_OBJECT_SET_INTEGER(data, "endTimestamp", profile->GetEndTime() / 1000);

    V8_RETURN(data);
}

// *******

class CodeEventHandler : public v8::CodeEventHandler
{
public:
    CodeEventHandler(v8::Isolate* isolate) : v8::CodeEventHandler(isolate)
    {

    }
    ~CodeEventHandler()
    {
        listeners.clear();
    }

    void Handle(v8::CodeEvent* event)
    {
        if(!resource->GetResource()->IsStarted()) return delete this;
        if(event->GetCodeType() == v8::CodeEventType::kBuiltinType ||
           event->GetCodeType() == v8::CodeEventType::kBytecodeHandlerType) return;

        auto data = SerializeEvent(event);
        for(auto& listener : listeners)
        {
            listener.Get(resource->GetIsolate())->Call(resource->GetContext(), v8::Undefined(resource->GetIsolate()), 1, &(data.As<v8::Value>()));
        }
    }

    v8::Local<v8::Object> SerializeEvent(v8::CodeEvent* event)
    {
        auto isolate = resource->GetIsolate();
        auto obj = v8::Object::New(isolate);
        obj->Set(resource->GetContext(), V8_NEW_STRING("function"), event->GetFunctionName());
        obj->Set(resource->GetContext(), V8_NEW_STRING("script"), event->GetScriptName());
        obj->Set(resource->GetContext(), V8_NEW_STRING("line"), v8::Integer::New(isolate, event->GetScriptLine()));
        obj->Set(resource->GetContext(), V8_NEW_STRING("type"), v8::Integer::New(isolate, event->GetCodeType()));
        obj->Set(resource->GetContext(), V8_NEW_STRING("typeName"), V8_NEW_STRING(v8::CodeEvent::GetCodeEventTypeName(event->GetCodeType())));
        obj->Set(resource->GetContext(), V8_NEW_STRING("size"), v8::Integer::New(isolate, event->GetCodeSize()));
        obj->Set(resource->GetContext(), V8_NEW_STRING("comment"), V8_NEW_STRING(event->GetComment()));

        return obj;
    }

    void AddListener(v8::Local<v8::Function> listener)
    {
        listeners.emplace_back(v8::UniquePersistent<v8::Function>(resource->GetIsolate(), listener));
    }

    void SetResource(CV8ResourceImpl* _resource)
    {
        resource = _resource;
    }

private:
    CV8ResourceImpl* resource;
    std::vector<v8::UniquePersistent<v8::Function>> listeners;
};

static void SetCodeEventListener(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_INTERNAL_FIELD_EXTERNAL(3, codeEventHandler, CodeEventHandler);
    V8_CHECK_ARGS_LEN(1);

    V8_ARG_TO_FUNCTION(1, listener);
    codeEventHandler->AddListener(listener);
}

static void ToggleCodeEvents(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_INTERNAL_FIELD_EXTERNAL(3, codeEventHandler, CodeEventHandler);
    V8_CHECK_ARGS_LEN(1);

    V8_ARG_TO_BOOLEAN(1, toggle);
    if(toggle) codeEventHandler->Enable();
    else codeEventHandler->Disable();
}

static void Constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT_RESOURCE();
    V8_CHECK_CONSTRUCTOR();
    V8_CHECK(alt::ICore::Instance().IsDebug(), "The profiler is only available in debug mode");

    info.This()->SetInternalField(0, v8::External::New(isolate, isolate->GetHeapProfiler())); // Heap Profiler
    info.This()->SetInternalField(1, v8::External::New(isolate, v8::CpuProfiler::New(isolate))); // CPU Profiler
    auto codeEventHandler = new CodeEventHandler(isolate);
    codeEventHandler->SetResource(static_cast<CV8ResourceImpl*>(resource));
    info.This()->SetInternalField(2, v8::External::New(isolate, codeEventHandler)); // Code Event Handler class
}

static void Destroy(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();

    V8_GET_THIS_INTERNAL_FIELD_EXTERNAL(2, cpuProfiler, v8::CpuProfiler);
    cpuProfiler->Dispose();
    V8_GET_THIS_INTERNAL_FIELD_EXTERNAL(3, codeEventHandler, CodeEventHandler);
    delete codeEventHandler;
}

extern V8Class v8Profiler("Profiler", Constructor, [](v8::Local<v8::FunctionTemplate> tpl) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::CpuProfiler::UseDetailedSourcePositionsForProfiling(isolate);

    tpl->InstanceTemplate()->SetInternalFieldCount(3);

    V8::SetMethod(isolate, tpl, "destroy", &Destroy);

    V8::SetMethod(isolate, tpl, "startHeapSampling", &StartHeapSampling);
    V8::SetMethod(isolate, tpl, "stopHeapSampling", &StopHeapSampling);
    V8::SetMethod(isolate, tpl, "getHeapSample", &GetHeapSample);

    V8::SetMethod(isolate, tpl, "startCpuProfiling", &StartCpuProfiling);
    V8::SetMethod(isolate, tpl, "stopCpuProfiling", &StopCpuProfiling);

    V8::SetMethod(isolate, tpl, "toggleCodeEvents", &ToggleCodeEvents);
    V8::SetMethod(isolate, tpl, "registerCodeEventListener", &SetCodeEventListener);
});
