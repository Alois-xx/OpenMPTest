#include <stdio.h>
#include <windows.h>
#include <chrono>
#include <thread>
#include <string>

using picoseconds = std::chrono::duration<long long, std::pico>;

class Stopwatch
{
public:
    Stopwatch()
    {
        _Start = std::chrono::high_resolution_clock::now();
    }

    void Start()
    {
        _Start = std::chrono::high_resolution_clock::now();
    }

    std::chrono::milliseconds Stop()
    {
        _Stop = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(_Stop - _Start);
    }

    std::chrono::nanoseconds StopNs()
    {
        _Stop = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(_Stop - _Start);
    }

    picoseconds StopPico()
    {
        _Stop = std::chrono::high_resolution_clock::now();
        return picoseconds{ _Stop - _Start };
    }

private:
    std::chrono::high_resolution_clock::time_point _Start;
    std::chrono::high_resolution_clock::time_point _Stop;
};

void MeasurePauseLatency()
{
    const int PauseLoopCount = 10 * 1000 * 1000;

    BOOL lret = ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    if (lret == 0)
    {
        wprintf(L"\nSetThreadPriority failed with %d", ::GetLastError());
        return;
    }

    auto cpuCount = min(std::thread::hardware_concurrency(), 64u);
    for (unsigned int i = 0; i < cpuCount; i++)
    {
        DWORD_PTR mask = static_cast<DWORD_PTR>(1) << i;
        auto afflret = ::SetThreadAffinityMask(::GetCurrentThread(), mask);
        if (afflret == 0)
        {
            wprintf(L"\nSetThreadAffinityMask did fail with %d", ::GetLastError());
            break;
        }


        int value = (int)time(nullptr);
        Stopwatch emptyLoop;
        for (int k = 0; k < PauseLoopCount; k++)
        {
            if (value == 1)
            {
                YieldProcessor();
            }
            else
            {
                value = 2;
            }
        }

        auto emtpyLoopTimePico = emptyLoop.StopPico();

        Stopwatch sw;
        for (int k = 0; k < PauseLoopCount; k++)
        {
            YieldProcessor();
        }
        auto pico = sw.StopPico();
        printf("Pause Latency CPU[%d]: %.1lf ns\n",i, ( ( pico.count() - emtpyLoopTimePico.count() )/1000.0 ) / (double) PauseLoopCount);

    }
}

void MeasureOpenMPPerformance()
{
    const int N = 10000;
    double* pValues = new double[N];
    for (int i = 0; i < N; i++)  // generate meaningful values which are in range 1.0 - 2.0
    {
        pValues[i] = 1.0f + i / N;
    }

    Stopwatch swParallel;

    const int OuterLoopCount = 500 * 1000;

    for (int k = 0; k < OuterLoopCount; k++)
    {
#pragma omp parallel for schedule(static)
        for (int i = 1; i < N; i++)
        {
            pValues[i] = 10.0 * pValues[i]; // simulate some parallel work
            pValues[i]++;
            pValues[i] /= 15.0;
        }
    }
    auto ms = swParallel.Stop();
    printf("OpenMP loop with %d outer and %d parallel inner loop. Elapsed time: %.1f s\n", OuterLoopCount, N, ms.count() / 1000.0);
}

void MeasureSmallWork()
{
    int ncpus = std::thread::hardware_concurrency();
    int Repeat = 25;
    Stopwatch swParallel;
    for (int k = 0; k < Repeat; k++)
    {
#pragma omp parallel
        for (int i = 0; i < 100*ncpus; i++)  // call in parallel an empty entry of a for loop on all cpus
        {
            // Empty loop which does nothing and simply returns
        }
        ::Sleep(200);
    }
    auto ms = swParallel.Stop();
    printf("Executed %dx empty OpenMP loop on all cores with a 200ms delay in between. Elapsed time: %.1f s\n", Repeat, ms.count() / 1000.0);
}


std::wstring GetFileVersion(std::wstring& pszFilePath)
{
    DWORD               dwSize = 0;
    BYTE* pbVersionInfo = NULL;
    wchar_t* pFileInfo = NULL;
    UINT                puLenFileInfo = 0;

    // Get the version information for the file requested
    dwSize = GetFileVersionInfoSize(pszFilePath.c_str(), NULL);
    if (dwSize == 0)
    {
        printf("Error in GetFileVersionInfoSize: %d\n", GetLastError());
        return L"";
    }

    pbVersionInfo = new BYTE[dwSize];

    if (!GetFileVersionInfo(pszFilePath.c_str(), 0, dwSize, pbVersionInfo))
    {
        printf("Error in GetFileVersionInfo: %d\n", GetLastError());
        delete[] pbVersionInfo;
        return L"";
    }

    if (!VerQueryValue(pbVersionInfo, L"\\StringFileInfo\\040704b0\\FileVersion", (LPVOID*)&pFileInfo, &puLenFileInfo))
    {
        printf("Error in VerQueryValue: %d\n", GetLastError());
        delete[] pbVersionInfo;
        return L"";
    }

    return std::wstring( pFileInfo);
}


void Help(std::wstring &exeFile)
{
    wprintf(L"OpenMPTest [-event] [-pause] or dd [-small] v%s\n", GetFileVersion(exeFile).c_str());
    wprintf(L"  -event     Measure Context Switch latency\n");
    wprintf(L"             Normal ranges are from 0.6s (no C - State) - 16s (C6 or higher) depending on OS and BIOS configured Sleep C-states.\n");
    wprintf(L"  -pause     Measure latency of pause instruction on up to 64 cores. P and E Cores have different latencies.\n");
    wprintf(L"  dd         The first argument is the value which is set for KMP_BLOCKTIME of Intel OpenMP Library. E.g. 100us.\n");
    wprintf(L"             This will test the performance of a tight OpenMP loop which creates many small tasks in a loop which need a lot of scheduling.\n");
    wprintf(L"   [-small]  When -small is used an empty loop is called on all cores 5 times with a second delay in between to show the effect of OpenMP CPU spinning.\n");
    wprintf(L"Examples\n");
    wprintf(L" Measure context switch latency\n");
    wprintf(L"  OpenMPTest -event\n");
    wprintf(L" Measure pause latency\n");
    wprintf(L"  OpenMPTest -pause\n");
    wprintf(L" Run OpenMP workload with KMP_BLOCKTIME=0\n");
    wprintf(L"  OpenMPTest 0\n");
    wprintf(L" Run OpenMP workload with KMP_BLOCKTIME=1\n");
    wprintf(L"  OpenMPTest 1\n");
    wprintf(L" Run small OpenMP workload with KMP_BLOCKTIME=200\n");
    wprintf(L"  OpenMPTest 200 -small\n");

}

HANDLE ev1, ev2;
const int NEvents = 100 * 1000;

void Thread1()
{
    DWORD_PTR mask = static_cast<DWORD_PTR>(1) << 1;
    auto afflret = ::SetThreadAffinityMask(::GetCurrentThread(), mask);
    if (afflret == 0)
    {
        wprintf(L"\nSetThreadAffinityMask did fail with %d", ::GetLastError());
        return;
    }

    for (int i = 0; i < NEvents; i++)
    {
        ::WaitForSingleObject(ev1, INFINITE);
        ::ResetEvent(ev1);
        ::SetEvent(ev2);
    }
}

void Thread2()
{
    DWORD_PTR mask = static_cast<DWORD_PTR>(1) << 2;
    auto afflret = ::SetThreadAffinityMask(::GetCurrentThread(), mask);
    if (afflret == 0)
    {
        wprintf(L"\nSetThreadAffinityMask did fail with %d", ::GetLastError());
        return;
    }

    for (int i = 0; i < NEvents; i++)
    {
        ::WaitForSingleObject(ev2, INFINITE);
        ::ResetEvent(ev2);
        ::SetEvent(ev1);
    }
}

void EventTest()
{
    Stopwatch sw;

    ev1 = ::CreateEvent(nullptr, TRUE, TRUE, nullptr);
    ev2 = ::CreateEvent(nullptr, TRUE, TRUE, nullptr);

    std::thread thread_1 = std::thread(Thread1);
    std::thread thread_2 = std::thread(Thread2);

    thread_1.join();
    thread_2.join();

    auto ms = sw.Stop();
    printf("Event Test of %d signals on two threads. Elapsed time: %.1f s\n", NEvents, ms.count() / 1000.0);
}

int wmain(int argc, wchar_t** argv)
{
    std::wstring exeName = argv[0];
    if (argc == 1)
    {
        Help(exeName);
        return 0;
    }

    if (argc > 1)
    {
        std::wstring firstArg = argv[1];

        if (firstArg == L"-pause")
        {
            MeasurePauseLatency();
            return 0;
        }
        else if (firstArg == L"-event")
        {
            EventTest();
            return 0;
        }

        wprintf(L"Setting KMP_BLOCKTIME to %s\n", argv[1]);
        ::SetEnvironmentVariable(L"KMP_BLOCKTIME", argv[1]);

        if (argc > 2)
        {
            
            std::wstring str = argv[2];
            if (str == L"-small")
            {
                MeasureSmallWork();
            }
            else
            {
                Help(exeName);
                return 0;
            }

        }
        else
        {
            MeasureOpenMPPerformance();
        }
    }
    else
    {
        Help(exeName);
    }
}