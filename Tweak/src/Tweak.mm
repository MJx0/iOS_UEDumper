
#include <cstdio>
#include <string>
#include <fstream>
#include <utility>
#include <thread>
#include <vector>
#include <chrono>

#import <SSZipArchive/ZipArchive.h>

#import "Utils/AlertUtils.hpp"

#include "Dumper.hpp"

#include "UE/UEGameProfiles/Farlight.hpp"
#include "UE/UEGameProfiles/PES.hpp"
#include "UE/UEGameProfiles/Dislyte.hpp"
#include "UE/UEGameProfiles/Torchlight.hpp"
#include "UE/UEGameProfiles/MortalKombat.hpp"
#include "UE/UEGameProfiles/ArenaBreakout.hpp"
#include "UE/UEGameProfiles/BlackClover.hpp"
#include "UE/UEGameProfiles/DeltaForce.hpp"
#include "UE/UEGameProfiles/WutheringWaves.hpp"
#include "UE/UEGameProfiles/RealBoxing2.hpp"
#include "UE/UEGameProfiles/OdinValhalla.hpp"
#include "UE/UEGameProfiles/Injustice2.hpp"
#include "UE/UEGameProfiles/DeltaForce.hpp"
#include "UE/UEGameProfiles/RooftopsParkour.hpp"
#include "UE/UEGameProfiles/BabyYellow.hpp"
#include "UE/UEGameProfiles/TowerFantasy.hpp"
#include "UE/UEGameProfiles/BladeSoul.hpp"
#include "UE/UEGameProfiles/Lineage2.hpp"
#include "UE/UEGameProfiles/NightCrows.hpp"
#include "UE/UEGameProfiles/Case2.hpp"
#include "UE/UEGameProfiles/KingArthur.hpp"
#include "UE/UEGameProfiles/Century.hpp"
#include "UE/UEGameProfiles/HelloNeighbor.hpp"
#include "UE/UEGameProfiles/HelloNeighborND.hpp"
#include "UE/UEGameProfiles/SFG2.hpp"
#include "UE/UEGameProfiles/ArkUltimate.hpp"
#include "UE/UEGameProfiles/FFXIV.hpp"
#include "UE/UEGameProfiles/RLSideswipe.hpp"
#include "UE/UEGameProfiles/PUBG.hpp"
#include "UE/UEGameProfiles/HOK_World.hpp"

#define DUMP_DELAY_SEC 30
#define DUMP_FOLDER @"UEDump"

static std::vector<IGameProfile *> UE_Games = {
    new FarlightProfile(),
    new PESProfile(),
    new DislyteProfile(),
    new TorchlightProfile(),
    new MortalKombatProfile(),
    new ArenaBreakoutProfile(),
    new BlackCloverProfile(),
    new DeltaForceProfile(),
    new WutheringWavesProfile(),
    new RealBoxing2Profile(),
    new OdinValhallaProfile(),
    new Injustice2Profile(),
    new DeltaForceProfile(),
    new RooftopParkourProfile(),
    new BabyYellowProfile(),
    new TowerFantasyProfile(),
    new BladeSoulProfile(),
    new Lineage2Profile(),
    new Case2Profile(),
    new CenturyProfile(),
    new KingArthurProfile(),
    new NightCrowsProfile(),
    new HelloNeighborProfile(),
    new HelloNeighborNDProfile(),
    new SFG2Profile(),
    new ArkUltimateProfile(),
    new FFXIVProfile(),
    new RLSideswipeProfile(),
    new PUBGProfile(),
    new HokWorldProfile()
};

void dump_thread();

__attribute__((constructor)) static void onLoad()
{
    static dispatch_once_t once;
    dispatch_once(&once, ^{
      NSLog(@"======= I'm Loaded ========");
      std::thread(dump_thread).detach();
    });
}

void dump_thread()
{
    // wait for the application to finish initializing
    sleep(5);

    Alert::showInfo([NSString stringWithFormat:@"Dumping after %d seconds.", DUMP_DELAY_SEC], nil, DUMP_DELAY_SEC / 2.f);

    sleep(DUMP_DELAY_SEC);

    KittyAlertView *waitingAlert = Alert::showWaiting(@"Initializing Dumper...\n", nil);

    UEDumper uEDumper{};

    uEDumper.setDumpExeInfoNotify([&waitingAlert](bool bFinished)
    {
        if (!bFinished)
        {
            execOnUIThread(^() {
              [waitingAlert setTitle:@"Dumping Executable Info..." needsLayout:NO];
            });
        }
    });

    uEDumper.setDumpNamesInfoNotify([&waitingAlert](bool bFinished)
    {
        if (!bFinished)
        {
            execOnUIThread(^() {
              [waitingAlert setTitle:@"Dumping Names Info..." needsLayout:NO];
            });
        }
    });

    uEDumper.setDumpObjectsInfoNotify([&waitingAlert](bool bFinished)
    {
        if (!bFinished)
        {
            execOnUIThread(^() {
              [waitingAlert setTitle:@"Dumping Objects Info..." needsLayout:NO];
            });
        }
    });

    uEDumper.setDumpOffsetsInfoNotify([&waitingAlert](bool bFinished)
    {
        if (!bFinished)
        {
            execOnUIThread(^() {
              [waitingAlert setTitle:@"Dumping Offsets Info..." needsLayout:NO];
            });
        }
    });

    uEDumper.setObjectsProgressCallback([&waitingAlert](const SimpleProgressBar &progress)
    {
        static int lastPercent = -1;
        int currPercent = progress.getPercentage();
        if (lastPercent != currPercent)
        {
            lastPercent = currPercent;
            execOnUIThread(^() {
              [waitingAlert setTitle:[NSString stringWithFormat:@"Gathering UObjects %d%%", currPercent] needsLayout:NO];
            });
        }
    });

    uEDumper.setDumpProgressCallback([&waitingAlert](const SimpleProgressBar &progress)
    {
        static int lastPercent = -1;
        int currPercent = progress.getPercentage();
        if (lastPercent != currPercent)
        {
            lastPercent = currPercent;
            execOnUIThread(^() {
              [waitingAlert setTitle:[NSString stringWithFormat:@"Dumping %d%%", currPercent] needsLayout:NO];
            });
        }
    });

    bool dumpSuccess = false;
    std::unordered_map<std::string, BufferFmt> dumpbuffersMap;
    auto dmpStart = std::chrono::steady_clock::now();

    NSString *appID = [[[NSBundle mainBundle] infoDictionary] objectForKey:(id)kCFBundleIdentifierKey];

    for (auto &it : UE_Games)
    {
        for (auto &pkg : it->GetAppIDs())
        {
            if (pkg.compare(appID.UTF8String) == 0)
            {
                if (uEDumper.Init(it))
                {
                    dumpSuccess = uEDumper.Dump(&dumpbuffersMap);
                }
                goto done;
            }
        }
    }

done:

    if (!dumpSuccess && uEDumper.GetLastError().empty())
    {
        Alert::dismiss(waitingAlert);
        Alert::showError(@"Not Supported, Check AppID", nil);
        return;
    }

    if (dumpbuffersMap.empty())
    {
        Alert::dismiss(waitingAlert);
        Alert::showError(@"Dump Failed", [NSString stringWithFormat:@"Error <Buffers empty>.\nStatus <%s>", uEDumper.GetLastError().c_str()], nil);
        return;
    }

    execOnUIThread(^() {
      [waitingAlert setTitle:@"Saving Files..." needsLayout:YES];
    });

    NSString *docDir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];

    std::string appName = IOUtils::remove_specials(uEDumper.GetProfile()->GetAppName());

    NSString *dumpPath = [NSString stringWithFormat:@"%@/%s_%@", docDir, appName.c_str(), DUMP_FOLDER];
    NSString *zipdumpPath = [NSString stringWithFormat:@"%@.zip", dumpPath];

    NSFileManager *fileManager = [NSFileManager defaultManager];

    if ([fileManager fileExistsAtPath:dumpPath])
    {
        [fileManager removeItemAtPath:dumpPath error:nil];
    }

    if ([fileManager fileExistsAtPath:zipdumpPath])
    {
        [fileManager removeItemAtPath:zipdumpPath error:nil];
    }

    NSError *error = nil;
    if (![fileManager createDirectoryAtPath:dumpPath withIntermediateDirectories:YES attributes:nil error:&error])
    {
        Alert::dismiss(waitingAlert);
        NSLog(@"Failed to create folders\nError: %@", error);
        Alert::showError(@"Failed to create folders", [NSString stringWithFormat:@"Error: %@", error]);
        return;
    }

    for (const auto &it : dumpbuffersMap)
    {
        if (!it.first.empty())
        {
            NSString *path = [NSString stringWithFormat:@"%@/%s", dumpPath, it.first.c_str()];
            it.second.writeBufferToFile(path.UTF8String);
        }
    }

    if ([SSZipArchive createZipFileAtPath:zipdumpPath withContentsOfDirectory:dumpPath] == YES)
    {
        [fileManager removeItemAtPath:dumpPath error:nil];
    }
    else
    {
        Alert::dismiss(waitingAlert);
        Alert::showError(@"Failed to zip dump folder", [NSString stringWithFormat:@"Folder: %@", dumpPath]);
        return;
    }

    auto dmpEnd = std::chrono::steady_clock::now();
    std::chrono::duration<float, std::milli> dmpDurationMS = (dmpEnd - dmpStart);

    Alert::dismiss(waitingAlert);

    ui_action_block_t shareAction = ^() {
      Alert::showNoOrYes(@"Share Dump", @"Do you want to share/transfer dump ZIP file?", nil, ^() {
        NSURL *zipFileURL = [NSURL fileURLWithPath:zipdumpPath];
        NSArray *activityItems = @[ zipFileURL ];
        UIActivityViewController *activityVC = [[UIActivityViewController alloc] initWithActivityItems:activityItems applicationActivities:nil];

        auto mainVC = GetTopViewController();
        if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad)
        {
            activityVC.popoverPresentationController.sourceView = mainVC.view;
            activityVC.popoverPresentationController.sourceRect = CGRectMake(mainVC.view.bounds.size.width / 2, mainVC.view.bounds.size.height / 2, 1, 1);
        }
        [mainVC presentViewController:activityVC animated:YES completion:nil];
      });
    };

    if (dumpSuccess)
    {
        Alert::showSuccess(@"Dump Succeeded", [NSString stringWithFormat:@"Duration: %.2fms\nPath:\n%@", dmpDurationMS.count(), zipdumpPath], shareAction);
    }
    else
    {
        Alert::showError(@"Dump Failed", [NSString stringWithFormat:@"Error <%s>.\nDuration: %.2fms\nDump Path:\n%@", uEDumper.GetLastError().c_str(), dmpDurationMS.count(), zipdumpPath], shareAction);
    }
}
