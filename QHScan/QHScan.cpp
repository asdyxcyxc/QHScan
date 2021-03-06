/*++

     #######  ##     ##  ######   ######     ###    ##    ##
    ##     ## ##     ## ##    ## ##    ##   ## ##   ###   ##
    ##     ## ##     ## ##       ##        ##   ##  ####  ##
    ##     ## #########  ######  ##       ##     ## ## ## ##
    ##  ## ## ##     ##       ## ##       ######### ##  ####
    ##    ##  ##     ## ##    ## ##    ## ##     ## ##   ###
     ##### ## ##     ##  ######   ######  ##     ## ##    ##

                 Quick Heal Scanner Client

Author : Ashfaq Ansari
Contact: ashfaq[at]cloudfuzz[dot]io
Twitter: @HackSysTeam
Website: http://www.cloudfuzz.io/

Copyright (C) 2019-2020 CloudFuzz Technolabs Pvt. Ltd. All rights reserved.

This program is free software: you can redistribute it and/or modify it under the terms of
the GNU General Public License as published by the Free Software Foundation, either version
3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.
If not, see <http://www.gnu.org/licenses/>.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

See the file 'LICENSE' for complete copying permission.

--*/

#include "QHScan.h"


INT
WINAPI
ScanCallback(
    PCALLBACK_PARAM_1 CallbackParam,
    INT a2
)
{
    INT retCode = QH_SCAN_CONTINUE;

    /*
    DEBUG("\t[+] ScanCallback\n");
    DEBUG("\t    [+] Code: 0x%x\n", CallbackParam->Code);
    DEBUG("\t    [+] Path: %s\n", CallbackParam->FilePath);
    DEBUG("\t    [+] DetectionDescription: %s\n", CallbackParam->DetectionDescription);
    DEBUG("\t    [+] a2: 0x%x\n", a2);
    */


    /**
     * There are some more codes to catch.
     * But current one works well for our case.
     */

    switch (CallbackParam->Code)
    {
    case QH_SCAN_SKIPPED:
        DEBUG("\t\t[+] [Skipped]\n");
        break;
    case QH_SCAN_DELETED:
        DEBUG("\t\t[+] [Deleted]\n");
        break;
    case QH_SCAN_UNREPAIRABLE:
        DEBUG("\t\t[+] [Unrepairable]\n");
        break;
    case QH_SCAN_MARKED_FOR_DELETION:
        DEBUG("\t\t[+] [Marked to be deleted]\n");
        break;
    case QH_SCAN_REPAIRED:
        DEBUG("\t\t[+] [Repaired]\n");
        break;
    case QH_SCAN_INFECTED1:
        if (CallbackParam->SuspiciousIndicator == 0x26)
        {
            DEBUG("\t\t[+] Warning: (Suspicious) - DNAScan\n");
        }
        else
        {
            DEBUG("\t\t[+] Infected: %s\n", CallbackParam->DetectionDescription);
        }
        break;
    case QH_SCAN_UNKNOWN:
        break;
    case QH_SCAN_ARCHIVE:
        DEBUG("\t\t[+] [Archive]\n");
        break;
    case QH_SCAN_INFECTED_ARCHIVE:
        DEBUG("\t\t[+] [Infected Archive]\n");
        break;
    case QH_SCAN_IO_ERROR:
        DEBUG("\t\t[+] [I/O Error]\n");
        break;
    case QH_SCAN_INFECTED2:
        if (CallbackParam->SuspiciousIndicator == 0x26)
        {
            DEBUG("\t\t[+] Warning: (Suspicious) - DNAScan\n");
        }
        else
        {
            DEBUG("\t\t[+] Infected: %s\n", CallbackParam->DetectionDescription);
        }
        break;
    default:
        retCode = QH_SCAN_CONTINUE;
        break;
    }

    return retCode;
}


VOID
ScanFilesInDrectory(
    PCHAR Directory
)
{
    PCHAR sSlash;
    CHAR Path[MAX_PATH] = { 0 };
    WIN32_FIND_DATAA FindFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    SIZE_T PathLength = strlen(Directory);

    if (PathLength > sizeof(Path))
    {
        DEBUG("[-] Path exceeds MAX_PATH\n");
        return;
    }

    strncpy(Path, Directory, PathLength);

    if (Path[PathLength - 1] == '\\')
    {
        strncat(Path, "*", strlen("*"));
    }
    else
    {
        strncat(Path, "\\*", strlen("\\*"));
    }

    sSlash = strrchr(Path, '*');

    hFind = FindFirstFileA(Path, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        // DEBUG("FindFirstFile Failed: 0x%X\n", GetLastError());
        // DEBUG("Failed path: %s", Path);
        return;
    }
    else
    {
        do
        {
            if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                memset(sSlash, 0, strlen(FindFileData.cFileName) + 1);
                strncpy(sSlash, FindFileData.cFileName, strlen(FindFileData.cFileName));

                // DEBUG("%s\n", Path);
                ScanFile(Path);
            }
            else if (strcmp(FindFileData.cFileName, ".") != 0 &&
                strcmp(FindFileData.cFileName, "..") != 0)
            {
                memset(sSlash, 0, strlen(FindFileData.cFileName) + 1);
                strncpy(sSlash, FindFileData.cFileName, strlen(FindFileData.cFileName));

                ScanFilesInDrectory(Path);
            }
        } while (FindNextFileA(hFind, &FindFileData));
    }
}


BOOL
__declspec(dllexport)
ScanFile(
    PCHAR PathToScan
)
{
    INT retCode = 0;
    BOOL Successful = FALSE;
    ULONG_PTR QhScanFileExOutBuffer;
    ULONG_PTR OpenFileForSASOutBuffer;
    ULONG_PTR GetFileTypeForSASOutBuffer1;
    ULONG_PTR GetFileTypeForSASOutBuffer2;

    DEBUG("[+] Scanning: %s\n", PathToScan);

    // DEBUG("\t[+] QhOpenFileForSAS\n");
    retCode = QhOpenFileForSAS(0, PathToScan, 0, 0, &OpenFileForSASOutBuffer);

    if (!retCode)
    {
        DEBUG("\t[-] QhOpenFileForSAS retCode: 0x%X\n", retCode);
        return Successful;
    }

    // DEBUG("\t[+] QhGetFileTypeForSAS\n");
    retCode = QhGetFileTypeForSAS(
        OpenFileForSASOutBuffer,
        &GetFileTypeForSASOutBuffer1,
        &GetFileTypeForSASOutBuffer2
    );

    if (!retCode)
    {
        DEBUG("\t[-] QhGetFileTypeForSAS retCode: 0x%X\n", retCode);
        return Successful;
    }

    DEBUG("\t[+] QhScanFileEx\n");
    retCode = QhScanFileEx(OpenFileForSASOutBuffer, &QhScanFileExOutBuffer);

    if (!retCode)
    {
        DEBUG("\t[-] QhScanFileEx retCode: 0x%X\n", retCode);
        return Successful;
    }

    // DEBUG("\t[+] QhCloseFile\n");
    retCode = QhCloseFile(&OpenFileForSASOutBuffer);

    if (!retCode)
    {
        DEBUG("\t[-] QhCloseFile retCode: 0x%X\n", retCode);
        return Successful;
    }

    Successful = TRUE;
    return Successful;
}


VOID
InitializeScanSDK(
    VOID
)
{
    HMODULE hScanSDK = NULL;
    BOOL SDKInitialized = FALSE;
    LPCTSTR WorkingDirectory = L"C:\\Program Files\\Quick Heal\\Quick Heal AntiVirus Pro\\";
    LPCTSTR ScanSDK = L"C:\\Program Files\\Quick Heal\\Quick Heal AntiVirus Pro\\SCANSDK.DLL";

    SetCurrentDirectory(L"C:\\Program Files\\Quick Heal\\Quick Heal AntiVirus Pro\\");

    DEBUG("[+] Loading ScanSDK\n");
    DEBUG("\t[+] DLL: %ls\n", ScanSDK);

    hScanSDK = LoadLibrary(ScanSDK);

    if (!hScanSDK)
    {
        DEBUG("\t[-] Error loading ScanSDK: 0x%X\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    DEBUG("\t[+] Handle: 0x%p\n", hScanSDK);

    DEBUG("[+] Resolving ScanSDK APIs\n");

    QhInitScanForSAS = (QhInitScanForSAS_t)GetProcAddress(hScanSDK, "QhInitScanForSAS");
    QhSetCallbackForSAS = (QhSetCallbackForSAS_t)GetProcAddress(hScanSDK, "QhSetCallbackForSAS");
    QhOpenFileForSAS = (QhOpenFileForSAS_t)GetProcAddress(hScanSDK, "QhOpenFileForSAS");
    QhGetFileTypeForSAS = (QhGetFileTypeForSAS_t)GetProcAddress(hScanSDK, "QhGetFileTypeForSAS");
    QhScanFileEx = (QhScanFileEx_t)GetProcAddress(hScanSDK, "QhScanFileEx");
    QhCloseFile = (QhCloseFile_t)GetProcAddress(hScanSDK, "QhCloseFile");
    QhDeinitScanForSAS = (QhDeinitScanForSAS_t)GetProcAddress(hScanSDK, "QhDeinitScanForSAS");

    /*
    DEBUG("\t[+] QhInitScanForSAS: 0x%p\n", QhInitScanForSAS);
    DEBUG("\t[+] QhSetCallbackForSAS: 0x%p\n", QhSetCallbackForSAS);
    DEBUG("\t[+] QhOpenFileForSAS: 0x%p\n", QhOpenFileForSAS);
    DEBUG("\t[+] QhGetFileTypeForSAS: 0x%p\n", QhGetFileTypeForSAS);
    DEBUG("\t[+] QhScanFileEx: 0x%p\n", QhScanFileEx);
    DEBUG("\t[+] QhCloseFile: 0x%p\n", QhCloseFile);
    DEBUG("\t[+] QhDeinitScanForSAS: 0x%p\n", QhDeinitScanForSAS);
    */
}


int
main(
    int argc,
    char *argv[]
)
{
    INT retCode = 0;
    HMODULE hScanSDK = NULL;
    PCHAR PathToScan = NULL;
    INITSCAN_1 initScanParam1 = { 0 };
    INITSCAN_2 initScanParam2 = { 0 };
    ULONG_PTR InitScanForSASOutBuffer;
    PCHAR QuickHealDirectory = (PCHAR)"C:\\PROGRA~1\\QUICKH~1\\QUICKH~1";
    PCHAR TempDirectory = (PCHAR)"C:\\PROGRA~1\\QUICKH~1\\QUICKH~1\\Temp";

    /**
     * Get the file to scan
     */

    DEBUG("%s", BANNER);

    if (argc != 2)
    {
        DEBUG("Usage: %s [file to scan]\n", argv[0]);
        return -1;
    }

    PathToScan = argv[1];

    /**
     * Check if the path exists
     */

    if (!PathFileExistsA(PathToScan))
    {
        DEBUG("[-] Invalid path to scan\n");
        return -2;
    }

    InitializeScanSDK();

    /**
     * Initialize the Scan Engine
     */

    initScanParam1.Size = sizeof(INITSCAN_1);
    initScanParam1.field_2 = 0x1;
    initScanParam1.field_4 = 0x2;
    initScanParam1.field_6 = 0x1;
    initScanParam1.field_8 = 0x1;
    initScanParam1.field_210 = 0x0;
    initScanParam1.field_212 = 0x17f;
    initScanParam1.field_214 = 0x540a;
    initScanParam1.field_216 = 0xfc6e;
    initScanParam1.field_218 = 0x0;
    initScanParam1.field_220 = 0x0;
    initScanParam1.field_222 = 0x0;
    initScanParam1.field_224 = 0x0;
    initScanParam1.field_226 = 0x0;
    initScanParam1.field_228 = 0x0;

    strcpy(initScanParam1.QuickHealDir, QuickHealDirectory);
    strcpy(initScanParam1.TempDir, TempDirectory);

    initScanParam2.field_0 = 0x0;
    initScanParam2.field_4 = 0x11;
    initScanParam2.CurrentPid = GetCurrentProcessId();
    initScanParam2.field_C = 0x4;
    initScanParam2.field_10 = 0x0;
    initScanParam2.field_14 = 0x0;

    // DEBUG("[+] QhInitScanForSAS\n");
    retCode = QhInitScanForSAS(&initScanParam1, &initScanParam2, &InitScanForSASOutBuffer);

    if (!retCode)
    {
        DEBUG("\t[-] QhInitScanForSAS retCode: 0x%X\n", retCode);
        return -3;
    }

    // DEBUG("[+] QhSetCallbackForSAS\n");
    retCode = QhSetCallbackForSAS(0, ScanCallback);

    if (!retCode)
    {
        DEBUG("\t[-] QhSetCallbackForSAS retCode: 0x%X\n", retCode);
        return -4;
    }

    if (PathIsDirectoryA(PathToScan))
    {
        ScanFilesInDrectory(PathToScan);
    }
    else
    {
        ScanFile(PathToScan);
    }

    // DEBUG("[+] QhDeinitScanForSAS\n");
    retCode = QhDeinitScanForSAS(0);

    if (!retCode)
    {
        DEBUG("\t[-] QhDeinitScanForSAS retCode: 0x%X\n", retCode);
        return -5;
    }
}
