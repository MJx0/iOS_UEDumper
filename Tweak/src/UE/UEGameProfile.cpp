#include "UEGameProfile.hpp"

#include "UEWrappers.hpp"

using namespace UEMemory;

UEVarsInitStatus IGameProfile::InitUEVars()
{
    kPtrValidator.setUseCache(true);
    kPtrValidator.refreshRegionCache();
    if (kPtrValidator.regions().empty())
        return UEVarsInitStatus::ERROR_INIT_PTR_VALIDATOR;

    _UEVars.BaseAddress = GetExecutableInfo().address;
    if (_UEVars.BaseAddress == 0)
        return UEVarsInitStatus::ERROR_EXE_NOT_FOUND;

    UE_Offsets *pOffsets = GetOffsets();
    if (!pOffsets)
        return UEVarsInitStatus::ERROR_INIT_OFFSETS;

    _UEVars.Offsets = pOffsets;

    _UEVars.NamesPtr = GetNamesPtr();
    if (IsUsingFNamePool())
    {
        if (!kPtrValidator.isPtrReadable(_UEVars.NamesPtr))
            return UEVarsInitStatus::ERROR_INIT_NAMEPOOL;
    }
    else
    {
        if (!kPtrValidator.isPtrReadable(_UEVars.NamesPtr))
            return UEVarsInitStatus::ERROR_INIT_GNAMES;
    }

    _UEVars.pGetNameByID = [this](int32_t id) -> std::string
    {
        return GetNameByID(id);
    };

    _UEVars.GUObjectsArrayPtr = GetGUObjectArrayPtr();
    if (!kPtrValidator.isPtrReadable(_UEVars.GUObjectsArrayPtr))
        return UEVarsInitStatus::ERROR_INIT_GUOBJECTARRAY;

    _UEVars.ObjObjectsPtr = _UEVars.GUObjectsArrayPtr + pOffsets->FUObjectArray.ObjObjects;

    if (!vm_rpm_ptr((void *)(_UEVars.ObjObjectsPtr + pOffsets->TUObjectArray.Objects), &_UEVars.ObjObjects_Objects, sizeof(uintptr_t)))
        return UEVarsInitStatus::ERROR_INIT_OBJOBJECTS;

    UEWrappers::Init(GetUEVars());

    return UEVarsInitStatus::SUCCESS;
}

uint8_t *IGameProfile::GetNameEntry(int32_t id) const
{
    if (id < 0) return nullptr;

    uintptr_t namesPtr = _UEVars.GetNamesPtr();
    if (namesPtr == 0) return nullptr;

    if (!IsUsingFNamePool())
    {
        static uintptr_t gNames = 0;
        if (gNames == 0)
        {
            gNames = vm_rpm_ptr<uintptr_t>((void *)namesPtr);
        }

        const int32_t ElementsPerChunk = 16384;
        const int32_t ChunkIndex = id / ElementsPerChunk;
        const int32_t WithinChunkIndex = id % ElementsPerChunk;

        // FNameEntry**
        uint8_t *FNameEntryArray = vm_rpm_ptr<uint8_t *>((void *)(gNames + ChunkIndex * sizeof(uintptr_t)));
        if (!FNameEntryArray) return nullptr;

        // FNameEntry*
        return vm_rpm_ptr<uint8_t *>(FNameEntryArray + WithinChunkIndex * sizeof(uintptr_t));
    }

    uintptr_t blockBit = GetOffsets()->FNamePool.BlocksBit;
    uintptr_t blocks = GetOffsets()->FNamePool.BlocksOff;
    uintptr_t chunckMask = (1 << blockBit) - 1;
    uintptr_t stride = GetOffsets()->FNamePool.Stride;

    uintptr_t block_offset = ((id >> blockBit) * sizeof(void *));
    uintptr_t chunck_offset = ((id & chunckMask) * stride);

    uint8_t *chunck = vm_rpm_ptr<uint8_t *>((void *)(namesPtr + blocks + block_offset));
    if (!chunck) return nullptr;

    return (chunck + chunck_offset);
}

std::string IGameProfile::GetNameEntryString(uint8_t *entry) const
{
    if (!entry) return "";

    UE_Offsets *offsets = GetOffsets();

    uint8_t *pStr = nullptr;
    // don't care for now
    // bool isWide = false;
    size_t strLen = 0;
    int strNumber = 0;

    if (!IsUsingFNamePool())
    {
        int32_t name_index = 0;
        if (!vm_rpm_ptr(entry + offsets->FNameEntry.Index, &name_index, sizeof(int32_t))) return "";

        pStr = entry + offsets->FNameEntry.Name;
        // isWide = offsets->FNameEntry.GetIsWide(name_index)
        strLen = kMAX_UENAME_BUFFER;
    }
    else
    {
        uint16_t header = 0;
        if (!vm_rpm_ptr(entry + offsets->FNamePoolEntry.Header, &header, sizeof(int16_t)))
            return "";

        if (isUsingOutlineNumberName() && offsets->FNamePoolEntry.GetLength(header) == 0)
        {
            const uintptr_t stringOff = offsets->FNamePoolEntry.Header + sizeof(int16_t);
            const uintptr_t entryIdOff = stringOff + ((stringOff == 6) * 2);
            const int32_t nextEntryId = vm_rpm_ptr<int32_t>(entry + entryIdOff);
            if (nextEntryId <= 0) return "";

            strNumber = vm_rpm_ptr<int32_t>(entry + entryIdOff + sizeof(int32_t));
            entry = GetNameEntry(nextEntryId);
            if (!vm_rpm_ptr(entry + offsets->FNamePoolEntry.Header, &header, sizeof(int16_t)))
                return "";
        }

        strLen = std::min<size_t>(offsets->FNamePoolEntry.GetLength(header), kMAX_UENAME_BUFFER);
        if (strLen <= 0) return "";

        // isWide = offsets->FNamePoolEntry.GetIsWide(header);
        pStr = entry + offsets->FNamePoolEntry.Header + sizeof(int16_t);
    }

    std::string result = vm_rpm_str(pStr, strLen);

    if (strNumber > 0)
        result += '_' + std::to_string(strNumber - 1);

    return result;
}

std::string IGameProfile::GetNameByID(int32_t id) const
{
    return GetNameEntryString(GetNameEntry(id));
}

std::vector<std::string> IGameProfile::GetExcludedObjects() const
{
    // full name
    /*return {
        "ScriptStruct CoreUObject.Vector",
        "ScriptStruct CoreUObject.Vector2D"
    };*/
    return {};
}

std::string IGameProfile::GetUserTypesHeader() const
{
    return R"(#pragma once
    
#include <cstdio>
#include <string>
#include <cstdint>

template <class T>
class TArray
{
protected:
    T *Data;
    int32_t NumElements;
    int32_t MaxElements;

public:
    TArray(const TArray &) = default;
    TArray(TArray &&) = default;

    inline TArray() : Data(nullptr), NumElements(0), MaxElements(0) {}
    inline TArray(int size) : NumElements(0), MaxElements(size), Data(reinterpret_cast<T *>(calloc(1, sizeof(T) * size))) {}

    TArray &operator=(TArray &&) = default;
    TArray &operator=(const TArray &) = default;

    inline T &operator[](int i) { return (IsValid() && IsValidIndex(i)) ? Data[i] : T(); };
    inline const T &operator[](int i) const { (IsValid() && IsValidIndex(i)) ? Data[i] : T(); }

    inline explicit operator bool() const { return IsValid(); };

    inline bool IsValid() const { return Data != nullptr; }
    inline bool IsValidIndex(int index) const { return index >= 0 && index < NumElements; }

    inline int Slack() const { return MaxElements - NumElements; }

    inline int Num() const { return NumElements; }
    inline int Max() const { return MaxElements; }

    inline T *GetData() const { return Data; }
    inline T *GetDataAt(int index) const { return Data + index; }

    inline bool Add(const T &element)
    {
        if (Slack() <= 0) return false;

        Data[NumElements] = element;
        NumElements++;
        return true;
    }

    inline bool RemoveAt(int index)
    {
        if (!IsValidIndex(index)) return false;

        NumElements--;

        for (int i = index; i < NumElements; i++)
        {
            Data[i] = Data[i + 1];
        }

        return true;
    }

    inline void Clear()
    {
        NumElements = 0;
        if (Data) memset(Data, 0, sizeof(T) * MaxElements);
    }
};

class FString : public TArray<wchar_t>
{
public:
    FString() = default;
    inline FString(const wchar_t *wstr)
    {
        MaxElements = NumElements = (wstr && *wstr) ? int32_t(std::wcslen(wstr)) + 1 : 0;
        if (NumElements) Data = const_cast<wchar_t *>(wstr);
    }

    inline FString operator=(const wchar_t *&&other) { return FString(other); }

    inline std::wstring ToWString() const { return IsValid() ? Data : L""; }
};

template <typename KeyType, typename ValueType>
class TPair
{
private:
    KeyType First;
    ValueType Second;

public:
    TPair() = default;
    inline TPair(KeyType Key, ValueType Value) : First(Key), Second(Value) {}

    inline KeyType &Key() { return First; }
    inline const KeyType &Key() const { return First; }
    inline ValueType &Value() { return Second; }
    inline const ValueType &Value() const { return Second; }
};)";
}
