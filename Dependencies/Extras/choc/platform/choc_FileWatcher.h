//
//    ██████ ██   ██  ██████   ██████
//   ██      ██   ██ ██    ██ ██            ** Classy Header-Only Classes **
//   ██      ███████ ██    ██ ██
//   ██      ██   ██ ██    ██ ██           https://github.com/Tracktion/choc
//    ██████ ██   ██  ██████   ██████
//
//   CHOC is (C)2022 Tracktion Corporation, and is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef CHOC_FILE_WATCHER_HEADER_INCLUDED
#define CHOC_FILE_WATCHER_HEADER_INCLUDED

#pragma once

#include <filesystem>
#include <chrono>
#include <memory>
#include <string>
#include <functional>
#include "choc_Platform.h"
#include "choc_Assert.h"

namespace choc::file
{

//==============================================================================
/// This object watches a filesystem path for changes, and calls a callback
/// to report any such events.
///
/// Note that there's no guarantee about exactly which events this will capture,
/// nor that it will capture everything that happens - if a file is created and
/// deleted in-between scans, it could easily be missed.
struct Watcher
{
    enum class EventType
    {
        modified,
        created,
        destroyed,
        renamed,
        ownerChanged,
        other
    };

    enum class FileType
    {
        file,
        folder,
        unknown
    };

    struct Event
    {
        Event (EventType, FileType, std::filesystem::path);

        EventType eventType;
        FileType fileType;
        std::filesystem::path file;
        std::chrono::time_point<std::chrono::system_clock> time;
    };

    //==============================================================================
    /// Creates a Watcher object which will monitor the given file or folder
    /// for changes.
    ///
    /// While the Watcher exists, it will call the callback whenever a file is
    /// modified, providing an Event argument that describes the type of change.
    /// The callbacks will be made from a background thread, and to stop it,
    /// simply delete the Watcher object.
    ///
    /// If millisecondsBetweenChecks is non-zero, it lets you override the default
    /// rate at which checking is done, although depending on the implementation,
    /// this may or may not make any difference.
    Watcher (std::filesystem::path fileOrFolderToWatch,
             std::function<void(const Event&)> callback,
             uint32_t millisecondsBetweenChecks = 0);

    /// When a Watcher is destroyed, it will shut down its thread and stop making
    /// callbacks.
    ~Watcher();

private:
    //==============================================================================
    std::filesystem::path target;
    std::function<void(const Event&)> callback;
    const uint32_t millisecondsBetweenChecks;

    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

} // namespace choc::file


//==============================================================================
//        _        _           _  _
//     __| |  ___ | |_   __ _ (_)| | ___
//    / _` | / _ \| __| / _` || || |/ __|
//   | (_| ||  __/| |_ | (_| || || |\__ \ _  _  _
//    \__,_| \___| \__| \__,_||_||_||___/(_)(_)(_)
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

#if CHOC_APPLE

#include <CoreServices/CoreServices.h>

struct choc::file::Watcher::Pimpl
{
    Pimpl (Watcher& w) : watcher (w)
    {
        const void* pathString = CFStringCreateWithCString (nullptr, w.target.string().c_str(), kCFStringEncodingUTF8);
        auto pathArray = CFArrayCreate (nullptr, std::addressof (pathString), 1, &kCFTypeArrayCallBacks);
        FSEventStreamContext context = {};
        context.info = this;

        eventStream = FSEventStreamCreate (nullptr, callback, std::addressof (context),
                                           pathArray, kFSEventStreamEventIdSinceNow,
                                           (w.millisecondsBetweenChecks != 0 ? w.millisecondsBetweenChecks : 100u) / 1000.0,
                                           kFSEventStreamCreateFlagFileEvents
                                             | kFSEventStreamCreateFlagUseExtendedData
                                             | kFSEventStreamCreateFlagUseCFTypes
                                             | kFSEventStreamCreateFlagNoDefer);

        auto attr = dispatch_queue_attr_make_with_qos_class (DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, -10);
        dispatchQueue = dispatch_queue_create ("choc.filewatcher", attr);

        FSEventStreamSetDispatchQueue (eventStream, dispatchQueue);
        FSEventStreamStart (eventStream);
    }

    ~Pimpl()
    {
        if (eventStream)
        {
            FSEventStreamStop (eventStream);
            FSEventStreamInvalidate (eventStream);
            FSEventStreamRelease (eventStream);
        }

        if (dispatchQueue)
            dispatch_release (dispatchQueue);
    }

    void handleEvents (size_t numEvents, CFArrayRef eventPaths, const FSEventStreamEventFlags* eventFlags)
    {
        for (size_t i = 0; i < numEvents; ++i)
        {
            auto pathDict = static_cast<CFDictionaryRef> (CFArrayGetValueAtIndex (eventPaths, static_cast<CFIndex> (i)));
            auto pathCFString = static_cast<CFStringRef> (CFDictionaryGetValue (pathDict, kFSEventStreamEventExtendedDataPathKey));
            auto path = CFStringGetCStringPtr (pathCFString, kCFStringEncodingUTF8);
            auto flags = eventFlags[i];

            auto fileType = (flags & kFSEventStreamEventFlagItemIsDir) ? FileType::folder
                                                                       : FileType::file;

            if (flags & kFSEventStreamEventFlagItemCreated)      sendEvent (path, fileType, EventType::created);
            if (flags & kFSEventStreamEventFlagItemModified)     sendEvent (path, fileType, EventType::modified);
            if (flags & kFSEventStreamEventFlagItemRemoved)      sendEvent (path, fileType, EventType::destroyed);
            if (flags & kFSEventStreamEventFlagItemRenamed)      sendEvent (path, fileType, EventType::renamed);
            if (flags & kFSEventStreamEventFlagItemChangeOwner)  sendEvent (path, fileType, EventType::ownerChanged);
        }
    }

    void sendEvent (const char* path, FileType fileType, EventType eventType)
    {
        watcher.callback (Event (eventType, fileType, path));
    }

    static void callback (ConstFSEventStreamRef, void* clientInfo,
                          size_t numEvents, void* eventPaths,
                          const FSEventStreamEventFlags* eventFlags,
                          const FSEventStreamEventId*)
    {
        static_cast<Pimpl*> (clientInfo)
           ->handleEvents (numEvents, static_cast<CFArrayRef> (eventPaths), eventFlags);
    }

    Watcher& watcher;
    dispatch_queue_t dispatchQueue = {};
    FSEventStreamRef eventStream = {};
};

//==============================================================================
#elif CHOC_LINUX || CHOC_ANDROID

#include <unistd.h>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include "../threading/choc_TaskThread.h"

struct choc::file::Watcher::Pimpl
{
    Pimpl (Watcher& w)  : watcher (w)
    {
       #if CHOC_ANDROID
        watchFD = inotify_init();
       #else
        watchFD = inotify_init1 (IN_NONBLOCK);
       #endif

        if (watchFD >= 0 && createEventCtl())
        {
            rootINode = std::make_unique<INodeWatcher> (watcher.target, watchFD);
            taskThread.start (10, [this] { check(); });
        }
    }

    ~Pimpl()
    {
        taskThread.stop();
        rootINode.reset();

        if (watchFD >= 0)
            close (watchFD);
    }

    bool createEventCtl()
    {
        eventConfig.events = EPOLLIN;
        eventConfig.data.fd = watchFD;
        eventFD = epoll_create1 (EPOLL_CLOEXEC);
        return epoll_ctl (eventFD, EPOLL_CTL_ADD, watchFD, std::addressof (eventConfig)) >= 0;
    }

    void check()
    {
        auto numEvents = epoll_wait (eventFD, eventList, eventListSize, 300);

        if (numEvents <= 0)
            return;

        for (;;)
        {
            alignas (inotify_event) char buffer[4096];
            auto bytesRead = read (watchFD, buffer, sizeof (buffer));

            if (bytesRead <= 0)
                break;

            for (char* i = buffer; i < buffer + bytesRead;)
            {
                auto e = reinterpret_cast<const inotify_event*> (i);
                handleEvent (*e);
                i += sizeof (inotify_event) + e->len;
            }
        }
    }

    void handleEvent (const inotify_event& e)
    {
        CHOC_ASSERT (rootINode != nullptr);

        if (auto inode = rootINode->findINode (e.wd))
        {
            auto fileType = (e.mask & IN_ISDIR) ? FileType::folder : FileType::file;
            auto path = inode->path / e.name;

            if ((e.mask & (IN_CREATE | IN_ISDIR)) == (IN_CREATE | IN_ISDIR))
                inode->addChildFolder (path);

            if ((e.mask & (IN_DELETE | IN_ISDIR)) == (IN_DELETE | IN_ISDIR))
                inode->removeChildFolder (path);

            if (e.mask & IN_CREATE)  watcher.callback (Event (EventType::created, fileType, path));
            if (e.mask & IN_DELETE)  watcher.callback (Event (EventType::destroyed, fileType, path));
            if (e.mask & IN_MOVE)    watcher.callback (Event (EventType::modified, fileType, path));
            if (e.mask & IN_MODIFY)  watcher.callback (Event (EventType::modified, fileType, path));
        }
    }

    struct INodeWatcher
    {
        INodeWatcher (std::filesystem::path p, int watchFDToUse)
            : path (std::move (p)), wd (watchFDToUse)
        {
            fd = inotify_add_watch (wd, path.string().c_str(),
                                    IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_Q_OVERFLOW);

            std::error_code errorCode = {};

            if (fd >= 0 && is_directory (path, errorCode))
                for (auto& f : std::filesystem::directory_iterator (p, std::filesystem::directory_options::skip_permission_denied, errorCode))
                    if (! errorCode)
                        if (is_directory (f, errorCode))
                            if (! errorCode)
                                addChildFolder (f.path());
        }

        ~INodeWatcher()
        {
            if (fd >= 0)
                inotify_rm_watch (fd, wd);
        }

        void addChildFolder (std::filesystem::path f)
        {
            auto w = std::make_unique<INodeWatcher> (std::move (f), wd);
            auto childFD = w->fd;
            childFolders[childFD] = std::move (w);
        }

        void removeChildFolder (const std::filesystem::path& f)
        {
            for (auto i = childFolders.begin(); i != childFolders.end();)
            {
                if (i->second->path == f)
                    i = childFolders.erase (i);
                else
                    ++i;
            }
        }

        INodeWatcher* findINode (int desc)
        {
            auto i = childFolders.find (desc);

            if (i != childFolders.end())
                return i->second.get();

            if (desc == fd)
                return this;

            for (auto& c : childFolders)
                if (auto inode = c.second->findINode (desc))
                    return inode;

            return {};
        }

        std::filesystem::path path;
        int fd = 0, wd = 0;
        std::unordered_map<int, std::unique_ptr<INodeWatcher>> childFolders;
    };

    Watcher& watcher;
    int watchFD = {}, eventFD = {};

    epoll_event eventConfig = {};
    static constexpr size_t eventListSize = 1;
    epoll_event eventList[eventListSize];

    std::unique_ptr<INodeWatcher> rootINode;
    choc::threading::TaskThread taskThread;
};

//==============================================================================
#elif 0 // CHOC_WINDOWS

struct choc::file::Watcher::Pimpl
{
    Pimpl (Watcher& w) : watcher (w)
    {
        // TODO! Implement using ReadDirectoryChangesW
    }

    ~Pimpl()
    {
    }

    Watcher& watcher;
};

//==============================================================================
#else

#include <unordered_map>
#include "../threading/choc_TaskThread.h"

struct choc::file::Watcher::Pimpl
{
    Pimpl (Watcher& w) : watcher (w)
    {
        findNewAndChangedFiles (std::filesystem::directory_entry (watcher.target), false);

        taskThread.start (watcher.millisecondsBetweenChecks != 0 ? watcher.millisecondsBetweenChecks : 100,
                          [this]
                          {
                              findNewAndChangedFiles (std::filesystem::directory_entry (watcher.target), true);
                              findDeletedItems();
                          });
    }

    ~Pimpl()
    {
        taskThread.stop();
    }

    void findNewAndChangedFiles (const std::filesystem::directory_entry& file, bool sendCallbacks)
    {
        std::error_code errorCode = {};
        auto lastWriteTime = file.last_write_time (errorCode);
        auto name = file.path().string();
        auto existingInfo = fileInfo.find (name);

        if (existingInfo != fileInfo.end())
        {
            if (! file.exists())
            {
                auto e = Event (EventType::destroyed, existingInfo->second.fileType, file);
                fileInfo.erase (name);

                if (sendCallbacks)
                    watcher.callback (e);
            }
            else if (file.is_directory())
            {
                for (auto& i : std::filesystem::directory_iterator (file, iterOptions, errorCode))
                {
                    if (errorCode)
                        break;

                    findNewAndChangedFiles (i, sendCallbacks);
                }
            }
            else if (existingInfo->second.lastWriteTime != lastWriteTime)
            {
                fileInfo[name].lastWriteTime = lastWriteTime;

                if (sendCallbacks)
                    watcher.callback (Event (EventType::modified, existingInfo->second.fileType, file));
            }
        }
        else
        {
            auto fileType = file.is_directory() ? FileType::folder : FileType::file;
            fileInfo[name] = { fileType, lastWriteTime };

            if (sendCallbacks)
                watcher.callback (Event (EventType::created, fileType, file));

            if (fileType == FileType::folder)
            {
                for (auto& i : std::filesystem::directory_iterator (file, iterOptions, errorCode))
                {
                    if (errorCode)
                        break;

                    findNewAndChangedFiles (i, sendCallbacks);
                }
            }
        }
    }

    void findDeletedItems()
    {
        for (auto i = fileInfo.begin(); i != fileInfo.end();)
        {
            if (std::filesystem::exists (i->first))
            {
                ++i;
            }
            else
            {
                watcher.callback (Event (EventType::destroyed, i->second.fileType, i->first));
                i = fileInfo.erase (i);
            }
        }
    }

    Watcher& watcher;
    choc::threading::TaskThread taskThread;

    struct FileInfo
    {
        FileType fileType = FileType::unknown;
        std::filesystem::file_time_type lastWriteTime = {};
    };

    std::unordered_map<std::string, FileInfo> fileInfo;

    static constexpr std::filesystem::directory_options iterOptions
        = std::filesystem::directory_options::skip_permission_denied;
};

#endif

//==============================================================================
//==============================================================================
namespace choc::file
{

inline Watcher::Event::Event (EventType t, FileType ft, std::filesystem::path f)
    : eventType (t), fileType (ft), file (std::move (f)),
      time (std::chrono::system_clock::now())
{
}

inline Watcher::Watcher (std::filesystem::path t, std::function<void(const Event&)> c, uint32_t rate)
    : target (std::move (t)), callback (std::move (c)), millisecondsBetweenChecks (rate)
{
    CHOC_ASSERT (callback); // You must supply a valid callback!
    pimpl = std::make_unique<Pimpl> (*this);
}

inline Watcher::~Watcher() = default;

} // namespace choc::file

#endif  // CHOC_FILE_WATCHER_HEADER_INCLUDED
