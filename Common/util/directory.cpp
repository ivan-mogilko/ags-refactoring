#include "util/directory.h"
#include "core/platform.h"
#include <errno.h>
#if AGS_PLATFORM_OS_WINDOWS
#define NOMINMAX
#include <windows.h>
#undef CreateDirectory
#undef SetCurrentDirectory
#undef GetCurrentDirectory
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "util/path.h"
#include "stdio_compat.h"

// TODO: implement proper portable path length
#ifndef MAX_PATH
#define MAX_PATH 512
#endif
// TODO: find a good place to share this
#define MAX_PATH_UTF8 (MAX_PATH * 4)

namespace AGS
{
namespace Common
{

namespace Directory
{

bool CreateDirectory(const String &path)
{
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wstr[MAX_PATH] = { 0 };
    MultiByteToWideChar(CP_UTF8, 0, path.GetCStr(), -1, wstr, MAX_PATH);
    return CreateDirectoryW(wstr, NULL) != FALSE;
#else
    return mkdir(path.GetCStr()
        , 0755
        ) == 0 || errno == EEXIST;
#endif
}

bool CreateAllDirectories(const String &parent, const String &path)
{
    if (parent.IsEmpty() || !ags_directory_exists(parent.GetCStr()))
        return false;
    if (path.IsEmpty())
        return true;
    if (!Path::IsSameOrSubDir(parent, path))
        return false;

    String fixup_parent = Path::MakeTrailingSlash(parent);
    String sub_path = Path::MakeRelativePath(fixup_parent, path);
    String make_path = parent;
    std::vector<String> dirs = Path::Split(sub_path);
    for (const String &dir : dirs)
    {
        if (dir.IsEmpty() || dir.Compare(".") == 0) continue;
        make_path = Path::ConcatPaths(make_path, dir);
        if (!CreateDirectory(make_path))
            return false;
    }
    return true;
}

String SetCurrentDirectory(const String &path)
{
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wstr[MAX_PATH] = { 0 };
    MultiByteToWideChar(CP_UTF8, 0, path.GetCStr(), -1, wstr, MAX_PATH);
    SetCurrentDirectoryW(wstr);
#else
    chdir(path.GetCStr());
#endif
    return GetCurrentDirectory();
}

String GetCurrentDirectory()
{
    char buf[MAX_PATH_UTF8];
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wstr[MAX_PATH] = { 0 };
    GetCurrentDirectoryW(MAX_PATH, wstr);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buf, MAX_PATH_UTF8, NULL, NULL);
#else
    getcwd(buf, MAX_PATH);
#endif
    String str(buf);
    Path::FixupPath(str);
    return str;
}

} // namespace Directory

} // namespace Common
} // namespace AGS
