
#include "../helpers/V8Helpers.h"
#include "../helpers/V8Class.h"
#include "../helpers/V8Entity.h"
#include "../helpers/V8ResourceImpl.h"

#include "../CV8ScriptRuntime.h"

#include "cpp-sdk/objects/IPlayer.h"
#include "cpp-sdk/objects/IVehicle.h"

using namespace alt;

static void ToString(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	V8_GET_ISOLATE_CONTEXT();

    auto player = info.This();
    V8_OBJECT_GET_INTEGER(player, "id", id);
	V8_OBJECT_GET_STRING(player, "name", name);

	std::ostringstream ss;
	ss << "Player{ id: " << std::to_string(id) << ", name: " << name.CStr() << " }";

	V8_RETURN_STRING(ss.str().c_str());
}

static void NameGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_STRING(player->GetName().CStr());
}

static void VehicleGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT_RESOURCE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_BASE_OBJECT(player->GetVehicle());
}

static void TalkingGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_BOOLEAN(player->IsTalking());
}

static void MicLevelGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_NUMBER(player->GetMicLevel());
}

static void CurrentWeaponComponentsGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    alt::Array<uint32_t> comps = player->GetCurrentWeaponComponents();

    v8::Local<v8::Array> componentsArray = v8::Array::New(isolate, comps.GetSize());

    for (uint32_t i = 0; i < comps.GetSize(); ++i)
        componentsArray->Set(ctx, i, v8::Integer::NewFromUnsigned(isolate, comps[i]));

    V8_RETURN(componentsArray);
}

static void CurrentWeaponTintIndexGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_INT(player->GetCurrentWeaponTintIndex());
}

static void CurrentWeaponGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_UINT(player->GetCurrentWeapon());
}

static void IsJumpingGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_BOOLEAN(player->IsJumping());
}

static void IsInRagdollGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_BOOLEAN(player->IsInRagdoll());
}

static void IsAimingGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_BOOLEAN(player->IsAiming());
}

static void IsShootingGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_BOOLEAN(player->IsShooting());
}

static void IsReloadingGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_BOOLEAN(player->IsReloading());
}

static void ArmourGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_INT(player->GetArmour());
}

static void MaxArmourGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_INT(player->GetMaxArmour());
}

static void MoveSpeedGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_NUMBER(player->GetMoveSpeed());
}

static void AimPosGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT_RESOURCE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN(resource->CreateVector3(player->GetAimPos()));
}

static void HeadRotationGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT_RESOURCE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN(resource->CreateVector3(player->GetHeadRotation()));
}

static void SeatGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_INT(player->GetSeat());
}

static void EntityAimingAtGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT_RESOURCE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_BASE_OBJECT(player->GetEntityAimingAt());
}

static void EntityAimOffsetGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT_RESOURCE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN(resource->CreateVector3(player->GetEntityAimOffset()));
}

static void FlashlightActiveGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_BOOLEAN(player->IsFlashlightActive());
}

static void HealthGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_INT(player->GetHealth());
}

static void MaxHealthGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_INT(player->GetMaxHealth());
}

static void IsDeadGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_BOOLEAN(player->IsDead());
}

static void WeaponHasComponent(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_CHECK_ARGS_LEN(2);
    V8_ARG_TO_INT(1, weaponHash);
    V8_ARG_TO_INT(2, componentHash);

    V8_RETURN_BOOLEAN(player->HasWeaponComponent(weaponHash, componentHash));
}

static void GetWeaponTintIndex(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_CHECK_ARGS_LEN(1);
    V8_ARG_TO_INT(1, weaponHash);

    V8_RETURN_INT(player->GetWeaponTintIndex(weaponHash));
}

static void GetCurrentWeapon(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE();
    V8_GET_THIS_BASE_OBJECT(player, alt::IPlayer);

    V8_RETURN_INT(player->GetCurrentWeapon());
}

static void SpatialVolumeGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_BASE_OBJECT(_this, alt::IPlayer);

    V8_RETURN_NUMBER(_this->GetSpatialVolume());
}

static void SpatialVolumeSetter(v8::Local<v8::String>, v8::Local<v8::Value> val, const v8::PropertyCallbackInfo<void>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_BASE_OBJECT(_this, alt::IPlayer);

    V8_TO_NUMBER(val, volume);

    _this->SetSpatialVolume(volume);
}

static void NonSpatialVolumeGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_BASE_OBJECT(_this, alt::IPlayer);

    V8_RETURN_NUMBER(_this->GetNonSpatialVolume());
}

static void NonSpatialVolumeSetter(v8::Local<v8::String>, v8::Local<v8::Value> val, const v8::PropertyCallbackInfo<void>& info)
{
    V8_GET_ISOLATE_CONTEXT();
    V8_GET_THIS_BASE_OBJECT(_this, alt::IPlayer);

    V8_TO_NUMBER(val, volume);

    _this->SetNonSpatialVolume(volume);
}

static void AllGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT_RESOURCE();

    V8_RETURN(resource->GetAllPlayers()->Clone());
}

static void StreamedInGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT_RESOURCE();

    auto streamedIn = CV8ScriptRuntime::Instance().GetStreamedInPlayers();
    auto arr = v8::Array::New(isolate, streamedIn.size());
    int i = 0;
    for(auto kv : streamedIn)
    {
        arr->Set(ctx, i, resource->GetOrCreateEntity(kv.second.Get(), "Player")->GetJSVal(isolate));
        i++;
    }

    V8_RETURN(arr);
}

static void LocalGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT_RESOURCE();

    V8_RETURN_BASE_OBJECT(alt::ICore::Instance().GetLocalPlayer());
}

static void StaticGetByScriptID(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT_RESOURCE();
    V8_CHECK_ARGS_LEN(1);
    V8_ARG_TO_INT(1, scriptGuid);
    V8_RETURN_BASE_OBJECT(alt::ICore::Instance().GetEntityByScriptGuid(scriptGuid).As<alt::IPlayer>());
}

static void StaticGetByID(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8_GET_ISOLATE_CONTEXT_RESOURCE();
    V8_CHECK_ARGS_LEN(1);
    V8_ARG_TO_INT(1, id);
    V8_RETURN_BASE_OBJECT(alt::ICore::Instance().GetEntityByID(id).As<alt::IPlayer>());
}

extern V8Class v8Entity;
extern V8Class v8Player("Player", v8Entity, [](v8::Local<v8::FunctionTemplate> tpl) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();

    V8::SetMethod(isolate, tpl, "toString", ToString);

    V8::SetStaticMethod(isolate, tpl, "getByID", StaticGetByID);
    V8::SetStaticMethod(isolate, tpl, "getByScriptID", StaticGetByScriptID);

    V8::SetStaticAccessor(isolate, tpl, "all", &AllGetter);
    V8::SetStaticAccessor(isolate, tpl, "streamedIn", &StreamedInGetter);
    V8::SetStaticAccessor(isolate, tpl, "local", &LocalGetter);

    // Common getters
    V8::SetAccessor(isolate, tpl, "name", &NameGetter);
    V8::SetAccessor(isolate, tpl, "vehicle", &VehicleGetter);
    V8::SetAccessor(isolate, tpl, "seat", &SeatGetter);
    V8::SetAccessor(isolate, tpl, "isTalking", &TalkingGetter);
    V8::SetAccessor(isolate, tpl, "micLevel", &MicLevelGetter);
    V8::SetAccessor(isolate, tpl, "health", &HealthGetter);
    V8::SetAccessor(isolate, tpl, "maxHealth", &MaxHealthGetter);
    V8::SetAccessor(isolate, tpl, "armour", &ArmourGetter);
    V8::SetAccessor(isolate, tpl, "maxArmour", &MaxArmourGetter);
    V8::SetAccessor(isolate, tpl, "spatialVolume", &SpatialVolumeGetter, &SpatialVolumeSetter);
    V8::SetAccessor(isolate, tpl, "nonSpatialVolume", &NonSpatialVolumeGetter, &NonSpatialVolumeSetter);

    // Weapon getters
    V8::SetAccessor(isolate, tpl, "currentWeaponComponents", &CurrentWeaponComponentsGetter);
    //V8::SetAccessor(isolate, tpl, "currentWeaponTintIndex", &CurrentWeaponTintIndexGetter);
    V8::SetAccessor(isolate, tpl, "currentWeapon", &CurrentWeaponGetter);
    V8::SetAccessor(isolate, tpl, "entityAimingAt", &EntityAimingAtGetter);
    V8::SetAccessor(isolate, tpl, "entityAimOffset", &EntityAimOffsetGetter);
    V8::SetAccessor(isolate, tpl, "flashlightActive", &FlashlightActiveGetter);
    V8::SetAccessor(isolate, tpl, "aimPos", &AimPosGetter);

    // Gamestate getters
    //V8::SetAccessor(isolate, tpl, "isJumping", &IsJumpingGetter);
    V8::SetAccessor(isolate, tpl, "isInRagdoll", &IsInRagdollGetter);
    V8::SetAccessor(isolate, tpl, "isAiming", &IsAimingGetter);
    //V8::SetAccessor(isolate, tpl, "isShooting", &IsShootingGetter);
    //V8::SetAccessor(isolate, tpl, "isReloading", &IsReloadingGetter);
    V8::SetAccessor(isolate, tpl, "isDead", &IsDeadGetter);
    V8::SetAccessor(isolate, tpl, "moveSpeed", &MoveSpeedGetter);
    V8::SetAccessor(isolate, tpl, "headRot", &HeadRotationGetter);
});
