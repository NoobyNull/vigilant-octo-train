// Digital Workshop - Filesystem Detection Implementation
// Cross-platform detection of local vs network filesystems.

#include "core/import/filesystem_detector.h"

#include <filesystem>

namespace dw {

namespace {

// Walk up to find the nearest existing ancestor directory
Path findExistingAncestor(const Path& p) {
    Path current = p;
    // Normalize to absolute if possible
    std::error_code ec;
    if (current.is_relative()) {
        auto abs = fs::absolute(current, ec);
        if (!ec)
            current = abs;
    }

    while (!current.empty() && !fs::exists(current, ec)) {
        auto parent = current.parent_path();
        if (parent == current)
            break; // reached root
        current = parent;
    }
    return current;
}

} // namespace

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

FilesystemInfo detectFilesystem(const Path& path) {
    if (path.empty())
        return {StorageLocation::Unknown, ""};

    Path target = findExistingAncestor(path);
    if (target.empty())
        return {StorageLocation::Unknown, ""};

    // GetDriveTypeW needs the root path (e.g., "C:\\")
    std::wstring root = target.root_path().wstring();
    if (root.empty())
        return {StorageLocation::Unknown, ""};

    UINT driveType = GetDriveTypeW(root.c_str());
    switch (driveType) {
    case DRIVE_REMOTE:
        return {StorageLocation::Network, "remote"};
    case DRIVE_FIXED:
        return {StorageLocation::Local, "fixed"};
    case DRIVE_REMOVABLE:
        return {StorageLocation::Local, "removable"};
    case DRIVE_CDROM:
        return {StorageLocation::Local, "cdrom"};
    case DRIVE_RAMDISK:
        return {StorageLocation::Local, "ramdisk"};
    default:
        return {StorageLocation::Unknown, "unknown"};
    }
}

#elif defined(__APPLE__)

#include <sys/mount.h>
#include <cstring>

FilesystemInfo detectFilesystem(const Path& path) {
    if (path.empty())
        return {StorageLocation::Unknown, ""};

    Path target = findExistingAncestor(path);
    if (target.empty())
        return {StorageLocation::Unknown, ""};

    struct statfs buf;
    if (statfs(target.string().c_str(), &buf) != 0)
        return {StorageLocation::Unknown, ""};

    std::string fsType(buf.f_fstypename);

    // Known network filesystem types on macOS
    if (fsType == "nfs" || fsType == "smbfs" || fsType == "afpfs" || fsType == "webdav") {
        return {StorageLocation::Network, fsType};
    }

    return {StorageLocation::Local, fsType};
}

#elif defined(__linux__)

#include <sys/vfs.h>

FilesystemInfo detectFilesystem(const Path& path) {
    if (path.empty())
        return {StorageLocation::Unknown, ""};

    Path target = findExistingAncestor(path);
    if (target.empty())
        return {StorageLocation::Unknown, ""};

    struct statfs buf;
    if (statfs(target.string().c_str(), &buf) != 0)
        return {StorageLocation::Unknown, ""};

    // Magic numbers for network filesystems
    constexpr long NFS_MAGIC = 0x6969;
    constexpr long SMB_MAGIC = 0x517B;
    constexpr long CIFS_MAGIC = 0xFF534D42;
    constexpr long SMB2_MAGIC = 0xFE534D42;
    constexpr long FUSE_MAGIC = 0x65735546;

    std::string fsName;
    auto ftype = static_cast<long>(buf.f_type);

    switch (ftype) {
    case NFS_MAGIC:
        fsName = "nfs";
        break;
    case SMB_MAGIC:
        fsName = "smb";
        break;
    case CIFS_MAGIC:
        fsName = "cifs";
        break;
    case SMB2_MAGIC:
        fsName = "smb2";
        break;
    case FUSE_MAGIC:
        fsName = "fuse";
        break;
    default:
        // Not a known network fs -- treat as local
        return {StorageLocation::Local, "local"};
    }

    return {StorageLocation::Network, fsName};
}

#else

// Unsupported platform fallback
FilesystemInfo detectFilesystem(const Path& /*path*/) {
    return {StorageLocation::Unknown, ""};
}

#endif

} // namespace dw
