#include "../helpers/V8Helpers.h"
#include "../helpers/V8Class.h"
#include "../helpers/V8Entity.h"
#include "../helpers/V8ResourceImpl.h"
#include "../CV8Resource.h"
#include "v8-profiler.h"

static void StartHeapSampling(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_INTERNAL_FIELD_EXTERNAL(1, profiler, v8::HeapProfiler)

    auto result = profiler->StartSamplingHeapProfiler();
    V8_CHECK(result, "Heap sampling is already active");
}

static void StopHeapSampling(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_INTERNAL_FIELD_EXTERNAL(1, profiler, v8::HeapProfiler)

    profiler->StopSamplingHeapProfiler();
}

static v8::Local<v8::Object> SerializeHeapSampleNode(v8::Isolate* isolate, v8::Local<v8::Context> ctx, v8::AllocationProfile::Node* node, std::vector<v8::AllocationProfile::Sample> samples)
{
    V8_NEW_OBJECT(data);
    data->Set(ctx, V8_NEW_STRING("name"), node->name);
    data->Set(ctx, V8_NEW_STRING("scriptName"), node->script_name);
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
    V8_GET_THIS_INTERNAL_FIELD_EXTERNAL(1, profiler, v8::HeapProfiler)

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

static void Constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_CHECK_CONSTRUCTOR();
    V8_CHECK(alt::ICore::Instance().IsDebug(), "The profiler is only available in debug mode");

    info.This()->SetInternalField(0, v8::External::New(isolate, isolate->GetHeapProfiler())); // Heap Profiler
    //info.This()->SetInternalField(1, v8::External::New(isolate, v8::CpuProfiler::New(isolate))); // CPU Profiler
}

extern V8Class v8Profiler("Profiler", Constructor, [](v8::Local<v8::FunctionTemplate> tpl) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();

    tpl->InstanceTemplate()->SetInternalFieldCount(2);

    V8::SetMethod(isolate, tpl, "startHeapSampling", &StartHeapSampling);
    V8::SetMethod(isolate, tpl, "stopHeapSampling", &StopHeapSampling);
    V8::SetMethod(isolate, tpl, "getHeapSample", &GetHeapSample);
});
