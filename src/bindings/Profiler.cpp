#include "../helpers/V8Helpers.h"
#include "../helpers/V8Class.h"
#include "../CV8ScriptRuntime.h"
#include "v8-profiler.h"
#include <chrono>
#include <vector>
#include <unordered_map>

static void GetHeapStatistics(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT();

	v8::HeapStatistics heapStats;
	isolate->GetHeapStatistics(&heapStats);
	V8_NEW_OBJECT(stats);
	V8_OBJECT_SET_UINT(stats, "heapSizeLimit", heapStats.heap_size_limit());
	V8_OBJECT_SET_UINT(stats, "totalHeapSize", heapStats.total_heap_size());
	V8_OBJECT_SET_UINT(stats, "usedHeapSize", heapStats.used_heap_size());
	V8_OBJECT_SET_UINT(stats, "mallocedMemory", heapStats.malloced_memory());
	V8_OBJECT_SET_UINT(stats, "peakMallocedMemory", heapStats.peak_malloced_memory());

	V8_RETURN(stats);
}

static void StartProfiling(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_CHECK_ARGS_LEN2(0, 1);
    
    v8::Local<v8::String> name;
    if(info.Length() == 1)
    {
        V8_ARG_TO_STRING(1, profileName);
        name = V8_NEW_STRING(profileName.CStr());
    }
    else name = v8::String::Empty(isolate);

    v8::CpuProfilingStatus status = CV8ScriptRuntime::Instance().GetProfiler()->StartProfiling(name, true);
    if(status == v8::CpuProfilingStatus::kStarted) return;
    else if(status == v8::CpuProfilingStatus::kAlreadyStarted) V8Helpers::Throw(isolate, "A profile with the given name is already running");
    else if(status == v8::CpuProfilingStatus::kErrorTooManyProfilers) V8Helpers::Throw(isolate, "There are already too many profilers running");
}

// Key = Node ID, Value = Timestamp
// We store a map of the timestamps here, so we can quickly
// access it when setting it while serializing the profiler node
// todo: There is probably some nicer way to do this
static std::unordered_map<unsigned int, int64_t> nodeMap;
static void GetProfileNodeData(v8::Isolate* isolate, const v8::CpuProfileNode* node, v8::Local<v8::Object> result);

static void StopProfiling(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_CHECK_ARGS_LEN2(0, 1);
    
    v8::Local<v8::String> name;
    if(info.Length() == 1)
    {
        V8_ARG_TO_STRING(1, profileName);
        name = V8_NEW_STRING(profileName.CStr());
    }
    else name = v8::String::Empty(isolate);

    v8::CpuProfile* result = CV8ScriptRuntime::Instance().GetProfiler()->StopProfiling(name);
    V8_CHECK(result, "The specified profiler is not running");

    // Store the node map
    int sampleCount = result->GetSamplesCount();
    for(int i = 0; i < sampleCount; i++)
    {
        unsigned int nodeId = result->GetSample(i)->GetNodeId();
        if(nodeMap.count(nodeId) != 0) continue;
        nodeMap.insert({ nodeId, (result->GetSampleTimestamp(i) / 1000) });
    }

    // Set top level info about the profile
    V8_NEW_OBJECT(resultObj);
    V8_OBJECT_SET_INT(resultObj, "id", std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
    V8_OBJECT_SET_STRING(resultObj, "type", alt::String("cpu"));
    V8_OBJECT_SET_INT(resultObj, "start", result->GetStartTime() / 1000);
    V8_OBJECT_SET_INT(resultObj, "end", result->GetEndTime() / 1000);
    V8_OBJECT_SET_INT(resultObj, "samples", result->GetSamplesCount());

    V8_NEW_OBJECT(root);
    GetProfileNodeData(isolate, result->GetTopDownRoot(), root);
    resultObj->Set(ctx, V8_NEW_STRING("root"), root);

    // Clear the nodemap to not cause a memory leak
    nodeMap.clear();

    V8_RETURN(resultObj);
}

extern V8Class v8Profiler("Profiler", [](v8::Local<v8::FunctionTemplate> tpl) {
    v8::Isolate *isolate = v8::Isolate::GetCurrent();

    V8::SetStaticAccessor(isolate, tpl, "heapStats", GetHeapStatistics);

    V8::SetStaticMethod(isolate, tpl, "startProfiling", StartProfiling);
    V8::SetStaticMethod(isolate, tpl, "stopProfiling", StopProfiling);
});

// *** CPU Profile Serialization

inline static const char* GetSourceTypeName(v8::CpuProfileNode::SourceType type)
{
    switch(type)
    {
        case v8::CpuProfileNode::SourceType::kScript: return "script";
        case v8::CpuProfileNode::SourceType::kBuiltin: return "builtins";
        case v8::CpuProfileNode::SourceType::kCallback: return "native-callback";
        case v8::CpuProfileNode::SourceType::kInternal: return "internal";
    }
    return "unknown";
}

static void GetProfileNodeData(v8::Isolate* isolate, const v8::CpuProfileNode* node, v8::Local<v8::Object> result)
{
    auto ctx = isolate->GetEnteredOrMicrotaskContext();

    // Node info
    result->Set(ctx, V8_NEW_STRING("id"), v8::Integer::NewFromUnsigned(isolate, node->GetNodeId()));

    v8::Local<v8::String> functionName;
    const char* name = node->GetFunctionNameStr();
    if(name == NULL || strlen(name) == 0) functionName = V8_NEW_STRING("(anonymous function)");
    else functionName = V8_NEW_STRING(name);
    result->Set(ctx, V8_NEW_STRING("function"), functionName);

    v8::Local<v8::String> sourceName;
    const char* source = node->GetScriptResourceNameStr();
    if(source == NULL || strlen(source) == 0) sourceName = V8_NEW_STRING("(unknown)");
    else sourceName = V8_NEW_STRING(source);
    result->Set(ctx, V8_NEW_STRING("source"), sourceName);

    result->Set(ctx, V8_NEW_STRING("sourceType"), V8_NEW_STRING(GetSourceTypeName(node->GetSourceType())));
    result->Set(ctx, V8_NEW_STRING("line"), v8::Integer::New(isolate, node->GetLineNumber()));

    v8::Local<v8::Value> bailoutReason;
    const char* reason = node->GetBailoutReason();
    if(reason == NULL || strlen(reason) == 0) bailoutReason = v8::Null(isolate);
    else bailoutReason = V8_NEW_STRING(reason);
    result->Set(ctx, V8_NEW_STRING("bailoutReason"), bailoutReason);

    result->Set(ctx, V8_NEW_STRING("hitCount"), v8::Integer::NewFromUnsigned(isolate, node->GetHitCount()));

    int64_t timestamp;
    if(nodeMap.count(node->GetNodeId()) == 0) timestamp = -1;
    else timestamp = nodeMap.at(node->GetNodeId());
    result->Set(ctx, V8_NEW_STRING("timestamp"), v8::Integer::New(isolate, timestamp));

    // Children
    {
        int childrenCount = node->GetChildrenCount();
        v8::Local<v8::Array> children = v8::Array::New(isolate, childrenCount);
        for(int i = 0; i < childrenCount; i++)
        {
            V8_NEW_OBJECT(child);
            GetProfileNodeData(isolate, node->GetChild(i), child);
            children->Set(ctx, i, child);
        }
        result->Set(ctx, V8_NEW_STRING("children"), children);
    }

    // Line ticks
    {
        std::vector<v8::CpuProfileNode::LineTick> ticks(node->GetHitLineCount());
        v8::Local<v8::Value> val;
        if(node->GetLineTicks(&ticks[0], ticks.size()))
        {
            val = v8::Array::New(isolate, ticks.size());
            for(auto& tick : ticks)
            {
                V8_NEW_OBJECT(tickObj);
                tickObj->Set(ctx, V8_NEW_STRING("line"), v8::Integer::New(isolate, tick.line));
                tickObj->Set(ctx, V8_NEW_STRING("hitCount"), v8::Integer::NewFromUnsigned(isolate, tick.hit_count));
                val.As<v8::Array>()->Set(ctx, val.As<v8::Array>()->Length(), tickObj);
            }
        }
        else val = v8::Null(isolate);
        result->Set(ctx, V8_NEW_STRING("lineTicks"), val);
    }
}
