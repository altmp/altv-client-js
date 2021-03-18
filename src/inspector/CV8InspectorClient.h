#pragma once

#include "v8-inspector.h"
#include "../helpers/V8Class.h"
#include "../CV8Resource.h"
#include "ixwebsocket/IXWebSocketServer.h"

class CV8InspectorClient : public v8_inspector::V8InspectorClient
{
public:
    CV8InspectorClient(v8::Local<v8::Context> context);
    ~CV8InspectorClient()
    {
        if(_wsServer != nullptr) 
        {
            _wsServer->stop();
            delete _wsServer;
        }
    }

    static v8::Local<v8::Promise> SendInspectorMessage(v8::Isolate* isolate, alt::String method, v8::Local<v8::Object> params);
    static v8::Local<v8::Promise> SendInspectorMessage(v8::Isolate* isolate, alt::String messageRaw);
    static uint32_t SendInspectorMessage(v8::Isolate* isolate, const std::string& messageRaw);

    v8_inspector::V8InspectorSession* GetSession()
    {
        return _session.get();
    }

    v8::Local<v8::Function> GetCallback(v8::Isolate* isolate)
    {
        if (_callback.IsEmpty()) return v8::Local<v8::Function>();
        return _callback.Get(isolate);
    }

    void SetCallback(v8::Isolate* isolate, v8::Local<v8::Function> callback)
    {
        _callback.Reset(isolate, callback);
    }

    void CreateWebSocketServer(uint32_t port);
    void StartWebSocketServer();
    ix::WebSocketServer* GetWebSocketServer()
    {
        return _wsServer;
    }

    auto GetReceivedMesssages()
    {
        return receivedMessages;
    }

    static void EmplacePendingMessage(uint32_t id, ix::WebSocket* socket)
    {
        pendingMessages.insert({id, socket});
    }

    static std::unordered_map<uint32_t, v8::Global<v8::Promise::Resolver>> promises;
    static std::unordered_map<uint32_t, ix::WebSocket*> pendingMessages;

private:

    v8::Local<v8::Context> ensureDefaultContextInGroup(int group_id) override 
    {
        auto isolate = _isolate;
        //V8_CHECK(isolate, "Invalid isolate");
        //V8_CHECK(group_id == 1, "Invalid context group id");
        return _context.Get(isolate);
    }

    std::unique_ptr<v8_inspector::V8Inspector> _inspector;
    std::unique_ptr<v8_inspector::V8InspectorSession> _session;
    std::unique_ptr<v8_inspector::V8Inspector::Channel> _channel;
    v8::Global<v8::Context> _context;
    v8::Isolate* _isolate;
    v8::Global<v8::Function> _callback;

    ix::WebSocketServer* _wsServer = nullptr;
    std::unordered_map<ix::WebSocket*, std::string> receivedMessages;

    static uint32_t GetNextMessageId();
    static uint32_t lastMessageId;
};