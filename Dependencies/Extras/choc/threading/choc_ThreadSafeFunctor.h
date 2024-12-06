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

#ifndef CHOC_THREADSAFEFUNCTOR_HEADER_INCLUDED
#define CHOC_THREADSAFEFUNCTOR_HEADER_INCLUDED

#include <mutex>
#include <memory>

namespace choc::threading
{

//==============================================================================
/**
    This acts as a smart-pointer to a shared function, which can be invoked and
    cleared in a thread-safe way. So all calls to invoke or to change the target
    function are safely locked.

    A good use-case for this is if you trigger an async callback using
    something like `choc::messageloop::postMessage` but need the option to be
    able to cancel the callback before it arrives.
*/
template <typename FunctionType>
struct ThreadSafeFunctor
{
    ThreadSafeFunctor();
    ThreadSafeFunctor (FunctionType&&);

    ThreadSafeFunctor (const ThreadSafeFunctor&) = default;
    ThreadSafeFunctor (ThreadSafeFunctor&&) = default;
    ThreadSafeFunctor& operator= (const ThreadSafeFunctor&) = default;
    ThreadSafeFunctor& operator= (ThreadSafeFunctor&&) = default;
    ~ThreadSafeFunctor() = default;

    /// Changes the function that will be called - this is thread-safe
    /// and will block until it's safe to change the function
    ThreadSafeFunctor& operator= (FunctionType&& newFunctor);

    /// Changes the function that will be called - this is thread-safe
    /// and will block until it's safe to change the function
    ThreadSafeFunctor& operator= (const FunctionType& newFunctor);

    /// Tries to invoke the function if one has been set. Returns true if
    /// the target function is valid and was invoked, or false if there was
    /// no function to call.
    template <typename... Args>
    bool operator() (Args&&... args) const;

    /// Returns true if the function isn't null (although it's still safe to
    /// call this when it's null, and it will do nothing)
    operator bool() const;

    /// Clears the function - this is thread-safe, and will block until
    /// the function can safely be changed
    void reset();


private:
    //==============================================================================
    struct CallbackHolder  : public std::enable_shared_from_this<CallbackHolder>
    {
        std::recursive_mutex lock;
        FunctionType fn;
    };

    std::shared_ptr<CallbackHolder> callback;
};


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

template <typename FunctionType>
ThreadSafeFunctor<FunctionType>::ThreadSafeFunctor() : callback (std::make_shared<CallbackHolder>())
{
}

template <typename FunctionType>
ThreadSafeFunctor<FunctionType>::ThreadSafeFunctor (FunctionType&& f) : ThreadSafeFunctor()
{
    callback->fn = std::move (f);
}

template <typename FunctionType>
template <typename... Args>
bool ThreadSafeFunctor<FunctionType>::operator() (Args&&... args) const
{
    std::lock_guard<decltype(callback->lock)> l (callback->lock);

    if (callback->fn)
    {
        callback->fn (std::forward<Args> (args)...);
        return true;
    }

    return false;
}

template <typename FunctionType>
ThreadSafeFunctor<FunctionType>& ThreadSafeFunctor<FunctionType>::operator= (FunctionType&& f)
{
    std::lock_guard<decltype(callback->lock)> l (callback->lock);
    callback->fn = std::move (f);
    return *this;
}

template <typename FunctionType>
ThreadSafeFunctor<FunctionType>& ThreadSafeFunctor<FunctionType>::operator= (const FunctionType& f)
{
    std::lock_guard<decltype(callback->lock)> l (callback->lock);
    callback->fn = f;
    return *this;
}

template <typename FunctionType>
ThreadSafeFunctor<FunctionType>::operator bool() const
{
    std::lock_guard<decltype(callback->lock)> l (callback->lock);
    return callback->fn;
}

template <typename FunctionType>
void ThreadSafeFunctor<FunctionType>::reset()
{
    std::lock_guard<decltype(callback->lock)> l (callback->lock);
    callback->fn = {};
}

} // namespace choc::threading

#endif // CHOC_THREADSAFEFUNCTOR_HEADER_INCLUDED
