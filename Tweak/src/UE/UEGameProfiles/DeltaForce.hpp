#pragma once

#include "../UEGameProfile.hpp"
using namespace UEMemory;

// DeltaForceProfile
// UE 4.27

class DeltaForceProfile : public IGameProfile
{
public:
    DeltaForceProfile() = default;
    
    std::string GetAppName() const override
    {
        return "Delta Force";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return {"com.proxima.dfm", "com.garena.game.df"};
    }

    bool isUsingCasePreservingName() const override
    {
        return false;
    }

    bool IsUsingFNamePool() const override
    {
        return true;
    }

    bool isUsingOutlineNumberName() const override
    {
        return false;
    }

    uintptr_t GetGUObjectArrayPtr() const override
    {
        std::vector<std::pair<std::string, int>> idaPatterns = {
            {"29 3D 10 12 08 01 09 4B ? ? ? ? ? ? ? ? 29 11 40 F9 29 D9 6A F8 0A 03 80 52", 8}, // New
            // FUObjectArray::FUObjectArray();
            {"94 E0 23 00 91 ? ? ? 94 08 7D 80 52", 0x11},
            {"00 E6 07 2F 00 00 00 FD 1F 08 00 B9 ? ? 80 52", 0xB4},
            {"00 E6 07 2F 00 00 00 FD 1F 08 00 B9 ? ? 80 52", 0x74},

            // FEngineLoop::PreInitPostStartupScreen
            {"A8 99 99 52 88 B9 A7 72 ? ? ? 1E ? ? 2C 1E E0 00 80 52", 0x34},

            {"40 20 1e ? 03 ? aa ? 03 ? aa ? ? 40 b9 ? ? ? ? ? ? ? 91", 0xF},

            {"40 B9 ? ? 40 39 ? ? ? ? ? ? ? 91 ? ? ? 97 ? ? ? 97 ? ? ? 97", 6},

            {"FF 9F 52 ? ? ? ? ? ? ? 91 ? 03 80 52 ? ? 40 F9", 3},
            {"80 52 ? FF 9F 52 ? ? ? ? ? ? ? 91 ? 03 80 52", 6},
        };

        auto text_seg = GetExecutableInfo().getSection("__TEXT", "__text");

        for (const auto &it : idaPatterns)
        {
            std::string ida_pattern = it.first;
            const int step = it.second;

            uintptr_t ins = KittyScanner::findIdaPatternFirst(text_seg.start, text_seg.end, ida_pattern);
            if (ins != 0)
            {
                uintptr_t adrl = Arm64::DecodeADRL(ins + step);
                if (adrl != 0) return adrl;
            }
        }

        return 0;
    }

    uintptr_t GetNamesPtr() const override
    {
        std::string ida_pattern = "4F D3 ? ? 7D 92 ? ? ? ? ? ? ? 91";
        const int step = 6;

        auto text_seg = GetExecutableInfo().getSection("__TEXT", "__text");
        uintptr_t ins = KittyScanner::findIdaPatternFirst(text_seg.start, text_seg.end, ida_pattern);
        if (ins != 0)
        {
            return Arm64::DecodeADRL(ins + step);
        }
        
        return 0;
    }

    UE_Offsets *GetOffsets() const override
    {
        static UE_Offsets offsets = UE_DefaultOffsets::UE4_25_27(isUsingCasePreservingName());

        static bool once = false;
        if (!once)
        {
            once = true;

            offsets.FNamePool.BlocksBit = 18;
            offsets.FNamePool.BlocksOff -= sizeof(void*);

            offsets.TUObjectArray.NumElements = sizeof(int32_t);
            offsets.TUObjectArray.Objects = offsets.TUObjectArray.NumElements + (sizeof(int32_t) * 3);

            offsets.UObject.ClassPrivate = sizeof(void *);
            offsets.UObject.OuterPrivate = offsets.UObject.ClassPrivate + sizeof(void *);
            offsets.UObject.ObjectFlags = offsets.UObject.OuterPrivate + sizeof(void *);
            offsets.UObject.NamePrivate = offsets.UObject.ObjectFlags + sizeof(int32_t);
            offsets.UObject.InternalIndex = offsets.UObject.NamePrivate + offsets.FName.Size;

            offsets.UStruct.PropertiesSize = offsets.UField.Next + (sizeof(void *) * 2) + sizeof(int32_t);
            offsets.UStruct.SuperStruct = offsets.UStruct.PropertiesSize + sizeof(int32_t);
            offsets.UStruct.Children = offsets.UStruct.SuperStruct + (sizeof(void *) * 2);
            offsets.UStruct.ChildProperties = offsets.UStruct.Children + (sizeof(void *) * 3);

            offsets.UFunction.NumParams = offsets.UStruct.ChildProperties + ((sizeof(void *) + sizeof(int32_t) * 2) * 2) + (sizeof(void *) * 5);
            offsets.UFunction.ParamSize = offsets.UFunction.NumParams + sizeof(int16_t);
            offsets.UFunction.EFunctionFlags = offsets.UFunction.ParamSize + sizeof(int16_t) + sizeof(int32_t);
            offsets.UFunction.Func = offsets.UFunction.EFunctionFlags + (sizeof(int32_t) * 2) + (sizeof(void *) * 3);

            offsets.FField.FlagsPrivate = sizeof(void *);
            offsets.FField.Next = offsets.FField.FlagsPrivate + (sizeof(void *) * 2);
            offsets.FField.ClassPrivate = offsets.FField.Next + sizeof(void *);
            offsets.FField.NamePrivate = offsets.FField.ClassPrivate + sizeof(void *);

            offsets.FProperty.ArrayDim = offsets.FField.NamePrivate + GetPtrAlignedOf(offsets.FName.Size) + sizeof(void *);
            offsets.FProperty.ElementSize = offsets.FProperty.ArrayDim + sizeof(int32_t);
            offsets.FProperty.PropertyFlags = offsets.FProperty.ElementSize + sizeof(int32_t);
            offsets.FProperty.Offset_Internal = offsets.FProperty.PropertyFlags + sizeof(int64_t) + sizeof(int32_t);
            offsets.FProperty.Size = offsets.FProperty.Offset_Internal + (sizeof(int32_t) * 3) + (sizeof(void *) * 6);

            offsets.FProperty.SubSetElement     = offsets.FProperty.Size - sizeof(void *);
            offsets.FProperty.SubMapKey         = offsets.FProperty.Size;
            offsets.FProperty.SubMapValue       = offsets.FProperty.Size + sizeof(void *);
            offsets.FProperty.SubEnumUnderlying = offsets.FProperty.Size - sizeof(void *);
            offsets.FProperty.SubEnumEnum       = offsets.FProperty.Size;
            offsets.FProperty.SubFieldPathClass = offsets.FProperty.Size - sizeof(void *);
        }

        return &offsets;
    }

    std::string GetNameEntryString(uint8_t *entry) const override
    {
        std::string name = IGameProfile::GetNameEntryString(entry);

        auto dec_ansi = [](char *str, uint32_t len)
        {
            if (!str || !*str || len == 0) return;

            uint32_t key = 0;
            switch (len % 9)
            {
            case 0u:
                key = ((len & 0x1F) + len);
                break;
            case 1u:
                key = ((len ^ 0xDF) + len);
                break;
            case 2u:
                key = ((len | 0xCF) + len);
                break;
            case 3u:
                key = (33 * len);
                break;
            case 4u:
                key = (len + (len >> 2));
                break;
            case 5u:
                key = (3 * len + 5);
                break;
            case 6u:
                key = (((4 * len) | 5) + len);
                break;
            case 7u:
                key = (((len >> 4) | 7) + len);
                break;
            case 8u:
                key = ((len ^ 0xC) + len);
                break;
            default:
                key = ((len ^ 0x40) + len);
                break;
            }

            for (uint32_t i = 0; i < len; i++)
            {
                str[i] = (key & 0x80) ^ ~str[i];
            }
        };

        dec_ansi(name.data(), uint32_t(name.length()));

        /*static char *(*pdec_ansi)(char *result, unsigned int len);
        if (!pdec_ansi) kSET_FUNC_PTR(pdec_ansi, GetExecutableInfo().address + 0x1062442F8);

        // static char* (*pdec_wide)(wchar_t* result, unsigned int len);
        // if (!pdec_wide) kSET_FUNC_PTR(pdec_wide, base+0x106244520);

        pdec_name(name.data(), int(name.length()));*/

        return name;
    }
};
