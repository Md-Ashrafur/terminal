﻿#include "pch.h"
#include <winrt/ScratchWinRTServer.h>
#include "../../types/inc/utils.hpp"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace ::Microsoft::Console;

// DAA16D7F-EF66-4FC9-B6F2-3E5B6D924576
static constexpr GUID MyStringable_clsid{
    0xdaa16d7f,
    0xef66,
    0x4fc9,
    { 0xb6, 0xf2, 0x3e, 0x5b, 0x6d, 0x92, 0x45, 0x76 }
};

// EAA16D7F-EF66-4FC9-B6F2-3E5B6D924576
static constexpr GUID ScratchStringable_clsid{
    0xeaa16d7f,
    0xef66,
    0x4fc9,
    { 0xb6, 0xf2, 0x3e, 0x5b, 0x6d, 0x92, 0x45, 0x76 }
};

// FAA16D7F-EF66-4FC9-B6F2-3E5B6D924576
static constexpr GUID ScratchClass_clsid{
    0xfaa16d7f,
    0xef66,
    0x4fc9,
    { 0xb6, 0xf2, 0x3e, 0x5b, 0x6d, 0x92, 0x45, 0x76 }
};

void createHostClassProcess(const winrt::guid& g)
{
    auto guidStr{ Utils::GuidToString(g) };
    std::wstring commandline{ fmt::format(L"ScratchWinRTServer.exe {}", guidStr) };
    STARTUPINFO siOne{ 0 };
    siOne.cb = sizeof(STARTUPINFOW);
    wil::unique_process_information piOne;
    auto succeeded = CreateProcessW(
        nullptr,
        commandline.data(),
        nullptr, // lpProcessAttributes
        nullptr, // lpThreadAttributes
        false, // bInheritHandles
        CREATE_UNICODE_ENVIRONMENT, // dwCreationFlags
        nullptr, // lpEnvironment
        nullptr, // startingDirectory
        &siOne, // lpStartupInfo
        &piOne // lpProcessInformation
    );
    if (!succeeded)
    {
        printf("Failed to create first host process\n");
        return;
    }

    // Ooof this is dumb, but we need a sleep here to make the server starts.
    // That's _sub par_. Maybe we could use the host's stdout to have them emit
    // a byte when they're set up?
    Sleep(50);
}

void scratchApp()
{
    printf("scratchApp\n");
    // {
    //     printf("Trying to directly create a ScratchClass...\n");
    //     auto server = create_instance<winrt::ScratchWinRTServer::ScratchClass>(ScratchClass_clsid, CLSCTX_LOCAL_SERVER);
    //     if (server)
    //     {
    //         printf("%ls\n", server.DoTheThing().c_str());
    //     }
    //     else
    //     {
    //         printf("Could not get the ScratchClass directly\n");
    //     }
    // }

    // 1. Generate a GUID.
    // 2. Spawn a Server.exe, with the guid on the commandline
    // 3. Make an instance of that GUID, as a HostClass
    // 4. Call HostClass::DoTheThing, and get the count with HostClass::DoCount [1]
    // 5. Make another instance of HostClass, and get the count with HostClass::DoCount. It should be the same. [1, 1]
    // 6. On the second HostClass, call DoTheThing. Verify that both instances have the same DoCount. [2. 2]
    // 7. Create a second Server.exe, and create a Third HostClass, using that GUID.
    // 8. Call DoTheThing on the third, and verify the counts of all three instances. [2, 2, 1]
    // 9. QUESTION: Does releasing the first instance leave the first object alive, since the second instance still points at it?

    // 1. Generate a GUID.
    winrt::guid firstGuid{ Utils::CreateGuid() };

    // 2. Spawn a Server.exe, with the guid on the commandline
    createHostClassProcess(firstGuid);

    // 3. Make an instance of that GUID, as a HostClass
    printf("Trying to directly create a HostClass...\n");
    auto firstHost = create_instance<winrt::ScratchWinRTServer::HostClass>(firstGuid, CLSCTX_LOCAL_SERVER);

    if (!firstHost)
    {
        printf("Could not get the first HostClass\n");
        return;
    }

    printf("DoCount: %d (Expected: 0)\n",
           firstHost.DoCount());
    // 4. Call HostClass::DoTheThing, and get the count with HostClass::DoCount [1]
    firstHost.DoTheThing();
    printf("DoCount: %d (Expected: 1)\n",
           firstHost.DoCount());

    // 5. Make another instance of HostClass, and get the count with HostClass::DoCount. It should be the same. [1, 1]
    auto secondHost = create_instance<winrt::ScratchWinRTServer::HostClass>(firstGuid, CLSCTX_LOCAL_SERVER);
    if (!secondHost)
    {
        printf("Could not get the second HostClass\n");
        return;
    }
    printf("DoCount: [%d, %d] (Expected: [1, 1])\n",
           firstHost.DoCount(),
           secondHost.DoCount());

    // 6. On the second HostClass, call DoTheThing. Verify that both instances have the same DoCount. [2. 2]
    secondHost.DoTheThing();
    printf("DoCount: [%d, %d] (Expected: [2, 2])\n",
           firstHost.DoCount(),
           secondHost.DoCount());

    // 7. Create a second Server.exe, and create a Third HostClass, using that GUID.
    winrt::guid secondGuid{ Utils::CreateGuid() };
    createHostClassProcess(secondGuid);
    auto thirdHost = create_instance<winrt::ScratchWinRTServer::HostClass>(secondGuid, CLSCTX_LOCAL_SERVER);
    if (!thirdHost)
    {
        printf("Could not get the third HostClass\n");
        return;
    }
    printf("DoCount: [%d, %d, %d] (Expected: [2, 2, 0])\n",
           firstHost.DoCount(),
           secondHost.DoCount(),
           thirdHost.DoCount());
    // 8. Call DoTheThing on the third, and verify the counts of all three instances. [2, 2, 1]
    thirdHost.DoTheThing();
    printf("DoCount: [%d, %d, %d] (Expected: [2, 2, 1])\n",
           firstHost.DoCount(),
           secondHost.DoCount(),
           thirdHost.DoCount());
}

int main()
{
    auto hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    init_apartment();

    try
    {
        scratchApp();
    }
    catch (hresult_error const& e)
    {
        printf("Error: %ls\n", e.message().c_str());
    }

    printf("Press Enter me when you're done.");
    getchar();
    printf("Exiting client");
}
