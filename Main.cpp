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
#pragma omp parallel for 
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
    wprintf(L"OpenMPTest -pause or dd [-small] v%s\n", GetFileVersion(exeFile).c_str());
    wprintf(L" The first mode measures the latency of the pause instruction. The second mode measures Intel OpenMP performance.\n");
    wprintf(L"  -pause     Measure latency of pause instruction on up to 64 cores.\n");
    wprintf(L"  dd         The first argument is the value which is set for KMP_BLOCKTIME of Intel OpenMP Library. E.g. 100us.\n");
    wprintf(L"             This will test the performance of a tight OpenMP loop which creates many small tasks in a loop which need a lot of scheduling.\n");
    wprintf(L"   [-small]  When -small is used an empty loop is called on all cores 5 times with a second delay in between to show the effect of OpenMP CPU spinning.\n");
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