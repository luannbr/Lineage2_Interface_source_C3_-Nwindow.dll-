/*=============================================================================
    UE2Engine.h - the part of Engine.dll (Lineage II C2 build) that NWindow uses.
    Same rules as UE2Core.h: ENGINE_API declarations must match Engine.dll's
    decorated exports, virtual lists follow the vtables in Engine.dll.

    C2 vs the C1 build this file started from:
      * every class derived from UObject has one more virtual slot (UObject::ClearL2Game);
      * the vtables (include/ue2/gen, tools/gen_engine_decls.py) were regenerated from the C2
        Engine.dll; UCanvas, ULevel and UL2Font (hand written here) were redone from it;
      * the field offsets that the script layout proves (AActor, APawn, AController, UEngine,
        UGameEngine, UCanvas: tools/ue2_check_headers.py) are the C2 ones; the C1 names that
        carry an offset (Unknown5C...) kept their name. User, PetInfo, FL2GameData (and the
        records it keeps: system messages, castles, skills) were re-derived from the C2 code
        (the evidence is in their comments). Fields nobody could prove (ItemInfo, PledgeInfo,
        UL2ConsoleWnd's tail...) are still the C1 layout: see the "C2 RISK" comments.
=============================================================================*/
#pragma once

#include "UE2Core.h"

class UMaterial;
class UTexture;
class UFont;
class UViewport;
class UClient;
class URenderDevice;
class AActor;
class FLevelSceneNode;
class FRenderInterface;
class FL2ColorFontInfo;
class UL2Font;
struct User;
class ULevel;
class APawn;
class ABrush;
class ALevelInfo;
class APlayerController;
class UNetConnection;
class UChannel;
class FURL;
class FMemStack;
struct FCheckResult;
class AController;
class UCanvas;
class FL2ReplayManager;
struct FL2QuestData;
struct FL2MagicSkillData;
enum AppendixVisualEffectBitType : INT;   // C3: APawn::AddAppendixEffect (the values are not read yet)

// the types the generated virtual lists name (include/ue2/gen/*.vt.inc)
#include "gen/EngineFwd.gen.h"

/*-----------------------------------------------------------------------------
    Math (UnMath.h).
-----------------------------------------------------------------------------*/

class FVector
{
public:
    FLOAT X, Y, Z;
    FVector() {}
    FVector(FLOAT InX, FLOAT InY, FLOAT InZ) : X(InX), Y(InY), Z(InZ) {}
    CORE_API FLOAT Size() const;
    // the two Core calls of the camera shake (EngineViewShake.cpp): ?SafeNormal@FVector@@QBE?AV1@XZ (a __thiscall that returns the 12-byte vector through a hidden pointer:
    // the pointer is the first stack argument, `ret 4`, `this` in ecx) and ?GetMax@FVector@@QBEMXZ (the largest component, in ST0). Core.dll's own x87 code: not rewritten
    CORE_API FVector SafeNormal() const;
    CORE_API FLOAT GetMax() const;
    // the Core calls of the polygon code (EngineGeometry.cpp), each of them Core.dll's own x87 code, never rewritten; the decorated name is what the import library resolves
    CORE_API FLOAT SizeSquared() const;                          // ?SizeSquared@FVector@@QBEMXZ: ((x*x + y*y) + z*z), left UNROUNDED in st0 (Core 0x1010C9C0)
    CORE_API INT IsZero() const;                                 // ?IsZero@FVector@@QBEHXZ: the three components == 0.0f (test ah,0x44; jp)
    CORE_API INT Normalize();                                    // ?Normalize@FVector@@QAEHXZ: in place; 0 (and nothing written) when the length is too small
    CORE_API FVector TransformVectorBy(const FCoords& Coords) const;   // ?TransformVectorBy@FVector@@QBE?AV1@ABVFCoords@@@Z: FCoords MUST be a `class` (the 'V' of the name)
    CORE_API INT operator==(const FVector& Other) const;         // ??8FVector@@QBEHABV0@@Z: IEEE, per component (a NaN is different)
    CORE_API INT operator!=(const FVector& Other) const;         // ??9FVector@@QBEHABV0@@Z
};

class FPlane : public FVector
{
public:
    FLOAT W;
    CORE_API FPlane operator*(FLOAT Scale) const;                // ??DFPlane@@QBE?AV0@M@Z: the four components times Scale (a hidden result pointer)
};

// a sphere is a plane's four floats (X, Y, Z and the radius in W): passed BY VALUE to FConvexVolume::SphereCheck
class FSphere : public FPlane
{
};
NW_ASSERT_SIZE(FSphere, 16);

// UE2's FCoords (UnMath.h): an origin and three axes, 0x30 bytes. A `class` in the Engine (the mangled name of FVector::TransformVectorBy says 'V')
class FCoords
{
public:
    FVector Origin, XAxis, YAxis, ZAxis;
};
NW_ASSERT_SIZE(FCoords, 0x30);

// what UModel::Transform builds for FPoly::Transform (Core's ??0FModelCoords@@QAE@XZ + ABrush::BuildCoords): the point transform at +0 and the vector transform at +0x30 are the only
// parts FPoly::Transform reads (VERIFIED in the assembly); the real object may be larger
struct FModelCoords
{
    FCoords PointXform, VectorXform;
};
NW_ASSERT_SIZE(FModelCoords, 0x60);

class FRotator
{
public:
    INT Pitch, Yaw, Roll;
    FRotator() {}
    FRotator(INT InPitch, INT InYaw, INT InRoll) : Pitch(InPitch), Yaw(InYaw), Roll(InRoll) {}
};
NW_ASSERT_SIZE(FRotator, 12);

// A collision check's result (UnPrim.h, FIteratorActorList's two pointers first; the NPC
// viewer traces the ground with one).
struct FCheckResult
{
    FCheckResult* Next;
    AActor* Actor;
    FVector Location;
    FVector Normal;
    UObject* Primitive;
    FLOAT Time;
    INT Item;
    UObject* Material;

    FCheckResult(FLOAT InTime = 0.f, FCheckResult* InNext = NULL)
        : Next(InNext), Actor(NULL), Location(0, 0, 0), Normal(0, 0, 0), Primitive(NULL),
          Time(InTime), Item(-1), Material(NULL) {}
};
NW_ASSERT_SIZE(FCheckResult, 0x30);

/*-----------------------------------------------------------------------------
    L2 text rendering enums (values still being mapped).
-----------------------------------------------------------------------------*/

enum L2FontType { L2FONT_Normal = 0 };
enum TargetRenderType { TARGETRENDER_Default = 0 };

/*-----------------------------------------------------------------------------
    UTexture. Real chain: UTexture : UBitmapMaterial : URenderedMaterial :
    UMaterial : UObject. Only the fields NWindow reads are placed, the rest is
    padding until decompiled code needs it.
-----------------------------------------------------------------------------*/

// UE2's texture formats (UnTex.h)
enum ETextureFormat
{
    TEXF_P8, TEXF_RGBA7, TEXF_RGB16, TEXF_DXT1, TEXF_RGB8, TEXF_RGBA8, TEXF_NODATA, TEXF_DXT3,
    TEXF_DXT5, TEXF_L8, TEXF_G16, TEXF_RRRGGGBBB,
};

class UTexture : public UObject
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }
    // mip level Miplevel decoded to 32 bpp BGRA (Data from GMalloc, freed by the caller)
    ENGINE_API ETextureFormat ConvertDXT(INT Miplevel, INT ForceRGBA, INT Use4444, BYTE** Data);

    BYTE _Unknown28[0x38 - 0x28];
    BYTE Format;  // +0x38 ETextureFormat (UBitmapMaterial)
    BYTE _Unknown39[0x40 - 0x39];
    INT USize;   // +0x40 (tentative name: NWindow skips textures where both are 0)
    INT VSize;   // +0x44
    BYTE _Unknown48[0x80 - 0x48];
    DWORD TextureFlags;  // +0x80 (name ours) bits: the cool time frames get bit 1
    BYTE _Unknown84[0xA4 - 0x84];
    FLOAT UnknownA4;     // +0xA4 the shortcut bar sets 9 on its toggle skill frame before each
                         //   draw (an animation rate?)

private:
    ENGINE_API static UClass PrivateStaticClass;
};
NW_ASSERT_OFFSET(UTexture, Format, 0x38);
NW_ASSERT_OFFSET(UTexture, USize, 0x40);
NW_ASSERT_OFFSET(UTexture, VSize, 0x44);
NW_ASSERT_OFFSET(UTexture, TextureFlags, 0x80);
NW_ASSERT_OFFSET(UTexture, UnknownA4, 0xA4);

// Classes NWindow only names (the pawn viewer finds and loads hair meshes and their skins)
class UMaterial : public UObject
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

private:
    ENGINE_API static UClass PrivateStaticClass;
};

// C2: named by the C2 NWindow (script class FinalBlend extends Modifier, a material)
class UFinalBlend : public UMaterial
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

private:
    ENGINE_API static UClass PrivateStaticClass;
};

// the engine's list of temporary objects the L2 client keeps (C2: the NWindow imports the pointer)
class UGlobalTempObjects;
ENGINE_API extern UGlobalTempObjects* GGlobalTempObjects;

// UE2's bounding box (UnMath.h): what UPrimitive::GetRenderBoundingBox returns
class FBox
{
public:
    FVector Min;
    FVector Max;
    BYTE IsValid;
};
NW_ASSERT_SIZE(FBox, 0x1C);

// the base of the meshes (UE2's; its fields are not declared)
class UPrimitive : public UObject
{
public:
#include "gen/UPrimitive.vt.inc"
};

class UMesh : public UPrimitive
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

#include "gen/UMesh.vt.inc"

private:
    ENGINE_API static UClass PrivateStaticClass;
};

class UMeshAnimation;

class USkeletalMesh : public UMesh
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

    BYTE _Unknown28[0x23C - 0x28];
    UMeshAnimation* DefaultAnim;    // +0x23C (UE2's) its animation set: the constructor clears it,
                                    //   Serialize keeps it, USkeletalMeshInstance plays from it

private:
    ENGINE_API static UClass PrivateStaticClass;
};
NW_ASSERT_OFFSET(USkeletalMesh, DefaultAnim, 0x23C);

// One sequence of an animation set (UE2's FMeshAnimSeq, 0x40 bytes here: UMeshAnimation::
// GetAnimSeq steps by it), the name first
struct FMeshAnimSeq
{
    FName Name;                     // +0x00
    BYTE _Unknown4[0x40 - 0x4];
};
NW_ASSERT_SIZE(FMeshAnimSeq, 0x40);

// UE2's animation set of a skeletal mesh (its copy constructor copies UT2003's InternalVersion,
// RefBones, Moves and AnimSeqs, in that order); only what NWindow reads
class UMeshAnimation : public UObject
{
public:
    BYTE _Unknown28[0x44 - 0x28];
    TArray<FMeshAnimSeq> AnimSeqs;  // +0x44 (UE2's)
};
NW_ASSERT_OFFSET(UMeshAnimation, AnimSeqs, 0x44);

// One animation channel of a skeletal mesh instance (UE2's MeshAnimChannel, 0x70 bytes here)
struct MeshAnimChannel
{
    BYTE _Unknown0[0xC];
    FLOAT AnimRate;                 // +0x0C (USkeletalMeshInstance::GetActiveAnimRate)
    BYTE _Unknown10[0x20 - 0x10];
    FLOAT Unknown20;                // +0x20 SetAnimRate makes AnimRate the rate times it
    BYTE _Unknown24[0x70 - 0x24];
};
NW_ASSERT_SIZE(MeshAnimChannel, 0x70);

// The instance of a skeletal mesh an actor plays (AActor::MeshInstance); only its channels
// C2 RISK: 0x330 bytes in C2 (C1: 0x3B4) and 84 vtable slots (C1: 81, first difference at slot 10):
// the class was reorganized (USubSkeletalMeshInstance is new), Blends below is the C1 offset
class USkeletalMeshInstance : public UObject
{
public:
    BYTE _Unknown28[0x114 - 0x28];
    TArray<MeshAnimChannel> Blends; // +0x114 (UE2's name) its channels (GetAnimChannelCount,
                                    //   SetAnimRate)
};
NW_ASSERT_OFFSET(USkeletalMeshInstance, Blends, 0x114);

/*-----------------------------------------------------------------------------
    Actors, players and the level: only the members NWindow touches. Field
    offsets come from Engine.dll code (noted per field), virtual lists from its
    vtables (tools/engine_vtables.py). Never embed or allocate these.
-----------------------------------------------------------------------------*/

// enum values from UE2 (UT2003 headers), not checked against L2 yet
enum ELevelTick { LEVELTICK_TimeOnly = 0, LEVELTICK_ViewportsOnly = 1, LEVELTICK_All = 2 };
enum ENetRole { ROLE_None, ROLE_DumbProxy, ROLE_SimulatedProxy, ROLE_AutonomousProxy, ROLE_Authority };
enum EDetailMode { DM_Low, DM_High, DM_SuperHigh };
enum ESoundOcclusion { OCCLUSION_Default, OCCLUSION_None, OCCLUSION_BSP, OCCLUSION_StaticMeshes };
enum EAcceptConnection { ACCEPTC_Reject, ACCEPTC_Accept, ACCEPTC_Ignore };

// C2 layout of AActor / APawn / AController: the offsets are the C2 script layout (Engine.u: the order
// of the `var` declarations is the layout, tools/ue2_layouts.py, checked against the class sizes
// 0x394 / 0x14F4 / 0x534 of the IMPLEMENT_CLASS initializers, tools/ue2_check_headers.py). A name
// like Unknown5C keeps the C1 offset it was given; its comment has the C2 one and the script
// variable. Several fields moved from AActor to APawn or the other way (the L2 additions of the
// pawn), so a field may now be declared in another class than in the C1 header.
class AActor : public UObject
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

#include "gen/AActor.vt.inc"

    BYTE _Unknown28[0x40 - 0x28];
    INT Unknown5C;      // +0x40 (C1 +0x5C, script RenderRevision) bumped when the chat balloon over a pawn
                        //   changes (NCConsole)
    BYTE _Unknown44[0x54 - 0x44];
    INT CreatureID;     // +0x54 (C1 +0x94) server id (ULevel::FindActorWithCreatureID)
    DWORD Unknown98;    // +0x58 (C1 +0x98, the bool word of NoCheatCollision, bDeleteNow...: the same bits)
                        //   bits: the pawn simulator sets bit 2 on a pawn's controller before destroying the pawn
    BYTE _Unknown5C[0xB6 - 0x5C];
    BYTE Role;          // +0xB6 (C1 +0xED) ENetRole: the viewers make what they spawn ROLE_Authority
    BYTE _UnknownB7[0xD8 - 0xB7];
    ULevel* XLevel;     // +0xD8 (C1 +0x118) (AActor::GetLevel, exported inline)
    BYTE _UnknownDC[0xF8 - 0xDC];
    UMesh* Mesh;        // +0xF8 (C1 +0x138) (the viewers list its animations)
    BYTE _UnknownFC[0x128 - 0xFC];
    UObject* MeshInstance;      // +0x128 (C1 +0x19C) the mesh's instance (a USkeletalMeshInstance for the
                                 //   viewers' pawns: the NPC viewer rescales its first channel)
    BYTE _Unknown12C[0x1B0 - 0x12C];
    FVector Location;   // +0x1B0 (C1 +0x2C8) (NWindow: /loc prints the player's)
    FRotator Rotation;  // +0x1BC (C1 +0x2D4) (NCPawnSelectWnd turns the characters to the camera)
    BYTE _Unknown1C8[0x2E4 - 0x1C8];
    FLOAT CollisionRadius; // +0x2E4 (C1 +0x4B0) (the NPC viewer's summon traces with it)
    FLOAT CollisionHeight; // +0x2E8 (C1 +0x4B4) (NCLobbyWnd: the name floats above it)
    BYTE _Unknown2EC[0x378 - 0x2EC];
    BYTE Unknown5C0;    // +0x378 (C1: APawn +0x5C0; script NAttackStatus, an AActor variable in C2) the
                        //   simulator's attacks set 1
    BYTE _Unknown379[0x398 - 0x379];

    ULevel* GetLevel() const { return XLevel; }

private:
    ENGINE_API static UClass PrivateStaticClass;
};
NW_ASSERT_OFFSET(AActor, Unknown5C, 0x40);
NW_ASSERT_OFFSET(AActor, CreatureID, 0x54);
NW_ASSERT_OFFSET(AActor, Unknown98, 0x58);
NW_ASSERT_OFFSET(AActor, Role, 0xB6);
NW_ASSERT_OFFSET(AActor, XLevel, 0xD8);
NW_ASSERT_OFFSET(AActor, Mesh, 0xF8);
NW_ASSERT_OFFSET(AActor, MeshInstance, 0x128);
NW_ASSERT_OFFSET(AActor, Location, 0x1B0);
NW_ASSERT_OFFSET(AActor, Rotation, 0x1BC);
NW_ASSERT_OFFSET(AActor, CollisionRadius, 0x2E4);
NW_ASSERT_OFFSET(AActor, CollisionHeight, 0x2E8);
NW_ASSERT_OFFSET(AActor, Unknown5C0, 0x378);
NW_ASSERT_SIZE(AActor, 0x398);

class APawn : public AActor
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

#include "gen/APawn.vt.inc"

    BYTE _Unknown398[0x3A8 - 0x398];
    UMesh* HairMeshes[2];        // +0x3A8 (C2 +0x3A4; name ours; C1: AActor +0x14C, SubMeshes[4..5] of the script, an APawn
                                 //   variable in C2) the hair change window's second mesh, then its first
    BYTE _Unknown3B0[0x438 - 0x3B0];
    UMaterial* HairMaterials[2]; // +0x438 (C2 +0x434; name ours; C1: AActor +0x418, SubSkins[4..5]) the materials of HairMeshes
    BYTE _Unknown440[0x4E4 - 0x440];
    INT Unknown578;                 // +0x4E4 (C2 +0x4D8; C1 +0x578, script MagicInfo.MagicType) the combined animations set 9
                                    //   after SetMagicInfo,
    BYTE _Unknown4E8[0x4EC - 0x4E8];
    INT Unknown580;                 // +0x4EC (C2 +0x4E0; C1 +0x580, MagicInfo.nSlice)   and 3, their number of steps (MixAnimSteps)
    BYTE _Unknown4F0[0x508 - 0x4F0];
    TArray<AActor*> MagicTargets;   // +0x508 (C2 +0x4FC; C1 +0x59C, MagicInfo.AssociatedActor; name ours) the targets of the skill
                                    //   SetMagicInfo sets up (the viewers add theirs before InitSkillProcess)
    BYTE _Unknown514[0x554 - 0x514];
    INT ActionType;                 // +0x554 (C2 +0x520; C1 +0x624, script NPrimeAction; names ours) L2's action: 5 a skill,
                                    //   1 an attack, 2 a bow attack, 0 none (the viewers and the simulator)
    INT Unknown628;                 // +0x558 (C2 +0x524; NPrimeAction.Damage) the attacks set 1
    DWORD Unknown62C;               // +0x55C (C2 +0x528; NPrimeAction.bMiss) bits 1..3: which attack
    BYTE _Unknown560[0x564 - 0x560];
    AActor* ActionTarget;           // +0x564 (C2 +0x52C)
    INT Unknown634;                 // +0x568 (C2 +0x530; NPrimeAction.MaxAtkShotNum) the attacks clear both
    INT Unknown638;                 // +0x56C (C2 +0x534; NPrimeAction.CurAtkShotNum)
    FArray Unknown63C;              // +0x570 (C2 +0x538; NPrimeAction.NAssociatedAttackParam) 12-byte elements, emptied by the attacks
    BYTE _Unknown57C[0x59C - 0x57C];
    DWORD AnimFlags;                // +0x59C (C2 +0x558; C1 +0x65C, the bool word of bPhysicInit...; name ours) bit 4: an animation the
                                    //   viewers chose plays instead of the idle one
    BYTE _Unknown5A0[0x5B8 - 0x5A0];
    BYTE Unknown678;                // +0x5B8 (C2 +0x574; C1 +0x678, bFaceRotation...) bit 2: the hair cannot change (hair change window)
    BYTE _Unknown5B9[0x5BC - 0x5B9];
    INT MeshType;                   // +0x5BC (C2 +0x578; C1 +0x67C, script CharClassID; names ours) its race and sex
                                    //   (User::GetMeshType): the prefix of its hair meshes' names
    INT NpcID;                      // +0x5C0 (C2 +0x57C; C1 +0x680) (the NPC viewer's pawn)
    INT LeftHandItem;               // +0x5C4 (C2 +0x580; C1 +0x684) the item class id in its left hand (NPC viewer)
    BYTE _Unknown5C8[0x5D0 - 0x5C8];
    DWORD Unknown690;               // +0x5D0 (C2 +0x58C; C1 +0x690, bGetOnVehicle) bit 0: nothing can be dropped where it stands
    FLOAT Unknown694;               // +0x5D4 (C2 +0x590; C1 +0x694) the rate the pawn viewer plays a chosen animation at
    FLOAT SpeedRate;                // +0x5D8 (C2 +0x594; C1 +0x698; names ours) the factor of its speeds (the status
                                    //   window's RunSpeed and WalkSpeed; the NPC viewer's rate)
    BYTE _Unknown5DC[0x610 - 0x5DC];
    FLOAT Speed;                    // +0x610 (C2 +0x5CC; C1 +0x6D0) (the NPC viewer's speed line)
    BYTE _Unknown614[0x641 - 0x614];
    BYTE WeaponType;                // +0x641 (C2 +0x5FD; C1 +0x701; names ours) the weapon in hand, 0..7 (5: a bow):
                                    //   the index into its FName[8] animation tables (script WaitAnimName
                                    //   at +0x720 every 0x20 bytes, Engine.dll)
    BYTE _Unknown642[0x764 - 0x642];
    FName WaitAnimName[8];          // +0x764 (C2 +0x720; C1 +0x7EC) by weapon type (APawn::GetWaitAnimName)
    BYTE _Unknown784[0xEE4 - 0x784];
    FName SocialAnimName[20];       // +0xEE4 (C2 +0xD40; C1 +0xDCC; script PcSocialAnimName; name ours) by social action, as
                                    //   UGameEngine::OnSocialAction plays them; the selection
                                    //   plays the third on a second click
    BYTE _UnknownF34[0x1374 - 0xF34];
    AController* Controller;        // +0x1374 (C2 +0x11D0; C1 +0x1198)
    BYTE _Unknown1378[0x14EC - 0x1378];
    TArray<FName> MixAnimSteps;     // +0x14EC (C2 +0x1348; C1 +0x1310; script MixedAnims; name ours) the combined animation's, played from
                                    //   the last
    BYTE _Unknown14F8[0x15AC - 0x14F8];
    DWORD Unknown13D4;              // +0x15AC (C2 +0x1410; C1 +0x13D4, bPhysicsAnimUpdate...) bit 0: set with the idle animation, cleared while
                                    //   one the viewers chose plays
    BYTE _Unknown15B0[0x1604 - 0x15B0];
    TCHAR* ChatBalloon;             // +0x1604 (C2 +0x1468; C1 +0x142C; script ChatMsg; name ours) the chat line drawn over it (NCConsole)
    BYTE _Unknown1608[0x1650 - 0x1608];
    DWORD SelectFlags;              // +0x1650 (C2 +0x14AC; C1 +0x1458; names ours) bit 0: one of the selection's
    INT SelectIndex;                // +0x1654 (C2 +0x14B0; C1 +0x145C)   characters, and its slot there (both set by
                                    //   UL2ConsoleWnd::InsertSelectCharacterInfo)
    BYTE _Unknown1658[0x16CC - 0x1658];

    // exported by Engine.dll too; inline in the headers NWindow was built with (the stock code
    // reads the table)
    FORCEINLINE FName GetWaitAnimName() { return WaitAnimName[WeaponType]; }

    // L2's skill use: the skill viewer's cast (NCMSView_InvenItemWnd::OnRButtonDown), the NPC
    // viewer's summon (NCNpcView_CharacterSelectWnd::OnSummon), the pawn viewer's combined
    // animations and simulator (which also cancels and stops); argument names ours
    // C3: returns INT and takes the skill's data record as a last argument (?SetMagicInfo@APawn@@QAEHHHMMPAVAActor@@MPAUFL2MagicSkillData@@@Z;
    // C2: void, six arguments)
    ENGINE_API INT SetMagicInfo(INT SkillID, INT Level, FLOAT HitTime, FLOAT HitTime2,
                                AActor* Target, FLOAT P6, FL2MagicSkillData* SkillData);
    ENGINE_API INT InitSkillProcess();
    ENGINE_API void SpawnEnterEvent();
    ENGINE_API void MagicCancel();
    ENGINE_API void NActionStop();
    // new in C2 (imported by the C2 NWindow; argument names ours)
    ENGINE_API FName GetCurWaitAnimName();                 // ?GetCurWaitAnimName@APawn@@QAE?AVFName@@XZ
    ENGINE_API FLOAT GetSocialAnimDuration(INT Index);     // ?GetSocialAnimDuration@APawn@@QAEMH@Z: seconds of social action Index
    ENGINE_API INT SpawnNTransientEffect(APawn* P1, INT P2);  // ?SpawnNTransientEffect@APawn@@QAEHPAV1@H@Z
    // new in C3 (imported by the C3 NWindow; argument names ours)
    ENGINE_API void AddAppendixEffect(AppendixVisualEffectBitType Effect);       // the extra visual effects of a pawn
    ENGINE_API void DeleteAppendixEffect(AppendixVisualEffectBitType Effect);
    ENGINE_API void DeleteAllAppendixEffect();
    ENGINE_API FMeshAnimSeq* FindSeq(FName AnimName);                            // ?FindSeq@APawn@@QAEPAUFMeshAnimSeq@@VFName@@@Z
    ENGINE_API INT Action_Attack(AActor* Target, INT P2, INT P3, INT P4, INT P5, INT P6, INT P7, INT P8);

private:
    ENGINE_API static UClass PrivateStaticClass;
};
NW_ASSERT_OFFSET(APawn, HairMeshes, 0x3A8);
NW_ASSERT_OFFSET(APawn, HairMaterials, 0x438);
NW_ASSERT_OFFSET(APawn, Unknown578, 0x4E4);
NW_ASSERT_OFFSET(APawn, Unknown580, 0x4EC);
NW_ASSERT_OFFSET(APawn, MagicTargets, 0x508);
NW_ASSERT_OFFSET(APawn, ActionType, 0x554);
NW_ASSERT_OFFSET(APawn, Unknown628, 0x558);
NW_ASSERT_OFFSET(APawn, Unknown62C, 0x55C);
NW_ASSERT_OFFSET(APawn, ActionTarget, 0x564);
NW_ASSERT_OFFSET(APawn, Unknown634, 0x568);
NW_ASSERT_OFFSET(APawn, Unknown638, 0x56C);
NW_ASSERT_OFFSET(APawn, Unknown63C, 0x570);
NW_ASSERT_OFFSET(APawn, AnimFlags, 0x59C);
NW_ASSERT_OFFSET(APawn, Unknown678, 0x5B8);
NW_ASSERT_OFFSET(APawn, MeshType, 0x5BC);
NW_ASSERT_OFFSET(APawn, NpcID, 0x5C0);
NW_ASSERT_OFFSET(APawn, LeftHandItem, 0x5C4);
NW_ASSERT_OFFSET(APawn, Unknown690, 0x5D0);
NW_ASSERT_OFFSET(APawn, Unknown694, 0x5D4);
NW_ASSERT_OFFSET(APawn, SpeedRate, 0x5D8);
NW_ASSERT_OFFSET(APawn, Speed, 0x610);
NW_ASSERT_OFFSET(APawn, WeaponType, 0x641);
NW_ASSERT_OFFSET(APawn, WaitAnimName, 0x764);
NW_ASSERT_OFFSET(APawn, SocialAnimName, 0xEE4);
NW_ASSERT_OFFSET(APawn, Controller, 0x1374);
NW_ASSERT_OFFSET(APawn, MixAnimSteps, 0x14EC);
NW_ASSERT_OFFSET(APawn, Unknown13D4, 0x15AC);
NW_ASSERT_OFFSET(APawn, ChatBalloon, 0x1604);
NW_ASSERT_OFFSET(APawn, SelectFlags, 0x1650);
NW_ASSERT_OFFSET(APawn, SelectIndex, 0x1654);
NW_ASSERT_SIZE(APawn, 0x16CC);

class AController : public AActor
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

#include "gen/AController.vt.inc"

    APawn* Pawn;                    // +0x398 (C2 +0x394; C1 +0x5DC) the pawn it drives
    BYTE _Unknown39C[0x3F0 - 0x39C];
    AActor* Target;                 // +0x3F0 (C2 +0x3EC; C1 +0x634, script SelectedActor; name ours) the player's (the viewers cast at it)
    DWORD Unknown638;               // +0x3F4 (C2 +0x3F0; C1 +0x638, bDead, bAutoAttacking, bAttackIntend) bits: the simulator's attacks set 2 and 4
    BYTE _Unknown3F8[0x40C - 0x3F8];
    BYTE MoveMode;                  // +0x40C (C2 +0x408; C1 +0x650, script MoveType; name ours) 1 running, 0 walking (status window)
    BYTE _Unknown40D[0x4D0 - 0x40D];
    UClass* PawnClass;              // +0x4D0 (C2 +0x4CC; C1 +0x714) what it spawns (the viewers load one by name)
    BYTE _Unknown4D4[0x538 - 0x4D4];

private:
    ENGINE_API static UClass PrivateStaticClass;
};
NW_ASSERT_OFFSET(AController, Pawn, 0x398);
NW_ASSERT_OFFSET(AController, Target, 0x3F0);
NW_ASSERT_OFFSET(AController, Unknown638, 0x3F4);
NW_ASSERT_OFFSET(AController, MoveMode, 0x40C);
NW_ASSERT_OFFSET(AController, PawnClass, 0x4D0);
NW_ASSERT_SIZE(AController, 0x538);

// an item on the ground (NConsoleWnd::GetPickupUnderMouse)
class AL2Pickup : public AActor
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

private:
    ENGINE_API static UClass PrivateStaticClass;
};

class APlayerController : public AController
{
};

// the player's controller of the L2 client (the C2 NWindow names its class: StaticClass; C2 script
// class LineagePlayerController extends PlayerController)
class ALineagePlayerController : public APlayerController
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

private:
    ENGINE_API static UClass PrivateStaticClass;
};

// UE2's bases: FOutputDevice at +0x28, FExec at +0x2C (the console's commands go to the
// viewport's Exec)
class UPlayer : public UObject, public FOutputDevice, public FExec
{
public:
#include "gen/UPlayer.vt.inc"

    APlayerController* Actor;       // +0x30 (UPlayer::Destroy: Actor->GetLevel()->DestroyActor(Actor, 1))
};
NW_ASSERT_OFFSET(UPlayer, Actor, 0x30);

// UE2 input keys (the ones NWindow tests)
enum EInputKey { IK_Shift = 0x10 };

class UInput : public USubsystem
{
public:
    ENGINE_API BYTE KeyDown(INT Key);
};

class UViewport : public UPlayer
{
public:
#include "gen/UViewport.vt.inc"

    BYTE _Unknown34[0x68 - 0x34];
    UCanvas* Canvas;                // +0x68 (UE2's; UViewport::Destroy deletes it after the input
                                    //   and the console)
    UInput* Input;                  // +0x6C (NWindow: Shift held for /actions)
    URenderDevice* RenDev;          // +0x70 (UClient::Flush calls its virtual +0x7C, Flush)
    BYTE _Unknown74[0x88 - 0x74];
    INT SizeX;                      // +0x88 (NConsoleWnd::AcceptGameStart)
};
NW_ASSERT_OFFSET(UViewport, Input, 0x6C);
NW_ASSERT_OFFSET(UViewport, RenDev, 0x70);
NW_ASSERT_OFFSET(UViewport, Canvas, 0x68);
NW_ASSERT_OFFSET(UViewport, SizeX, 0x88);

class UClient : public USubsystem
{
public:
    TArray<UViewport*> Viewports;   // +0x2C
    BYTE _Unknown38[0x60 - 0x38];
    // the config properties of the client (l2.ini [WinDrv.WindowsClient]); the offsets come from
    // UClient::StaticConstructor (Engine.dll 0x10491F0A..) and from WinDrv's ToggleFullscreen /
    // EndFullscreen, which read them: window -> fullscreen opens the device at the Fullscreen size,
    // fullscreen -> window at the Windowed one
    INT WindowedViewportX;          // +0x60
    INT WindowedViewportY;          // +0x64
    INT FullscreenViewportX;        // +0x68
    INT FullscreenViewportY;        // +0x6C
    BYTE _Unknown70[0x78 - 0x70];
    FLOAT Gamma;                    // +0x78 (the video options; UEngine::UpdateGamma applies it)
};
NW_ASSERT_OFFSET(UClient, Viewports, 0x2C);
NW_ASSERT_OFFSET(UClient, WindowedViewportX, 0x60);
NW_ASSERT_OFFSET(UClient, FullscreenViewportY, 0x6C);
NW_ASSERT_OFFSET(UClient, Gamma, 0x78);

// UE2's URL (0x44 bytes: protocol, host, port, map, options, portal, valid); only built and
// handed to UEngine's Browse (NConsoleWnd::RestartToLobby)
enum ETravelType { TRAVEL_Absolute, TRAVEL_Partial, TRAVEL_Relative };
class FURL
{
public:
    ENGINE_API FURL(const TCHAR* Filename);
    ENGINE_API FURL(FURL* Base, const TCHAR* TextURL, ETravelType Type);
    ENGINE_API FURL(const FURL& Other);
    ENGINE_API ~FURL();
    ENGINE_API void LoadURLConfig(const TCHAR* Section, const TCHAR* Filename);
    ENGINE_API void AddOption(const TCHAR* Str);

private:
    BYTE _Data[0x44];
};
NW_ASSERT_SIZE(FURL, 0x44);

class FNetworkNotify
{
public:
    virtual EAcceptConnection NotifyAcceptingConnection() = 0;
    virtual void NotifyAcceptedConnection(UNetConnection* Connection) = 0;
    virtual UBOOL NotifyAcceptingChannel(UChannel* Channel) = 0;
    virtual ULevel* NotifyGetLevel() = 0;
    virtual void NotifyReceivedText(UNetConnection* Connection, const TCHAR* Text) = 0;
    virtual UBOOL NotifySendingFile(UNetConnection* Connection, FGuid GUID) = 0;
    virtual void NotifyReceivedFile(UNetConnection* Connection, INT PackageIndex, const TCHAR* Error,
                                    UBOOL Skipped) = 0;
    virtual void NotifyProgress(const TCHAR* Str1, const TCHAR* Str2, FLOAT Seconds) = 0;
};

class ULevelBase : public UObject, public FNetworkNotify
{
};

// ??_7ULevel@@6BUObject@@@: UObject 25 + 49 = 74 slots in C2 (C1: 24 + 45); the ULevelObject
// ones (CompactLevelObjects, DestroyLevelObject, CleanupDestroyedLevelObject, SpawnLevelObject)
// are new in C2
class ULevel : public ULevelBase
{
public:
    virtual void Modify(UBOOL DoTransArrays = 0);                                              // 25
    virtual void SetActorCollision(UBOOL bCollision, UBOOL bForce = 0);                        // 26
    virtual void Tick(ELevelTick TickType, FLOAT DeltaSeconds);                                // 27
    virtual void TickNetClient(FLOAT DeltaSeconds);                                            // 28
    virtual void L2TickNetClient(FLOAT DeltaSeconds);                                          // 29
    virtual void TickNetServer(FLOAT DeltaSeconds);                                            // 30
    virtual INT ServerTickClient(UNetConnection* Conn, FLOAT DeltaSeconds);                    // 31
    virtual void ReconcileActors();                                                            // 32
    virtual void RememberActors();                                                             // 33
    virtual UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar);                                   // 34
    virtual void ShrinkLevel();                                                                // 35
    virtual void CompactActors();                                                              // 36
    virtual void CompactLevelObjects();                                                        // 37 (new in C2)
    virtual UBOOL Listen(FString& Error);                                                      // 38
    virtual UBOOL IsServer();                                                                  // 39
    virtual UBOOL MoveActor(AActor* Actor, FVector Delta, FRotator NewRotation, FCheckResult& Hit,
                            UBOOL Test = 0, UBOOL IgnorePawns = 0, UBOOL bIgnoreBases = 0,
                            UBOOL bNoFail = 0);                                                // 40
    virtual UBOOL FarMoveActor(AActor* Actor, FVector DestLocation, UBOOL Test = 0,
                               UBOOL bNoCheck = 0, UBOOL bAttachedMove = 0);                   // 41
    virtual UBOOL DestroyActor(AActor* Actor, UBOOL bNetForce = 0);                            // 42
    virtual UBOOL DestroyLevelObject(ULevelObject* LevelObject);                               // 43 (new in C2)
    virtual void CleanupDestroyed(UBOOL bForce);                                               // 44
    virtual void CleanupDestroyedLevelObject(UBOOL bForce);                                    // 45 (new in C2; argument name ours)
    virtual ULevelObject* SpawnLevelObject(UClass* Class, FName InName, FVector Location,
                                           ULevelObject* Owner);                               // 46 (new in C2; argument names ours)
    virtual AActor* SpawnActor(UClass* Class, FName InName = NAME_None,
                               FVector Location = FVector(0, 0, 0),
                               FRotator Rotation = FRotator(0, 0, 0), AActor* Template = NULL,
                               UBOOL bNoCollisionFail = 0, UBOOL bRemoteOwned = 0,
                               AActor* Owner = NULL, APawn* Instigator = NULL);                // 47
    virtual ABrush* SpawnBrush();                                                              // 48
    virtual void SpawnViewActor(UViewport* Viewport);                                          // 49
    virtual APlayerController* SpawnPlayActor(UPlayer* Viewport, ENetRole RemoteRole,
                                              const FURL& URL, FString& Error);                // 50
    virtual UBOOL FindSpot(FVector Extent, FVector& Location);                                 // 51
    virtual UBOOL CheckSlice(FVector& Location, FVector Extent, INT& bKeepTrying);             // 52
    virtual UBOOL CheckEncroachment(AActor* Actor, FVector TestLocation, FRotator TestRotation,
                                    UBOOL bTouchNotify);                                       // 53
    virtual UBOOL SinglePointCheck(FCheckResult& Hit, FVector Location, FVector Extent,
                                   DWORD ExtraNodeFlags, ALevelInfo* Level, UBOOL bActors);    // 54
    virtual UBOOL EncroachingWorldGeometry(FCheckResult& Hit, FVector Location, FVector Extent,
                                           DWORD ExtraNodeFlags, ALevelInfo* Level);           // 55
    virtual UBOOL SingleLineCheck(FCheckResult& Hit, AActor* SourceActor, const FVector& End,
                                  const FVector& Start, DWORD TraceFlags, FVector Extent);     // 56
    virtual FCheckResult* GetPawnListExtent(FMemStack& Mem, INT P2, INT P3, INT P4, INT P5,
                                            INT P6);                                           // 57
    virtual FCheckResult* GetPawnListExtent2(FMemStack& Mem, AActor* SourceActor,
                                             const FVector& End, const FVector& Start,
                                             FVector Extent);                                  // 58
    virtual FCheckResult* MultiPointCheck(FMemStack& Mem, FVector Location, FVector Extent,
                                          DWORD ExtraNodeFlags, ALevelInfo* Level, UBOOL bActors,
                                          UBOOL bOnlyWorldGeometry, UBOOL bSingleResult);      // 59
    virtual FCheckResult* MultiLineCheck(FMemStack& Mem, FVector End, FVector Start,
                                         FVector Size, ALevelInfo* LevelInfo, DWORD TraceFlags,
                                         AActor* SourceActor);                                 // 60
    virtual void DetailChange(EDetailMode NewDetailMode);                                      // 61
    virtual UBOOL TickDemoRecord(FLOAT DeltaSeconds);                                          // 62
    virtual UBOOL TickDemoPlayback(FLOAT DeltaSeconds);                                        // 63
    virtual void UpdateTime(ALevelInfo* Info);                                                 // 64
    virtual UBOOL IsPaused();                                                                  // 65
    virtual UBOOL L2SingleLineCheck(FCheckResult& Hit, AActor* SourceActor, const FVector& End,
                                    const FVector& Start, DWORD TraceFlags, FVector Extent);   // 66
    virtual FCheckResult* L2MultiLineCheck(FMemStack& Mem, FVector End, FVector Start,
                                           FVector Size, ALevelInfo* LevelInfo, DWORD TraceFlags,
                                           AActor* SourceActor);                               // 67
    virtual AActor* FindActorWithCreatureID(INT CreatureID);                                   // 68
    virtual void ResetActorStaticLight();                                                      // 69
    virtual void WelcomePlayer(UNetConnection* Connection, TCHAR* Optional);                   // 70
    virtual void BuildRenderData();                                                            // 71
    virtual UBOOL IsAudibleAt(FVector SoundLocation, FVector ListenerLocation, AActor* SoundActor,
                              ESoundOcclusion SoundOcclusion);                                 // 72
    virtual FLOAT CalculateRadiusMultiplier(INT P1, INT P2);                                   // 73
};

/*-----------------------------------------------------------------------------
    UCanvas. 69 virtual slots in C2 (UObject 25 + 44; C1: 64 = 24 + 40), order and signatures read
    from Engine.dll's ??_7UCanvas@@6B@. C2 vs C1: LoadFont and DestroyFont are new (34, 35);
    DrawActor takes (X, Y, Actor, Scale); DrawText is DrawTextToCanvas (the winuser.h macro) with two
    more arguments, its ColorInfo list holds pointers, and DrawThaiText, DrawNormalText, GetTextSize
    and GetThaiTextSize are new; ImportPledgeCrest and ImportAllyCrest are no longer virtual
    (ImportCrestFile, MakeDDSCrestFile below). The class is 0x13C bytes (C1: 0x134): the fields
    before m_L2Font did not move. Most fields not mapped yet: never embed or allocate a UCanvas.

    MSVC puts overloaded virtuals next to each other in REVERSE declaration
    order, so each overload pair below is declared second-slot first.
-----------------------------------------------------------------------------*/

struct ThaiCharacter;                  // Thai text: a glyph of the C2 font engine (layout not known)
enum EFontExceptionType : INT;         // values not known (opaque: the argument only travels)

class UCanvas : public UObject
{
public:
    BYTE _Unknown28[0x98 - 0x28];
    UViewport* Viewport;   // +0x98 (UCanvas::Init)
    BYTE _Unknown9C[0x130 - 0x9C];
    UL2Font* L2Fonts[3];   // +0x130 indexed by L2FontType (UCanvas::GetTextExtent); the script's m_L2Font[3]
                           //   (C1: +0x12C and two of them)
    BYTE _Unknown13C[0x1A4 - 0x13C];   // C3: twelve more script variables (the broadcast observer's textures...,
                                       //   m_pBroadcastObserverNumberTex* up to +0x180+0x24)

    virtual void Init(UViewport* InViewport);                                                   // 25 +0x64
    virtual void Update();                                                                      // 26 +0x68
    virtual void DrawTile(UMaterial* Material, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U,
                          FLOAT V, FLOAT UL, FLOAT VL, FLOAT Z, FPlane Color, FPlane Fog);       // 27 +0x6c
    virtual void DrawIcon(UMaterial* Material, FLOAT ScreenX, FLOAT ScreenY, FLOAT XSize,
                          FLOAT YSize, FLOAT Z, FPlane Color, FPlane Fog);                      // 28 +0x70
    virtual void DrawPattern(UMaterial* Material, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL,
                             FLOAT Scale, FLOAT OrgX, FLOAT OrgY, FLOAT Z, FPlane Color,
                             FPlane Fog);                                                       // 29 +0x74
    virtual void VARARGS WrappedStrLenf(UFont* Font, INT& XL, INT& YL, const TCHAR* Fmt, ...);  // 31 +0x7c
    virtual void VARARGS WrappedStrLenf(UFont* Font, FLOAT ScaleX, FLOAT ScaleY, INT& XL,
                                        INT& YL, const TCHAR* Fmt, ...);                        // 30 +0x78
    virtual void VARARGS WrappedPrintf(UFont* Font, UBOOL Center, const TCHAR* Fmt, ...);       // 33 +0x84
    virtual void VARARGS WrappedPrintf(UFont* Font, FLOAT ScaleX, FLOAT ScaleY, UBOOL Center,
                                       const TCHAR* Fmt, ...);                                  // 32 +0x80
    virtual void LoadFont(INT Index, TCHAR* P2, TCHAR* P3);                                     // 34 +0x88 (new in C2; argument names ours)
    virtual void DestroyFont(INT Index);                                                        // 35 +0x8c (new in C2)
    virtual void ReleaseClip();                                                                 // 36 +0x90
    virtual void DrawStretchedTex(INT X, INT Y, INT XL, INT YL, UTexture* Tex, bool P6,
                                  BYTE P7);                                                     // 38 +0x98 (C3: + P6, P7; names ours)
    virtual void DrawStretchedTex(INT X, INT Y, INT XL, INT YL, INT SrcX, INT SrcY, INT SrcXL,
                                  INT SrcYL, UTexture* Tex, BYTE P10);                          // 37 +0x94 (C3: + P10)
    virtual void DrawTexture(INT X, INT Y, INT XL, INT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL,
                             UTexture* Tex, BYTE P10, bool P11);                                // 39 +0x9c (C3: + P10, P11)
    virtual void DrawLine(INT X1, INT Y1, INT X2, INT Y2, DWORD Color, INT Width);              // 40 +0xa0
    virtual void DrawActor(INT X, INT Y, AActor* Actor, FLOAT Scale);                           // 41 +0xa4 (C1: (Actor, X, Y, Z); the last argument name ours)
    virtual void DrawRect(INT X, INT Y, INT XL, INT YL, UTexture* Tex, DWORD Color);            // 42 +0xa8
    virtual DWORD DrawHtmlText(INT X, INT Y, DWORD Color, const TCHAR* Text, DWORD P5, DWORD P6,
                               DWORD P7, FLOAT P8, INT P9, L2FontType FontType, INT P11,
                               INT P12, DWORD P13, INT P14);                                    // 43 +0xac
    virtual void DrawBrightenTile(UMaterial* Material, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL,
                                  FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FLOAT Z, FPlane Color,
                                  FPlane Fog);                                                  // 44 +0xb0
    virtual void DrawBrightenTexture(INT X, INT Y, INT XL, INT YL, FLOAT U, FLOAT V, FLOAT UL,
                                     FLOAT VL, UTexture* Tex, DWORD Brightness);                // 45 +0xb4
    // C1's DrawText (the winuser.h macro renames it): C2 calls it DrawTextToCanvas, with the
    // exception type and one more word at the end (P18, ExceptionType: names ours)
    virtual DWORD DrawTextToCanvas(INT X, INT Y, DWORD Color, const TCHAR* Text, DWORD P5, DWORD P6,
                                   DWORD P7, FLOAT P8, INT P9, L2FontType FontType, INT P11, INT P12,
                                   DWORD P13, INT P14, INT P15, INT P16,
                                   TArray<FL2ColorFontInfo*>* ColorInfo, _WORD P18,
                                   EFontExceptionType ExceptionType);                           // 46 +0xb8
    virtual DWORD DrawThaiText(INT X, INT Y, DWORD Color, TArray<ThaiCharacter> Text, DWORD P5,
                               DWORD P6, DWORD P7, FLOAT P8, INT P9, L2FontType FontType, INT P11,
                               INT P12, DWORD P13, INT P14, INT P15, INT P16,
                               TArray<FL2ColorFontInfo*>* ColorInfo, _WORD P18,
                               EFontExceptionType ExceptionType);                               // 47 +0xbc (new in C2)
    virtual DWORD DrawNormalText(INT X, INT Y, DWORD Color, const TCHAR* Text, DWORD P5, DWORD P6,
                                 DWORD P7, FLOAT P8, INT P9, L2FontType FontType, INT P11,
                                 INT P12, DWORD P13, INT P14, INT P15, INT P16,
                                 TArray<FL2ColorFontInfo*>* ColorInfo, _WORD P18,
                                 EFontExceptionType ExceptionType);                             // 48 +0xc0 (new in C2)
    virtual void Draw3DCoordText(FLevelSceneNode* SceneNode, FRenderInterface* RI, FVector Loc,
                                 DWORD Color, TCHAR* Text, UTexture* Tex, INT P7, INT P8,
                                 L2FontType FontType, INT P10);                                 // 50 +0xc8
    virtual void Draw3DCoordText(FVector Loc, DWORD Color, TCHAR* Text, UTexture* Tex, INT P5,
                                 INT P6, L2FontType FontType, INT P8);                          // 49 +0xc4
    virtual DWORD GetTextExtent(const TCHAR* Text, SIZE* Size, DWORD P3, DWORD P4, INT P5,
                                L2FontType FontType, INT P7, DWORD P8);                         // 51 +0xcc
    virtual DWORD GetTextSize(const TCHAR* Text, SIZE* Size, DWORD P3, DWORD P4, INT P5,
                              L2FontType FontType, INT P7, DWORD P8);                           // 52 +0xd0 (new in C2; argument names ours)
    virtual DWORD GetThaiTextSize(TArray<ThaiCharacter> Text, SIZE* Size, DWORD P3, DWORD P4,
                                  INT P5, L2FontType FontType, INT P7, DWORD P8);               // 53 +0xd4 (new in C2)
    virtual void DrawTargetName(FLevelSceneNode* SceneNode, FRenderInterface* RI, FVector Loc,
                                DWORD Color, User* Target, TargetRenderType RenderType,
                                L2FontType FontType);                                           // 54 +0xd8
    virtual void DrawTargetOptionName(FLevelSceneNode* SceneNode, FRenderInterface* RI,
                                      FVector Loc, DWORD Color, User* Target,
                                      TargetRenderType RenderType, L2FontType FontType);        // 55 +0xdc
    virtual void DrawTargetTex(FLOAT P1, FLOAT P2, FLOAT P3, FLOAT P4, FLOAT P5, FLOAT P6,
                               FLOAT P7, FLOAT P8, FLOAT P9, UTexture* Tex);                    // 56 +0xe0
    virtual void DrawDepthBar(FLevelSceneNode* SceneNode, FRenderInterface* RI, FVector Loc,
                              DWORD P4, DWORD P5, INT P6, FLOAT P7, bool P8);                   // 57 +0xe4 (C3: P6 INT, + P7, P8)
    virtual DWORD DrawSpecialDigit(INT DigitWidth, INT X, INT Y, DWORD Color, TCHAR* Text,
                                   UTexture* DigitTexture);                                     // 58 +0xe8
    virtual void WrapStringToArray(const TCHAR* Text, TArray<FString>* OutArray, FLOAT Width,
                                   UFont* Font, TCHAR EOL);                                     // 59 +0xec
    virtual void ClippedStrLen(UFont* Font, FLOAT ScaleX, FLOAT ScaleY, INT& XL, INT& YL,
                               const TCHAR* Text);                                              // 60 +0xf0
    virtual void ClippedPrint(UFont* Font, FLOAT ScaleX, FLOAT ScaleY, UBOOL Checked,
                              const TCHAR* Text);                                               // 61 +0xf4
    virtual void DrawTileStretched(UMaterial* Material, FLOAT Left, FLOAT Top, FLOAT AWidth,
                                   FLOAT AHeight);                                              // 62 +0xf8
    virtual void DrawTileScaled(UMaterial* Material, FLOAT Left, FLOAT Top, FLOAT NewXScale,
                                FLOAT NewYScale);                                               // 63 +0xfc
    virtual void DrawTileBound(UMaterial* Material, FLOAT Left, FLOAT Top, FLOAT Width,
                               FLOAT Height);                                                   // 64 +0x100
    virtual void DrawTileJustified(UMaterial* Material, FLOAT Left, FLOAT Top, FLOAT Width,
                                   FLOAT Height, BYTE Justification);                           // 65 +0x104
    virtual void DrawTileScaleBound(UMaterial* Material, FLOAT Left, FLOAT Top, FLOAT Width,
                                    FLOAT Height);                                              // 66 +0x108
    virtual void VARARGS DrawTextJustified(BYTE Justification, FLOAT X1, FLOAT Y1, FLOAT X2,
                                           FLOAT Y2, const TCHAR* Fmt, ...);                    // 67 +0x10c
    virtual void SetClip(INT X, INT Y, INT XL, INT YL);                                         // 68 +0x110
    // new in C3: the TrueType text path, the chat balloons and the broadcast observer (argument names ours)
    virtual DWORD DrawTrueFontText(INT X, INT Y, DWORD Color, const TCHAR* Text, DWORD P5, DWORD P6,
                                   DWORD P7, FLOAT P8, INT P9, L2FontType FontType, INT P11, INT P12,
                                   DWORD P13);                                                  // 69 +0x114
    virtual DWORD GetTrueFontTextSize(const TCHAR* Text, SIZE* Size, DWORD P3, DWORD P4, INT P5,
                                      L2FontType FontType, INT P7, DWORD P8);                   // 70 +0x118
    virtual DWORD DrawTrueFontSingleLineText(INT X, INT Y, DWORD Color, const TCHAR* Text, FLOAT P5,
                                             L2FontType FontType);                              // 71 +0x11c
    virtual DWORD DrawTrueFontHtmlText(INT X, INT Y, DWORD Color, const TCHAR* Text, DWORD P5, DWORD P6,
                                       DWORD P7, INT P8, L2FontType FontType, INT P10, INT P11,
                                       DWORD P12, INT P13, INT P14,
                                       TArray<FL2ColorFontInfo*>* ColorInfo);                   // 72 +0x120
    virtual void DrawChatting(FVector Loc, DWORD Color, TCHAR* Text, INT P4, INT P5,
                              L2FontType FontType, INT P7, INT P8, DWORD P9);                   // 73 +0x124
    virtual void StartBroadcastObserverMode();                                                  // 74 +0x128
    virtual void FinishBroadcastObserverMode();                                                 // 75 +0x12c
    virtual void ToggleBroadcastObserverTargetName();                                           // 76 +0x130

    // non-virtual L2 additions exported by Engine.dll
    // C2's DrawRadar() is gone in C3 (DrawRadarEtc / DrawRadarBack below)
    ENGINE_API void DrawRadarTarget(INT X, INT Y);
    ENGINE_API void DrawMinimapLoc(INT X, INT Y, UTexture* Tex);
    // new in C3 (imported by the C3 NWindow; argument names ours)
    ENGINE_API void DrawRadarEtc();
    ENGINE_API void DrawRadarBack();
    ENGINE_API void DrawMinimapTex(INT X, INT Y, INT XL, INT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL,
                                   UTexture* Tex);
    // new in C2 (the C2 NWindow imports these two; argument names ours)
    ENGINE_API void MakeBMPToDXT1(TCHAR* Source, TCHAR* Dest, INT& Size, INT P4, INT P5);   // C3: + P5 (argument names ours)
    ENGINE_API void MakeThaiCharArray(const TCHAR* Text, TArray<ThaiCharacter>& Chars, TCHAR* P3);
};
NW_ASSERT_OFFSET(UCanvas, Viewport, 0x98);
NW_ASSERT_OFFSET(UCanvas, L2Fonts, 0x130);
NW_ASSERT_SIZE(UCanvas, 0x1A4);

/*-----------------------------------------------------------------------------
    UL2Font: L2's font interface. Abstract in Engine.dll (8 pure slots, 26-33; C1 had 4 pure
    slots, 25-28) and 0x2C bytes (the Engine's IMPLEMENT_CLASS size: no field of its own);
    NWindow's NL2Font implements it (names ours).
    C2: Load, GetTexture and GetChar are 26-28 (the shift of UObject's ClearL2Game), Clear is 30
    (the one that frees the pages and zeroes the counters), and four pure slots are new. What the
    C2 NL2Font code (0x10072310..0x100726a0) does with them:
      29 GetCharMask(C): the 5th word of the 20-byte glyph of a font file of the "v1.1" kind (the
         mask of the variants the glyph has); 0 for a font without that table
      31 GetMaterial(Mode): a UFinalBlend over the font texture (alpha blended, z write/test, alpha
         test, two sided), made on first use and kept in a global; NULL without a texture. Mode 1
         and any other value use two separate cached objects, built the same way
      32 HasTexture(): Texture != NULL
      33 GetCharMaskValue(C, Mask): for a single-bit Mask the glyph has, the page's table entry of
         that bit (a "v1.1" font page keeps 32 words); 0 when no page has it
    Texture sits at +0x2C of NL2Font (GetTexture returns it): it is NL2Font's, not this class's.
-----------------------------------------------------------------------------*/

struct FL2FontChar;

class UL2Font : public USubsystem
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }
    ENGINE_API virtual ~UL2Font();

    virtual INT Load(const TCHAR* TextureName, const TCHAR* FontFile) = 0;   // 26
    virtual UTexture* GetTexture() = 0;                                        // 27
    virtual FL2FontChar* GetChar(TCHAR C) = 0;                                 // 28
    virtual INT GetCharMask(TCHAR C) = 0;                                      // 29 (new in C2; name ours)
    virtual void Clear() = 0;                                                  // 30
    virtual UMaterial* GetMaterial(INT Mode) = 0;                              // 31 (new in C2; name ours)
    virtual UBOOL HasTexture() = 0;                                            // 32 (new in C2; name ours)
    virtual INT GetCharMaskValue(TCHAR C, INT Mask) = 0;                       // 33 (new in C2; name ours)

private:
    ENGINE_API static UClass PrivateStaticClass;
};
NW_ASSERT_SIZE(UL2Font, 0x2C);

/*-----------------------------------------------------------------------------
    FL2TextAlignParser (L2 line breaker). sizeof 0x6C: NWindow allocates it
    with new (NCGDevice::Init). The fields were mapped from the Engine.dll
    assembly (every offset verified there); only Width is declared here
    because SetWidthParam is inline for NWindow.
    GetLineBuffer's out value is the offset in Text where the returned line
    starts (stock 0x1049F613: mov [edx], ecx with ecx = [esi+0x14]), not a length.
-----------------------------------------------------------------------------*/

class FL2TextAlignParser
{
public:
    ENGINE_API FL2TextAlignParser();
    ENGINE_API ~FL2TextAlignParser();
    ENGINE_API void Clear();
    ENGINE_API void SetParserParam(TCHAR* Text, DWORD Width, UL2Font* Font, INT P4, INT P5, INT P6, INT P7,
                                   FRenderInterface* RI, INT P9);   // C3: P7, RI, P9 are new (?SetParserParam@FL2TextAlignParser@@QAEXPAGKPAVUL2Font@@HHHHPAVFRenderInterface@@H@Z)
    ENGINE_API TCHAR* GetLineBuffer(INT& LineStart);
    // exported too, but NWindow writes the field directly: inline in the headers it used
    void SetWidthParam(DWORD InWidth) { Width = InWidth; }

private:
    BYTE _Unknown00[0x48];
    DWORD Width;           // +0x48 line width in pixels
    BYTE _Unknown4C[0x6C - 0x4C];
};
NW_ASSERT_SIZE(FL2TextAlignParser, 0x6C);

/*-----------------------------------------------------------------------------
    The engine and the console base. NWindow talks to the game through the
    FL2NetNotify side of the engine (requests, interface sounds); its virtual
    list is generated from Engine.dll (include/ue2/gen).
-----------------------------------------------------------------------------*/

class FL2NetNotify
{
public:
#include "gen/FL2NetNotify.vt.inc"
};

// The engine's subsystems NWindow calls. Their virtuals are mostly pure in Engine.dll: the
// generated lists name them after the client's implementations (ALAudio.dll's
// UALAudioSubsystem, D3DDrv.dll's UD3DRenderDevice, and Engine.dll's own UNetworkHandler: the C2
// client has no Network.dll, the handler moved into the Engine).
class UAudioSubsystem : public USubsystem
{
public:
#include "gen/UAudioSubsystem.vt.inc"
};

// what URenderDevice::GetRenderCaps returns (UE2's; the video options read the pixel shader
// version)
struct FRenderCaps
{
    INT MaxSimultaneousTerrainLayers;  // +0x00
    INT PixelShaderVersion;            // +0x04 12 = 1.2
};

class URenderDevice : public USubsystem
{
public:
#include "gen/URenderDevice.vt.inc"
};

// the connection to the game server: the requests go through it
class UL2NetHandler : public USubsystem
{
public:
#include "gen/UL2NetHandler.vt.inc"
};

// UEngine : USubsystem (+0x00, FExec at +0x28), FL2NetNotify (+0x2C). Only the fields NWindow
// reads are placed.
// C3: the replay recorder / player the engine keeps (UEngine::ReplayManager); only what the NWindow calls is declared
// (the benchmark score and the state of the recording; the layout is the Engine's)
class FL2ReplayManager
{
public:
    ENGINE_API TCHAR* GetBMScore();     // ?GetBMScore@FL2ReplayManager@@QAEPAGXZ: the benchmark result text
    ENGINE_API void ResetBMData();
    ENGINE_API void SetState(INT NewState);
    ENGINE_API INT GetState();
};

// C3: a visual effect of a skill (script SkillVisualEffect; the Engine's class, 0x60 bytes). The NWindow's viewers
// trigger one on a pawn.
class USkillVisualEffect : public UObject
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }
    ENGINE_API void TriggerTransientEffect(APawn* Source, AActor* Target, INT Param);   // argument names ours

    BYTE _Unknown28[0x60 - 0x28];

private:
    ENGINE_API static UClass PrivateStaticClass;
};
NW_ASSERT_SIZE(USkillVisualEffect, 0x60);

// C3: APawn's magic (skill cast) record, script struct NMagicInfo, 168 bytes (C2: 124): the NWindow clears one
// through the Engine's FNMagicInfo::Clear (the cast fields moved with it: see APawn)
struct FNMagicInfo
{
    BYTE _Data[168];
    ENGINE_API void Clear(INT Param);   // ?Clear@FNMagicInfo@@QAEXH@Z (argument name ours)
};
NW_ASSERT_SIZE(FNMagicInfo, 168);

class UEngine : public USubsystem, public FL2NetNotify
{
public:
#include "gen/UEngine.vt.inc"

    BYTE _Unknown30[0x4C - 0x30];
    UClient* Client;                // +0x4C the viewports
    UAudioSubsystem* Audio;         // +0x50 NULL without sound
    URenderDevice* GRenDev;         // +0x54
    UL2NetHandler* NetHandler;      // +0x58 the requests to the server (C1: +0x5C; the stock C2 code of 110 functions
                                    //   reads it at +0x58: tools/port/verify_data.py)
    BYTE _Unknown5C[0x180 - 0x5C];
    FL2ReplayManager* ReplayManager; // +0x180 (script ReplayManager, new in C3): the replay recorder / player
};
NW_ASSERT_OFFSET(UEngine, ReplayManager, 0x180);
NW_ASSERT_OFFSET(UEngine, Client, 0x4C);       // the same in C1 and C2: the script's variables up to +0xAC did not move
NW_ASSERT_OFFSET(UEngine, Audio, 0x50);
NW_ASSERT_OFFSET(UEngine, GRenDev, 0x54);
NW_ASSERT_OFFSET(UEngine, NetHandler, 0x58);
NW_ASSERT_SIZE(UEngine, 0x184);   // C2 (C1: 0x164): UGameEngine's copy constructor copies its own fields from here

// The game's engine: the console's Engine is one. Only the fields NWindow reads.
class UGameEngine : public UEngine
{
public:
#include "gen/UGameEngine.vt.inc"

    // C2 offsets from the script layout (UEngine grew by 0x10, GSkyLevel is new at +0x17C); the
    // class is 0x330 bytes (C1: 0x2E8)
    ULevel* GLevel;                 // +0x184 (C2 +0x174; C1 +0x164) the level being played (UE2's; UT2003's GEntry,
                                    //   GPendingLevel and LastURL follow, the FURL at +0x184)
    BYTE _Unknown188[0x210 - 0x188];
    FVector MouseHitLocation;       // +0x210 (C2 +0x200; C1 +0x1EC; script ClickLocation) (names ours) where the line under
                                    //   the mouse hits the world
    BYTE _Unknown21C[0x22C - 0x21C];
    AActor* MouseHitActor;          // +0x22C (C2 +0x21C; C1 +0x208; script ClickActor) the actor it hits, NULL for none
                                    //   (both as UGameEngine::FindMouseTargetObject leaves them)
    BYTE _Unknown230[0x36C - 0x230];

    // new in C2 (imported by the C2 NWindow; argument names ours)
    ENGINE_API void L2_Teleport(AActor* Actor, FVector Location, INT P3, INT P4, INT P5);
};
NW_ASSERT_OFFSET(UGameEngine, GLevel, 0x184);
NW_ASSERT_OFFSET(UGameEngine, MouseHitLocation, 0x210);
NW_ASSERT_OFFSET(UGameEngine, MouseHitActor, 0x22C);
NW_ASSERT_SIZE(UGameEngine, 0x36C);

// UL2ConsoleWnd::State (names ours; only the values decompiled code uses)
enum L2ConsoleState
{
    L2CS_Login = 2,                     // the login screen (back from the server list)
    L2CS_Waiting = 1,                   // (name ours) a request of the character stage sent, the
                                        //   server's answer awaited
    L2CS_None = 0,                      // (name ours) MoveCameraByState's "from anywhere"
    L2CS_Create = 3,                    // (names ours) the character creation
    L2CS_Lobby = 4,                     //   the character selection
    L2CS_InGame = 6,                    //   in the world (AcceptGameStart)
    L2CS_Loading = 7,                   //   the loading screen: SetState keeps the new state in
                                        //   SavedState (phase 6, ReturnToLogin)
};

// the engine's ItemInfo, passed by value to three console slots (NWindow's declaration of it:
// NCItemWnd.h)
struct NItemRequest;

/*-----------------------------------------------------------------------------
    Game data tables (sysstring, items, skills...). Only placed members.
-----------------------------------------------------------------------------*/

// one line of systemmsg-*.dat (name ours; 0x24 bytes in C2, 0x14 in C1). C2: from the copy of a line
// (0x10055EE0: a dword, a string, two dwords, a string) and the readers (NConsoleWnd::ShowSystemMessage,
// EndSystemMessageParam: the table is at +0x2DD34 of the game data, 3000 lines)
struct FL2SystemMsgData
{
    INT Unknown0;                       // +0x00 0 when the line does not exist
    FString Message;                    // +0x04 (C1: FStringNoInit; the message of the line)
    INT Type;                           // +0x10 4: shown in a dialog box
    INT Unknown14;                      // +0x14 new in C2: ShowSystemMessage hands it to NCConsole::AddSystemMessage
                                        //   (the color, going by the C1 call)
    FString Message2;                   // +0x18 new in C2: a second text of the line (ShowSystemMessage gives it to
                                        //   0x100578D0 after the chat line)
    BYTE _Unknown24[0x30 - 0x24];       // +0x24 new in C3: 12 bytes of plain data (SystemMsgFormatClear walks the
                                        //   records at stride 0x30 and destroys only the FStrings at +4 and +0x18)
};
NW_ASSERT_OFFSET(FL2SystemMsgData, Message, 0x4);
NW_ASSERT_OFFSET(FL2SystemMsgData, Type, 0x10);
NW_ASSERT_OFFSET(FL2SystemMsgData, Message2, 0x18);
NW_ASSERT_SIZE(FL2SystemMsgData, 0x30);   // C3 (C2: 0x24)

// /useskill's table (FL2GameData::GetMSCommandData, name ours): only the first field is known
struct FL2MagicSkillCommandData
{
    INT SkillID;                        // +0x00
};

// A name of L2's own table (Engine.dll), kept by the data tables: an index, 4 bytes.
class L2FName
{
public:
    ENGINE_API const TCHAR* operator*() const;

private:
    INT Index;
};
NW_ASSERT_SIZE(L2FName, 4);

// one line of actionname-*.dat (name ours, 0x1C bytes; NCActionItemListWnd::LoadActions)
// C1 layout only: in C2 the actions are heap entries (FL2ActionEntry below) in a TArray of pointers; the type
// stays for NCPetWnd.cpp's LoadActionCategory (C1 code that nothing calls)
struct FL2ActionData
{
    INT bValid;                         // +0x00 0: no action here
    INT ActionID;                       // +0x04
    INT Param;                          // +0x08 the social action of the social ones
    INT Category;                       // +0x0C 1 basic, 2 party, 3 social, 4 pet, 5 summon
                                        //   (the pet and summon windows list 4 and 5)
    L2FName Name;                       // +0x10
    L2FName IconName;                   // +0x14
    L2FName Description;                // +0x18
};
NW_ASSERT_SIZE(FL2ActionData, 0x1C);

// an action of actionname-*.dat in C2 (name ours): a heap object, found by the first dword (set) and the id
// (NConsoleWnd's GetActionName 0x10060C40 and GetActionIconName 0x10060C90 walk the table). Only what those
// two read is placed; the size is not known
struct FL2ActionEntry
{
    INT bValid;                         // +0x00 0: no action here
    INT ActionID;                       // +0x04
    BYTE _Unknown08[0x1C - 0x08];
    FString IconName;                   // +0x1C GetActionIconName
    FString Name;                       // +0x28 GetActionName (C1: L2FName at +0x10)
};
NW_ASSERT_OFFSET(FL2ActionEntry, IconName, 0x1C);
NW_ASSERT_OFFSET(FL2ActionEntry, Name, 0x28);

// one level of a skill (FL2GameData::GetMSData, name ours): only the fields NWindow reads
// C2 layout: from the readers in NConsoleWnd (GetSkillName 0x10056880, GetSkillDesc 0x100568B0,
// GetSkillIconName 0x10056850, IsToggleSkill 0x100568E0, GetSkillMpCost 0x10056980, GetSkillRange 0x100569B0,
// GetSkillTypeText 0x10056910) and the Engine's GetMSData (compares +0x8, the level); C1 offsets in brackets.
// The record was reordered, not shifted: do not add the C1 offsets of fields that are not listed.
struct FL2MagicSkillData
{
    BYTE _Unknown0[0x4];
    INT SkillID;                        // +0x04 (the skill viewer walks the table) (C1 +0x04)
    INT Level;                          // +0x08 (C1 +0x08)
    INT OperateType;                    // +0x0C 2: passive, 3: toggle, else active (C1 +0x14)
    INT bMagic;                         // +0x10 shown as "Magic" instead (C1 +0x18)
    INT MpCost;                         // +0x14 (C1 +0x1C)
    INT Range;                          // +0x18 < 0: none (C1 +0x2C)
    BYTE _Unknown1C[0x20 - 0x1C];
    FName AnimName;                     // +0x20 (name ours) its animation (the pawn simulator casts the level-1 skills
                                        //   that have one) (C1 +0x44; NCPawnViewr_Simulator::OnCreate)
    BYTE _Unknown24[0x28 - 0x24];
    FString Name;                       // +0x28 (C1: L2FName at +0x4C)
    FString Description;                // +0x34 (C1: L2FName at +0x54)
    L2FName IconName;                   // +0x40 (C1 +0x50)
    // C1 had HitTime (FLOAT, +0x40: APawn::SetMagicInfo, the skill viewer's cast): its C2 offset is not derived
};
NW_ASSERT_OFFSET(FL2MagicSkillData, Level, 0x8);
NW_ASSERT_OFFSET(FL2MagicSkillData, OperateType, 0xC);
NW_ASSERT_OFFSET(FL2MagicSkillData, MpCost, 0x14);
NW_ASSERT_OFFSET(FL2MagicSkillData, Range, 0x18);
NW_ASSERT_OFFSET(FL2MagicSkillData, AnimName, 0x20);
NW_ASSERT_OFFSET(FL2MagicSkillData, Name, 0x28);
NW_ASSERT_OFFSET(FL2MagicSkillData, Description, 0x34);
NW_ASSERT_OFFSET(FL2MagicSkillData, IconName, 0x40);

// one line of questname-*.dat (name ours, 0x14 bytes): a quest's text at one step
struct FL2QuestNameData
{
    INT bValid;                         // +0x00 0: no quest here
    INT QuestID;                        // +0x04
    INT Step;                           // +0x08
    L2FName Name;                       // +0x0C the quest's (NConsoleWnd::AddQuestID)
    L2FName Text;                       // +0x10 the step's
};
NW_ASSERT_SIZE(FL2QuestNameData, 0x14);

// one line of castlename-*.dat (name ours; 0x2C bytes in C2, 0x14 in C1: the three texts are strings now)
// C2: NConsoleWnd::GetCastleName / GetCastleLocation / GetCastleDescription (0x10056E80, 0x10056ED0,
// 0x10056F20) walk the 100 lines at +0x5D7AC of the game data, `imul 0x2C`
struct FL2CastleNameData
{
    INT bValid;                         // +0x00 0: no castle here
    INT CastleID;                       // +0x04
    FString Name;                       // +0x08 (the system messages' castle parameter, type 5)
    FString Location;                   // +0x14 (names ours, after the columns of the later
    FString Description;                // +0x20   clients' castlename files)
};
NW_ASSERT_OFFSET(FL2CastleNameData, Name, 0x8);
NW_ASSERT_OFFSET(FL2CastleNameData, Location, 0x14);
NW_ASSERT_OFFSET(FL2CastleNameData, Description, 0x20);
NW_ASSERT_SIZE(FL2CastleNameData, 0x2C);

// one word of the chat filter (obscene-*.dat, FL2GameData::ObsceneDataLoad; name ours)
struct FL2ObsceneData
{
    INT Unknown0;                       // +0x00
    FString Word;                       // +0x04
};

// one line of abnormalgrp.txt + abnormalname-*.txt (name ours, 0x14 bytes): a buff/debuff's icon and text
// (AbnormalDataLoad writes bValid, AbnormalID and the icon; AbnormalNameDataLoad adds name and description
// to the line with the same abnormal_id)
struct FL2AbnormalData
{
    INT bValid;                         // +0x00 0: no abnormal status here
    INT AbnormalID;                     // +0x04 abnormal_id
    L2FName Name;                       // +0x08 name (abnormalname)
    L2FName Description;                // +0x0C desc (abnormalname)
    L2FName IconName;                   // +0x10 icon (abnormalgrp)
};
NW_ASSERT_SIZE(FL2AbnormalData, 0x14);

// the data tables' hash maps and entries (NWindow's declarations: NGameData.h)
template <class T> struct NDataMap;
struct NViewItemData;
struct NViewNpcData;
class FL2ItemDataBase;     // C2: what FL2GameData::GetItemData returns (a class: the 'V' of the mangled name)
struct FL2HennaData;       // C2: GetHennaData
struct FL2RecipeData;      // C2: GetRecipeDataByIndex / GetRecipeDataBy2Condition

// C3 layout (the object is 0x7AF90 bytes; C2: 0x605FC, C1: 0x3AF90). tools/ue2_gamedata_regions.py reads the copy
// constructor of both Engine.dlls, and every Engine.dll method that clears or reads a table pairs a C2
// displacement with its C3 one (the same function name in both builds): the 0x28..0xB0 block is new (0x88 bytes),
// BlowFish +0x84, HairData shrinks to 450 dwords (0x708 bytes) and 8 new bytes come before ItemTable, everything
// up to the 1000-record quest table is -0xFDC, the SystemMsg records are 0x30 (C2: 0x24), the strings and the
// 200/255-record tables +0x1A5A4, a fourth FString (+0x77D50) sits before CastleNames (+0x1A5B0), EnterEvents is
// +0x1A5A4 again, AgitInfo's pointer +0x1A988 and the last 0x418 bytes +0x1A994 (0x3E4 new bytes in between:
// regions of 0x40, 0x120 and 0x80 bytes). The C2 offsets are in the field comments as "C2 +0x..."; below is the C2
// description, which still holds for the members' meaning.
// C2 layout. The object is 0x605FC bytes (C1: 0x3AF90). tools/ue2_gamedata_regions.py reads both builds' copy
// constructors; the regions of C2 are: the first three the same as C1 (UObject, BlowFish, HairData, CharData up to
// +0x4CB8), then 1000 x 0x24 @ +0x4CF4, 3000 x 0x2C @ +0xD994, 3000 x 0x24 @ +0x2DD34 (SystemMsg), 200 x 0x1C @
// +0x48314, 255 x 0x1C @ +0x498F4, FString[3000] @ +0x4B4E8 (SysString) and @ +0x54188, FString[100] @ +0x5CE28
// (ClassInfo) and @ +0x5D2D8 (ClassType), 100 x 0x2C @ +0x5D7AC (CastleNames), 200 x 0x20 @ +0x5E8E0, the last
// 0x418 bytes @ +0x601E4. Every member below that NWindow reads has its offset from the stock code that reads it
// (the evidence is in the comment); the tables NWindow does not read are NOT placed (BYTE fillers, with what the
// copy constructor says): C1's QuestNames, Abnormals, PetActions, EnterEvents, Symbols, CharData's neighbours,
// the pointers at +0x4CBC..0x4CC8 and +0x4CD8, and the actions of C1 (a fixed array there, a TArray of
// pointers here). The member functions are exports and do not depend on this layout.
class FL2GameData
{
public:
    // reads Length bytes of a data file from Archive into Buffer: decrypts them with BlowFish when the archive says the file
    // is version 0xD3; otherwise (when the strict-file option is on) it tells "Files are corrupted" and asks to exit
    // C2: the export is ?GetTextInformation@FL2GameData@@QAEHPAVFArchive@@PAPAEAAH@Z, i.e. (FArchive*, BYTE**, INT&): the
    // signature CHANGED (C1: (FArchive*, BYTE* Buffer, INT Length)); what the two out arguments mean in C2 is not
    // read from its code yet (the buffer and its length, going by the types; argument names ours)
    ENGINE_API INT GetTextInformation(FArchive* Archive, BYTE** Buffer, INT& Length, INT P4);   // C3: + P4 (C2: three arguments)

    BYTE _UObject[0x28];                // +0x00 UObject (0x28 bytes in the C1)
    INT Unknown28;                      // +0x28 (NpcDataLoad sets it to 0; nothing else reads it)
    BYTE _Unknown2C[0xB0 - 0x2C];
    BYTE BlowFish[0x1048];              // +0xB0 (C2 +0x2C) FBlowFish: the cipher of the data files (18 + 4 * 256
                                        //   dwords of key schedule); GetTextInformation decrypts a
                                        //   file the archive calls version 0xD3 (Lineage2Ver211)
    DWORD HairData[450];                // +0x10F8 (C2 +0x1074, [1500]) hairgrp.txt (HairDataLoad)
    BYTE _Unknown1800[0x1808 - 0x1800];
    NDataMap<NViewItemData>* ItemTable; // +0x1808 (C2 +0x27E4; names ours) the items, by item id (the same offset in C2)
    NDataMap<NViewNpcData>* NpcTable;   // +0x180C (C2 +0x27E8) the NPCs, by npc id (the same in C2)
    BYTE CharData[15][0x274];           // +0x1810 (C2 +0x27EC) chargrp.txt (CharDataLoad): 15 records (the same in C2)
    NDataMap<FL2MagicSkillData>* SkillTable; // +0x3CDC (C2 +0x4CB8) the skills' levels (the same in C2: GetMSData reads it)
    BYTE _Unknown4CBC[0x4CC0 - 0x4CBC]; // +0x3CE0 (C2 +0x4CBC) C1: MSCommandTable (skillname -> /useskill)
    void* CreditTable;                  // +0x3CE4 (C2 +0x4CC0; name ours; new in C2) NCCreditWnd::OnCreate reads it
    BYTE _Unknown4CC4[0x4CCC - 0x4CC4]; // +0x3CE8 (C2 +0x4CC4) C1: CommandTable, MusicTable
    TArray<FL2ObsceneData*> ObsceneData; // +0x3CF0 (C2 +0x4CCC; name ours) the chat filter's words (C1 +0x4CC8: the Engine's
                                        //   ObsceneDataLoad; NWindow's HasBadWord 0x1005B430 reads +0x4CCC and +0x4CD0)
    BYTE _Unknown4CD8[0x4CDC - 0x4CD8]; // +0x3CFC (C2 +0x4CD8) C1: NpcSkillAnimTable
    TArray<FL2ActionEntry*> ActionTable; // +0x3D00 (C2 +0x4CDC; name ours) actionname-*.dat: heap entries (C1: Actions[1000] at +0x4CD8);
                                        //   0x10060C40 / 0x10060C90 read +0x4CDC (data) and +0x4CE0 (count)
    BYTE _Unknown3D0C[0x3F638 - 0x3D0C]; // +0x3D0C (C2 +0x4CE8) 12 bytes, then 1000 x 0x70 @ +0x3D18 (C2: 1000 x 0x24
                                        //   @ +0x4CF4: QuestData, the quest table: +0x18 -> +0x24 in a record), 3000 x 0x2C @
                                        //   +0x1F298 (C2 @ +0xD994), then 0x80 bytes (the same in both)
    FL2SystemMsgData SystemMsg[3000];   // +0x3F638 (C2 +0x2DD34) systemmsg-*.dat (NConsoleWnd checks id < 3000): `imul 0x24`
    BYTE _Unknown48314[0x4B4E8 - 0x48314]; // +0x628B8 (C2 +0x48314) 200 x 0x1C @ +0x48314, 255 x 0x1C @ +0x498F4, 16 bytes
                                        //   (C1: SkillSoundTable and StaticObjectTable were just before SysString)
    FString SysString[3000];            // +0x65A8C (C2 +0x4B4E8) sysstring-*.dat (FL2GameData::SysStringClear) (C1 +0x2E810); the string of id N
                                        //   is at +0x4B4E8 + 12 * N: GetClassNameString reads SysString[247 + ClassID] at
                                        //   +0x4C07C (C1 +0x2F3A4), GetSkillTypeText SysString[311..313] at +0x4C37C..0x4C394
    FString Unknown54188[3000];         // +0x6E72C (C2 +0x54188) a second table of 3000 strings (copy constructor)
    FString ClassInfo[100];             // +0x773CC (C2 +0x5CE28) classinfo-*.dat (name ours): 0 the creation
                                        //   hint, 1..9 the base classes (count from the file); the
                                        //   array has room for 100 (ClassInfoClear empties 100)
    FString ClassType[100];             // +0x7787C (C2 +0x5D2D8; name ours) ClassTypeLoad: "ClassType-k.txt", a
                                        //   fixed file name (the same four columns as classinfo)
    FString EulaText;                   // +0x77D2C (C2 +0x5D788; name ours; new in C2) NCEulaWnd and NCAnnounceWnd::OnKeyDown read it
    FString WarningNotice;              // +0x77D38 (C2 +0x5D794; name ours) the warning page shown after the
                                        //   announcements (WarningNoticeLoad; ShowWarningWnd reads +0x5D798)
    FString NoticeText;                 // +0x77D44 (C2 +0x5D7A0; name ours) the announcements shown before
                                        //   the server list (NConsoleWnd::StartNotice reads +0x5D7A4)
    BYTE _Unknown77D50[0x77D5C - 0x77D50];
    FL2CastleNameData CastleNames[100]; // +0x77D5C (C2 +0x5D7AC) castlename-*.dat: `imul 0x2C`
    BYTE _Unknown5E8DC[0x601E4 - 0x5E8DC]; // +0x78E8C (C2 +0x5E8DC) 4 bytes, then 200 x 0x20 @ +0x5E8E0 (C1: EnterEvents, Symbols)
    BYTE _Unknown7A794[0x7AB78 - 0x7A794];
    BYTE _Unknown601E4[0x418];          // +0x7AB78 (C2 +0x601E4) 262 dwords no loader or getter touches

    ENGINE_API INT GetCommandType(TCHAR* Command);   // commandname-*.dat: code of a chat command
    ENGINE_API FL2MagicSkillCommandData* GetMSCommandData(TCHAR* Command);
    ENGINE_API INT Load(INT P1);                     // reads the tables again (language change); C3: INT Load(INT) (C2: void Load())
    ENGINE_API TCHAR* GetStaticObjectName(INT ID);   // staticobject-*.dat (doors...)
    ENGINE_API TCHAR* GetItemName(INT ID);           // itemname-*.dat, by item id
    ENGINE_API TCHAR* GetItemDescription(INT ID);
    ENGINE_API FL2MagicSkillData* GetMSData(INT SkillID, INT Level);   // NULL: no such skill
    ENGINE_API TCHAR* GetNpcName(INT ID);            // npcname-*.dat, by npc id

    // new in C2 (imported by the C2 NWindow; argument names ours). The item, henna and recipe records
    // are only named here: their layouts are in the C2 Engine.dll, not in this header yet
    ENGINE_API FL2ItemDataBase* GetItemData(INT ItemID);                      // ?GetItemData@FL2GameData@@QAEPAVFL2ItemDataBase@@H@Z (a `class`)
    ENGINE_API FL2HennaData* GetHennaData(INT HennaID);                       // ?GetHennaData@FL2GameData@@QAEPAUFL2HennaData@@H@Z
    ENGINE_API FL2RecipeData* GetRecipeDataByIndex(INT Index);                // ?GetRecipeDataByIndex@FL2GameData@@QAEPAUFL2RecipeData@@H@Z
    ENGINE_API FL2RecipeData* GetRecipeDataBy2Condition(INT P1, INT P2);      // ?GetRecipeDataBy2Condition@FL2GameData@@QAEPAUFL2RecipeData@@HH@Z
    ENGINE_API TCHAR* GetItemAdditionalName(INT ItemID);                      // ?GetItemAdditionalName@FL2GameData@@QAEPAGH@Z
    ENGINE_API TCHAR* GetCmdStringByActionID(INT ActionID);                   // ?GetCmdStringByActionID@FL2GameData@@QAEPAGH@Z
    // new in C3 (imported by the C3 NWindow; argument names ours)
    ENGINE_API FL2QuestData* GetQuestData(INT QuestID, INT Step);             // ?GetQuestData@FL2GameData@@QAEPAUFL2QuestData@@HH@Z
    ENGINE_API INT GetZoneID(FLOAT X, FLOAT Y, FLOAT Z);                      // the zone at a world position
    ENGINE_API INT GetCurrentZoneID();                                        // the zone the player is in
    ENGINE_API TCHAR* GetCurrentZoneName();

    // C1 members that C2 does not have where they were (a fixed array of actions): the name stays, undefined, so
    // that NCPetWnd.cpp's LoadActionCategory (C1 code, nothing calls it) still parses; a use that is emitted does
    // not link
    static FL2ActionData* Actions;
};
NW_ASSERT_OFFSET(FL2GameData, BlowFish, 0xB0);
NW_ASSERT_OFFSET(FL2GameData, HairData, 0x10F8);
NW_ASSERT_OFFSET(FL2GameData, ItemTable, 0x1808);
NW_ASSERT_OFFSET(FL2GameData, NpcTable, 0x180C);
NW_ASSERT_OFFSET(FL2GameData, CharData, 0x1810);
NW_ASSERT_OFFSET(FL2GameData, SkillTable, 0x3CDC);
NW_ASSERT_OFFSET(FL2GameData, CreditTable, 0x3CE4);
NW_ASSERT_OFFSET(FL2GameData, ObsceneData, 0x3CF0);
NW_ASSERT_OFFSET(FL2GameData, ActionTable, 0x3D00);
NW_ASSERT_OFFSET(FL2GameData, SystemMsg, 0x3F638);
NW_ASSERT_OFFSET(FL2GameData, SysString, 0x65A8C);
NW_ASSERT_OFFSET(FL2GameData, Unknown54188, 0x6E72C);
NW_ASSERT_OFFSET(FL2GameData, ClassInfo, 0x773CC);
NW_ASSERT_OFFSET(FL2GameData, ClassType, 0x7787C);
NW_ASSERT_OFFSET(FL2GameData, EulaText, 0x77D2C);
NW_ASSERT_OFFSET(FL2GameData, WarningNotice, 0x77D38);
NW_ASSERT_OFFSET(FL2GameData, NoticeText, 0x77D44);
NW_ASSERT_OFFSET(FL2GameData, CastleNames, 0x77D5C);
NW_ASSERT_OFFSET(FL2GameData, _Unknown601E4, 0x7AB78);
NW_ASSERT_SIZE(FL2GameData, 0x7AF90);

ENGINE_API extern FL2GameData GL2GameData;
ENGINE_API extern FMemCache GCache;           // Core's cache (UEngine::Flush empties it): UE2Core.h

/*-----------------------------------------------------------------------------
    L2ParamStack: the argument list of the requests to the server (16 bytes: buffer, size,
    read and write positions). PushBack takes any 32-bit value.
-----------------------------------------------------------------------------*/

class L2ParamStack
{
public:
    ENGINE_API L2ParamStack(INT Count);
    ENGINE_API ~L2ParamStack();
    ENGINE_API INT PushBack(void* Value);
    ENGINE_API void* Top();             // takes the next value, first pushed first
    ENGINE_API L2ParamStack& operator=(L2ParamStack& Other);

private:
    void** Buffer;
    INT Size;
    INT ReadPos;
    INT WritePos;
};
NW_ASSERT_SIZE(L2ParamStack, 0x10);

/*-----------------------------------------------------------------------------
    ItemInfo: L2's record of an item as the server sends it (Engine.dll "ItemInfo", 0x30
    bytes): the console's item slots get it by reference, three of them by value; NWindow's
    item requests fill one (NItemRequest, NCItemWnd.h). Names ours, after what the console
    makes of the fields (NItemInfo).
-----------------------------------------------------------------------------*/

struct ItemInfo
{
    SWORD Type;                // +0x00 NItemInfo::Param (0..3: the enchant shows)
    SWORD Unknown02;           // +0x02
    INT ObjectID;              // +0x04 the server's id of this one
    INT ItemID;                // +0x08 itemname and item table id
    INT Count;                 // +0x0C
    INT Unknown10;             // +0x10 (NItemInfo +0xC1C: the shops' and stores' lists)
    INT Unknown14;             // +0x14
    INT BodyPart;              // +0x18 slot mask
    SWORD Unknown1C;           // +0x1C (NItemInfo +0xC84)
    SWORD bEquipped;           // +0x1E (NItemInfo +0xC8C)
    INT Enchant;               // +0x20
    INT Unknown24;             // +0x24 (NItemInfo +0xC70)
    INT Unknown28;             // +0x28 (NItemInfo +0xC98: the private stores' price)
    INT Unknown2C;             // +0x2C (NItemInfo +0xC90: the warehouse's and the buy
                               //   stores' lists)
};
NW_ASSERT_OFFSET(ItemInfo, ItemID, 0x8);
NW_ASSERT_OFFSET(ItemInfo, bEquipped, 0x1E);
NW_ASSERT_SIZE(ItemInfo, 0x30);

// The new character NCPawnCreateWnd asks the server for (Engine.dll "CharacterInfo": 0x60 bytes in C2, C1's was 0x58,
// passed by value to UL2NetHandler::RequestCharacterCreate (NConsoleWnd::RequestCharacterCreate: ret 0x60); field
// names ours). The name is 24 characters now (C1: 20), so every field after it is 8 bytes further.
// NCPawnSetupWnd::GetCreateInfo fills the name and the choices; the creation window adds the
// stats of the last class template the server sent and the hair and face (NCharacterInfo, NCPawnCreateWnd.h, is the
// same record).
struct CharacterInfo
{
    TCHAR Name[24];            // +0x00 (C1: 20)
    INT Race;                  // +0x30 0 human, 1 elf, 2 dark elf, 3 orc, 4 dwarf (C1 +0x28)
    INT ClassID;               // +0x34 base class (NCPawnSetupWnd::GetClassID)
    INT Sex;                   // +0x38 0 male, 1 female
    INT Stats[6];              // +0x3C STR, DEX, CON, INT, WIT, MEN
    INT HairStyle;             // +0x54
    INT HairColor;             // +0x58
    INT Face;                  // +0x5C
};
NW_ASSERT_OFFSET(CharacterInfo, Race, 0x30);
NW_ASSERT_OFFSET(CharacterInfo, Stats, 0x3C);
NW_ASSERT_OFFSET(CharacterInfo, HairStyle, 0x54);
NW_ASSERT_SIZE(CharacterInfo, 0x60);

// L2's record of a clan (Engine.dll "PledgeInfo"): the console copies it into NPledgeInfo
// (NCPledgeWnd.h), whose names these follow; the network handler keeps one per clan
// (UL2NetHandler::GetPledgeInfo), with the crest textures. Only the fields NWindow reads.
// C2 layout: the three texts are shorter (Name and LeaderName 24 characters, AllyName 26: -0x10 each), so every
// field after the name is at its C1 offset minus 0x10, 0x20 or 0x30 (in brackets): from the copy into an NPledgeInfo
// (NConsoleWnd::ReceivePledgeInfo, python tools/port/packet.py) and the readers in NCDetailStatusWnd::OnPaint and
// NCTargetStatusWnd::OnPaint (the crests). The size is not known.
struct PledgeInfo
{
    INT PledgeID;              // +0x00
    TCHAR Name[24];            // +0x04 (C1: [32])
    TCHAR LeaderName[24];      // +0x34 (C1 +0x44)
    INT Level;                 // +0x64 (C1 +0x84)
    UBOOL bHasCastle;          // +0x68 (C1 +0x88)
    UBOOL bHasHideout;         // +0x6C (C1 +0x8C)
    INT Unknown90;             // +0x70 (NPledgeInfo +0x94) (C1 +0x90)
    INT Unknown94;             // +0x74 (NPledgeInfo +0x98) (C1 +0x94)
    INT Status;                // +0x78 0..4, the status text's (C1 +0x98)
    INT Unknown9C;             // +0x7C (NPledgeInfo +0xE8) (C1 +0x9C)
    INT CrestID;               // +0x80 its texture is "crest.<server>_<id>" (the GM status
                               //   window finds it by that name) (C1 +0xA0)
    BYTE _UnknownA4[0x9C - 0x84];
    TCHAR AllyName[(0xD0 - 0x9C) / 2]; // +0x9C (size: up to the next known field) (C1 +0xBC)
    UTexture* CrestTex;        // +0xD0 NULL = no crest; the character status window draws it
                               //   directly (the engine hands the network handler each crest
                               //   texture it imports) (C1 +0x100)
    UTexture* AllyCrestTex;    // +0xD4 (tested the same way before the alliance crest is drawn) (C1 +0x104)
};
NW_ASSERT_OFFSET(PledgeInfo, LeaderName, 0x34);
NW_ASSERT_OFFSET(PledgeInfo, Level, 0x64);
NW_ASSERT_OFFSET(PledgeInfo, CrestID, 0x80);
NW_ASSERT_OFFSET(PledgeInfo, AllyName, 0x9C);
NW_ASSERT_OFFSET(PledgeInfo, CrestTex, 0xD0);

/*-----------------------------------------------------------------------------
    User: L2's record of a creature the client knows (Engine.dll "User", 0x1F8 bytes in C2, 0x208 in
    C1: the character selection keeps seven by value and copies them around). Only the fields NWindow
    reads are placed.

    C2 layout: derived, field by field, from the Engine's own User code (the constructor 0x1034E960,
    GetName, SetNickName, GetPledgeID, GetMeshType... : the C1 and C2 builds side by side) and from the
    NWindow functions that read the fields (NCDetailStatusWnd::OnPaint, NCGameOptionWnd::ShowWindow,
    NCEquipItemWnd::OnLButtonUp, NCConsole::Say, NCPawnViewrWnd::OnUseWeapon, python tools/port/packet.py). The
    record is C1's with: Name and NickName 0x10 bytes shorter (24 characters: the shift is -0x10 up to the speeds,
    -0x08 up to the nick name, -0x18 after it), two new speeds after WalkSpeed (+8) and some bytes at the end. The
    C1 offsets are in brackets.

    C3 layout (0x230 bytes; the C2 offsets are in the field comments as "C2 +0x..."): the Engine.dll of the C3
    client side by side with the C2 one. Fixed by one-instruction getters: GetNickColor 0xD8 -> 0xDC,
    GetGuiltyStatus 0xDC -> 0xE0, GetCriminalRate (Karma) 0xE0 -> 0xE4, Pawn 0x140 -> 0x150, GetNickName 0x188 ->
    0x198, GetPledgeID 0x1B8 -> 0x1C8, the party/pledge flags 0x1C8 -> 0x1D8, GetSurrenderWarID 0x1D0 -> 0x1E0,
    GetPrivateStoreState 0x1E0 -> 0x1F0; by the constructor (memset 0x230, then 0x120, 0x12C, 0x138, 0x144 cleared:
    the C2 had three of those 12-byte records, the C3 four; bytes 0x218-0x21A; 0x22C = -1); by the votes of the
    matched NWindow functions (Race ... MaxLoad unchanged). The bytes 0xD0..0xDC (+4 after the equipment array),
    0x1F2..0x200 (+0xC after the store type) and 0x208..0x230 (the tail) are new and unknown; the speeds and the
    fields between the two getter-proven ranges (0xE8..0x110, 0x154..0x194) are moved with their neighbours:
    tools/port/verify_data.py (stock displacements against ours) is what checks them.
-----------------------------------------------------------------------------*/

#pragma pack(push, 4)   // the MoveSpeedRate double sits at a 4-aligned offset
struct User
{
    BYTE _Unknown0[0x8];                // +0x00 the summoned creature's id (GetSummonedID), +0x04 the pet's (GetPetID)
    INT Unknown8;                       // +0x08 non-zero: not a player (invitations refuse it)
    INT Unknown0C;                      // +0x0C with Unknown8: a pet or summon (target window)
    INT Unknown10;                      // +0x10 with Unknown8, without Unknown0C: the target
                                        //   window shows its HP (monsters)
    INT NpcID;                          // +0x14 npcname id (NConsoleWnd::IsSiegeNpc)
    INT ObjectID;                       // +0x18 (attack and target requests)
    BYTE _Unknown1C[0x4C - 0x1C];       // +0x1C the name (User::GetName, SetName: TCHAR[24]; C1: [32])
    INT Race;                           // +0x4C (names ours) the pawn viewers set them with the (C1 +0x5C)
    INT Sex;                            // +0x50   class id (C1 +0x60)
    INT ClassID;                        // +0x54 (NConsoleWnd::GetClassNameString) (C1 +0x64)
    INT Level;                          // +0x58 (C1 +0x68)
    INT Exp;                            // +0x5C (C1 +0x6C)
    INT Str;                            // +0x60 the six stats (character status window) (C1 +0x70)
    INT Dex;                            // +0x64
    INT Con;                            // +0x68
    INT Int;                            // +0x6C
    INT Wit;                            // +0x70
    INT Men;                            // +0x74 (C1 +0x84)
    INT MaxHP;                          // +0x78 (C1 +0x88)
    INT HP;                             // +0x7C
    INT MaxMP;                          // +0x80
    INT MP;                             // +0x84
    INT MaxLoad;                        // +0x88 weight limit (C1 +0x98)
    BYTE _Unknown8C[0x94 - 0x8C];
    INT Equipped[15];                   // +0x94 (name ours) what it wears, by body part slot:
                                        //   object ids on the player's record (the inventory
                                        //   compares them), item ids on the viewers' records (C1 +0xA4)
    BYTE _UnknownD8[0xDC - 0xD8];
    BYTE _UnknownD0[0xE0 - 0xD0];       // +0xDC (C2 +0xD8) the nick name's color (GetNickColor), +0xDC the guilty status (GetGuiltyStatus)
    INT Karma;                          // +0xE4 (C2 +0xE0; GetCriminalRate; also reddens the name in the status window;
                                        //   shown beside "Karma" on the character selection) (C1 +0xF0)
    INT RunSpeed;                       // +0xE8 (C2 +0xE4) times the speed rate (+0x10C) (C1 +0xF4)
    INT WalkSpeed;                      // +0xEC (C2 +0xE8; C1 +0xF8)
    INT RunSpeedWater;                  // +0xF0 (C2 +0xEC; name ours; new in C2) NCDetailStatusWnd::OnPaint reads it instead of RunSpeed
                                        //   when the pawn's +0x62C is 1 (swimming, going by the use)
    INT WalkSpeedWater;                 // +0xF4 (C2 +0xF0; name ours; new in C2) the same for WalkSpeed
    BYTE _UnknownF8[0x110 - 0xF8];
    DOUBLE MoveSpeedRate;               // +0x110 (C2 +0x10C; name ours) a double in C2: the speed window multiplies both speeds
                                        //   by it (`fmul qword`); C1 had the pawn's float +0x698 for the player and a
                                        //   float at +0x118 here for the GM status window
    BYTE _Unknown114[0x140 - 0x114];    // +0x120 (C2 +0x11C), +0x128, +0x134: cleared by the constructor
    BYTE _Unknown144[0x150 - 0x144];
    APawn* Pawn;                        // +0x150 (C2 +0x140; C1 +0x148)
    INT DeleteDays;                     // +0x154 (C2 +0x144; name ours) days left until the character is
                                        //   deleted, 0 = not being deleted (C1 +0x14C; offset by the -8 of its neighbours)
    INT Load;                           // +0x158 (C2 +0x148) weight carried (C1 +0x150)
    INT SP;                             // +0x15C (C2 +0x14C; also on the character selection, unlabeled) (C1 +0x154)
    INT Accuracy;                       // +0x160 (C2 +0x150) combat values (character status window)
    INT Critical;                       // +0x164 (C2 +0x154)
    INT PAtk;                           // +0x168 (C2 +0x158)
    INT PAtkSpeed;                      // +0x16C (C2 +0x15C)
    INT PDef;                           // +0x170 (C2 +0x160)
    INT Evasion;                        // +0x174 (C2 +0x164)
    INT MAtk;                           // +0x178 (C2 +0x168)
    INT MDef;                           // +0x17C (C2 +0x16C)
    INT CastSpeed;                      // +0x180 (C2 +0x170; C1 +0x178)
    INT HairStyle;                      // +0x184 (C2 +0x174; names ours) the creation's choices, applied (C1 +0x17C; the
    INT HairColor;                      // +0x188 (C2 +0x178)   constructor clears +0x174 and +0x178)
    INT Face;                           // +0x18C (C2 +0x17C) by SetPawnResource (C1 +0x184)
    INT Unknown188;                     // +0x190 (C2 +0x180) set: the party loot rule cannot be changed
                                        //   (game options) (C1 +0x188)
    INT BuilderLevel;                   // +0x194 (C2 +0x184) GM rights, 0 = none (// and /// commands) (C1 +0x18C)
    BYTE _Unknown188[0x1B8 - 0x188];    // +0x198 (C2 +0x188) the nick name (User::GetNickName, SetNickName: TCHAR[24]; C1 +0x190, [32])
    INT PledgeID;                       // +0x1C8 (C2 +0x1B8; NConsoleWnd::GetPledgeInfo, crests) (C1 +0x1D0)
    BYTE _Unknown1CC[0x1D8 - 0x1CC];
    DWORD StatusFlags;                  // +0x1D8 (C2 +0x1C8; name ours) bit 6: clan leader (C1 +0x1E0)
    BYTE _Unknown1DC[0x1E0 - 0x1DC];
    INT Unknown1E8;                     // +0x1E0 (C2 +0x1D0) UNetHandler::IsSurrender compares it with its
                                        //   clan's +0xA8 (the console's attack cursor) (C1 +0x1E8)
    BYTE _Unknown1E4[0x1E8 - 0x1E4];
    INT bCanCrystallize;                // +0x1E8 (C2 +0x1D8; name ours) the inventory's crystallize icon (C1 +0x1F0)
    BYTE _Unknown1EC[0x1F0 - 0x1EC];
    SWORD PrivateStoreType;             // +0x1F0 (C2 +0x1E0; name ours) 1 selling, 3 buying (the console's
                                        //   talk cursor); set: no chat balloon over it (C1 +0x1F8)
    BYTE _Unknown1F2[0x200 - 0x1F2];
    INT PKCount;                        // +0x200 (C2 +0x1E4; names ours) shown as "PvP / PK" (C1 +0x1FC)
    INT PvPCount;                       // +0x204 (C2 +0x1E8; C1 +0x200)
    BYTE _Unknown208[0x230 - 0x208];    // +0x208 the tail: the constructor clears the bytes +0x218, +0x219, +0x21A (C2: +0x1EE,
                                        //   +0x1EF, +0x1F0, new in C2) and sets +0x22C to -1 (C2: a word at +0x1F6)

    ENGINE_API User();
    ENGINE_API TCHAR* GetName();
    ENGINE_API TCHAR* GetNickName();    // the title (NULL or empty: none)
    ENGINE_API INT GetMeshType();
    ENGINE_API TCHAR* GetClassNamePointer();
    ENGINE_API void SetPawnResource();
    ENGINE_API INT GetItemClassID(INT Slot);        // the item worn in a body part slot
};
NW_ASSERT_OFFSET(User, NpcID, 0x14);
NW_ASSERT_OFFSET(User, ObjectID, 0x18);
NW_ASSERT_OFFSET(User, Race, 0x4C);
NW_ASSERT_OFFSET(User, ClassID, 0x54);
NW_ASSERT_OFFSET(User, Level, 0x58);
NW_ASSERT_OFFSET(User, Str, 0x60);
NW_ASSERT_OFFSET(User, MaxHP, 0x78);
NW_ASSERT_OFFSET(User, MP, 0x84);
NW_ASSERT_OFFSET(User, MaxLoad, 0x88);
NW_ASSERT_OFFSET(User, Equipped, 0x94);
NW_ASSERT_OFFSET(User, Karma, 0xE4);
NW_ASSERT_OFFSET(User, RunSpeed, 0xE8);
NW_ASSERT_OFFSET(User, WalkSpeedWater, 0xF4);
NW_ASSERT_OFFSET(User, MoveSpeedRate, 0x110);
NW_ASSERT_OFFSET(User, Pawn, 0x150);
NW_ASSERT_OFFSET(User, Load, 0x158);
NW_ASSERT_OFFSET(User, CastSpeed, 0x180);
NW_ASSERT_OFFSET(User, HairStyle, 0x184);
NW_ASSERT_OFFSET(User, Face, 0x18C);
NW_ASSERT_OFFSET(User, Unknown188, 0x190);
NW_ASSERT_OFFSET(User, BuilderLevel, 0x194);
NW_ASSERT_OFFSET(User, PledgeID, 0x1C8);
NW_ASSERT_OFFSET(User, StatusFlags, 0x1D8);
NW_ASSERT_OFFSET(User, Unknown1E8, 0x1E0);
NW_ASSERT_OFFSET(User, bCanCrystallize, 0x1E8);
NW_ASSERT_OFFSET(User, PrivateStoreType, 0x1F0);
NW_ASSERT_OFFSET(User, PKCount, 0x200);
NW_ASSERT_OFFSET(User, PvPCount, 0x204);
NW_ASSERT_SIZE(User, 0x230);
#pragma pack(pop)

// What the network handler keeps about the player's pet or summon (Engine.dll "PetInfo":
// UNetworkHandler::GetPetInfo returns it by value, NConsoleWnd::GetPetInfo passes it on;
// 0xAC bytes in C2, 0xB4 in C1).
// C2 layout: from the copy a status window keeps (`rep movsd` of 0x2B dwords in NCPetStatusWnd::OnPaint) and the
// fields its paint reads (NCPetStatusWnd::OnPaint, NCSummonedStatusWnd::OnPaint, NCPetDetailStatusWnd::OnPaint,
// the status windows' constructors): the name is shorter by 0x10 bytes, so everything after it is at the C1
// offset minus 0x10 (in brackets), and two ints are new at the end.
struct PetInfo
{
    INT Type;                           // +0x00 2: pet (NCPetStatusWnd), 1: summon
                                        //   (NCSummonedStatusWnd); each window hides otherwise
    INT ObjectID;                       // +0x04 (the pet status window's constructor sets it to -1)
    TCHAR Name[(0x44 - 0x8) / 2];       // +0x08 (size: up to the next known field; C1: 0x54 - 0x8)
    INT HP;                             // +0x44 (C1 +0x54)
    INT MaxHP;                          // +0x48
    INT MP;                             // +0x4C
    INT MaxMP;                          // +0x50
    INT SP;                             // +0x54
    INT Level;                          // +0x58
    INT Exp;                            // +0x5C
    INT LevelExp;                       // +0x60 exp at the start of its level
    INT NextExp;                        // +0x64 exp of the next level (0: no exp bar)
    INT Fed;                            // +0x68 pet's food (the green bar) (C1 +0x78)
    INT MaxFed;                         // +0x6C
    INT Load;                           // +0x70 weight carried (pet window) (C1 +0x80; between MaxFed and PAtk)
    INT MaxLoad;                        // +0x74
    INT PAtk;                           // +0x78 combat values (pet and summon info tabs) (C1 +0x88)
    INT PDef;                           // +0x7C
    INT Accuracy;                       // +0x80
    INT Critical;                       // +0x84
    INT PAtkSpeed;                      // +0x88
    INT MAtk;                           // +0x8C
    INT MDef;                           // +0x90
    INT Evasion;                        // +0x94
    INT Speed;                          // +0x98
    INT CastSpeed;                      // +0x9C (C1 +0xAC)
    INT UnknownB0;                      // +0xA0 (the status windows' constructors clear it) (C1 +0xB0)
    INT UnknownA4;                      // +0xA4 new in C2
    INT UnknownA8;                      // +0xA8 new in C2 (the status windows' constructors clear it)
};
NW_ASSERT_OFFSET(PetInfo, ObjectID, 0x4);
NW_ASSERT_OFFSET(PetInfo, HP, 0x44);
NW_ASSERT_OFFSET(PetInfo, MaxMP, 0x50);
NW_ASSERT_OFFSET(PetInfo, Level, 0x58);
NW_ASSERT_OFFSET(PetInfo, Fed, 0x68);
NW_ASSERT_OFFSET(PetInfo, MaxFed, 0x6C);
NW_ASSERT_OFFSET(PetInfo, PAtk, 0x78);
NW_ASSERT_OFFSET(PetInfo, CastSpeed, 0x9C);
NW_ASSERT_OFFSET(PetInfo, UnknownB0, 0xA0);
NW_ASSERT_OFFSET(PetInfo, UnknownA8, 0xA8);
NW_ASSERT_SIZE(PetInfo, 0xAC);

// An item lying on the ground as the network handler keeps it (UNetHandler::GetItem, by its
// object id). Only the fields NWindow reads.
struct Item
{
    INT ObjectID;                       // +0x00 the server's id (the pet picks it up by it)
    INT ItemID;                         // +0x04 itemname id
    UBOOL bStackable;                   // +0x08 (names ours) set: its name shows the count
    INT Count;                          // +0x0C
};
NW_ASSERT_OFFSET(Item, ItemID, 0x4);
NW_ASSERT_OFFSET(Item, Count, 0xC);

// A door or another object the server placed (UNetHandler::GetStaticObject, by the actor's
// creature id; its constructor and assignment are exported, 0x10 bytes)
struct FServerStaticObject
{
    INT Unknown0;                       // +0x00 -1 when made
    INT StaticObjectID;                 // +0x04 (names ours) staticobject-*.dat id
                                        //   (FL2GameData::GetStaticObjectName)
    INT Unknown8;                       // +0x08
    AActor* Actor;                      // +0x0C (UGameEngine::OnMyTargetSelected targets it)
};
NW_ASSERT_SIZE(FServerStaticObject, 0x10);

// L2's console base (Engine.dll): C2 has 294 virtuals after USubsystem's (C1: 247), 275 of them pure
// (C1: 228). NConsoleWnd implements those; the declarations are NConsoleWnd's
// (tools/ue2_data/l2consolewnd_slots_c2.csv, first made by tools/ue2_console_slots.py from the C1 table and
// revised by hand with the evidence of each row in its `source` column: 236 slots have a name, the C1 one
// or, for the ones C2 added, the UGameEngine::On* handler's / the guard string's / the UL2NetHandler slot
// they forward to; the 39 left are PureSlotNNN, with what is known about them in the comment).
//
// C3 layout, read from the C3 Engine.dll's own code (no script class: tools/ue2_shift_layout.py has
// nothing to anchor to): the class is 0x392C bytes (C2: 0x3518, C1: 0x36B8) and a User is 0x230 bytes
// (C2: 0x1F8, C1: 0x208: `imul r, r, 0x230` in SelectedCharacterNum, `add ebx, 0x230` in
// ResetSelectCharacterInfo). Evidence: ResetSelectCharacterInfo walks the eight selection records from
// this+0x28FC (their Pawn at +0x150 of a User, so the array starts at +0x27AC) and writes -1 at +0x40;
// SpawnDefaultCharacter clears 18 records (the table it walks has 18 12-byte entries, in C2 too) from
// this+0x4C; every access through the Engine pointer uses +0x48 (the votes of tools/port/maps.py: C2 +0x44
// -> C3 +0x48, 194 of them). The 18 default records end exactly where the eight selection ones start, and
// those end at the class size: the 0x120-byte block C2 kept between them is gone.
class UL2ConsoleWnd : public USubsystem
{
    ENGINE_API static UClass PrivateStaticClass;   // private, as DECLARE_CLASS has it

public:
    static UClass* StaticClass() { return &PrivateStaticClass; }   // NConsoleWnd's parent class

    INT State;                          // +0x2C L2ConsoleState (UL2ConsoleWnd::GetState)
    INT SavedState;                     // +0x30 (name ours) while State is L2CS_Loading, the state
                                        //   SetState set (it only writes this one then)
    BYTE _Unknown34[0x48 - 0x34];       // +0x40: the selected index (ResetSelectCharacterInfo sets -1);
                                        //   +0x44 is new in C3
    UEngine* Engine;                    // +0x48 (C2 +0x44; UL2ConsoleWnd::PlayIntroMusic: Engine->Audio)
    User DefaultCharacterInfo[18];      // +0x4C (C2 +0x48; names ours, after LoadDefaultCharacterInfo and
                                        //   InsertSelectCharacterInfo) the creation stage's
                                        //   characters, Pawn their actor
    User SelectCharacterInfo[8];        // +0x27AC (C2 +0x24D8) the selection's (ResetSelectCharacterInfo
                                        //   empties the eight); the class ends with them

    ENGINE_API UL2ConsoleWnd();
    ENGINE_API virtual ~UL2ConsoleWnd();
    ENGINE_API void SetState(L2ConsoleState NewState);

    ENGINE_API virtual void Tick(FLOAT DeltaTime);                                   //  25 +0x064 (C1: 24)
#include "gen/UL2ConsoleWnd.vt.inc"
};
NW_ASSERT_OFFSET(UL2ConsoleWnd, State, 0x2C);
NW_ASSERT_OFFSET(UL2ConsoleWnd, SavedState, 0x30);
NW_ASSERT_OFFSET(UL2ConsoleWnd, Engine, 0x48);
NW_ASSERT_OFFSET(UL2ConsoleWnd, DefaultCharacterInfo, 0x4C);
NW_ASSERT_OFFSET(UL2ConsoleWnd, SelectCharacterInfo, 0x27AC);
NW_ASSERT_SIZE(UL2ConsoleWnd, 0x392C);   // C3 (C2: 0x3518, C1: 0x36B8)

// A class template the server sends before the character creation (Engine.dll
// "RecommandedDataStr", 0x50 bytes, passed by value to FL2NetNotify::AddRecommandedData; field
// names ours, laid out as the server's packet sends them: race, class, then 70 / value / 10 for
// each stat). NWindow reads only the values.
struct RecommandedDataStr
{
    INT Race;                           // +0x00
    INT ClassID;                        // +0x04
    struct
    {
        INT Max;
        INT Value;
        INT Min;
    } Stats[6];                         // +0x08 STR, DEX, CON, INT, WIT, MEN
};
NW_ASSERT_SIZE(RecommandedDataStr, 0x50);

/*-----------------------------------------------------------------------------
    FParser (L2 addition): splits a line into tokens at the separator characters. Only the
    size matters here (0x418: flags, position, length, token length, string, the token and
    the separators, 257 characters each).
-----------------------------------------------------------------------------*/

class FParser
{
public:
    ENGINE_API FParser();
    ENGINE_API ~FParser();
    ENGINE_API TCHAR* GetNextToken();
    ENGINE_API TCHAR* GetRemainStr();                 // new in C2: the rest of the line, from the current position (its copy
                                                      //   loop is in Engine.dll 0x10417B30; name from the export)
    ENGINE_API void SetSperator(TCHAR* Separators);   // sic
    ENGINE_API TCHAR* SetString(TCHAR* String);       // the first token

private:
    ENGINE_API TCHAR* GetToken();
    ENGINE_API UBOOL IsSeperator(TCHAR C);

    BYTE Data[0x418];
};
NW_ASSERT_SIZE(FParser, 0x418);
