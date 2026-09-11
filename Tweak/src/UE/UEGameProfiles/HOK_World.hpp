#pragma once

#include "../UEGameProfile.hpp"
using namespace UEMemory;

// HOK: World
// UE 4.26

class HokWorldProfile : public IGameProfile
{
public:
    HokWorldProfile() = default;
    
    std::string GetAppName() const override
    {
        return "HOK: World";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return {"com.tencent.ngr"};
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
            {"? ? ? ? 29 79 45 F9 29 D9 6A F8 0A 03 80 52 08 25 2A 9B 08 2D 40 39", 0},
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
        std::vector<std::pair<std::string, int>> idaPatterns = {
            {"? ? ? ? 29 01 15 91 28 21 08 8B", 0},
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

    UE_Offsets *GetOffsets() const override
    {
        static UE_Offsets offsets = UE_DefaultOffsets::UE4_25_27(isUsingCasePreservingName());

        static bool once = false;
        if (!once)
        {
            once = true;
            offsets.FNamePool.BlocksOff += sizeof(void *);
            offsets.FUObjectArray.ObjObjects = 0x0;
        }

        return &offsets;
    }
};
