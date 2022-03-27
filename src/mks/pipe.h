#pragma once

#include <functional>
#include <cassert>
#include <mks/log.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#endif

namespace mks {

#ifdef _WIN32
#define mks_socket_t intptr_t
#else
#define mks_socket_t int
#endif

#ifdef _WIN32
#define mks_socklen_t int
#else
#define mks_socklen_t socklen_t
#endif

#ifdef _WIN32
/** Return the most recent socket error.  Not idempotent on all platforms. */
#define MKS_SOCKET_ERROR() WSAGetLastError()
/** Replace the most recent socket error with errcode */
#define MKS_SET_SOCKET_ERROR(errcode)		\
	do { WSASetLastError(errcode); } while (0)
/** Return the most recent socket error to occur on sock. */
int mks_socket_geterror(mks_socket_t sock);
#define MKS_INVALID_SOCKET INVALID_SOCKET
#elif defined(EVENT_IN_DOXYGEN_)
/**
   @name Socket error functions

   These functions are needed for making programs compatible between
   Windows and Unix-like platforms.

   You see, Winsock handles socket errors differently from the rest of
   the world.  Elsewhere, a socket error is like any other error and is
   stored in errno.  But winsock functions require you to retrieve the
   error with a special function, and don't let you use strerror for
   the error codes.  And handling EWOULDBLOCK is ... different.

   @{
*/
/** Return the most recent socket error.  Not idempotent on all platforms. */
#define MKS_SOCKET_ERROR() ...
/** Replace the most recent socket error with errcode */
#define MKS_SET_SOCKET_ERROR(errcode) ...
/** Return the most recent socket error to occur on sock. */
#define mks_socket_geterror(sock) ...
/** Convert a socket error to a string. */
#define mks_socket_error_to_string(errcode) ...
#define MKS_INVALID_SOCKET -1
/**@}*/
#else /** !EVENT_IN_DOXYGEN_ && !_WIN32 */
#define MKS_SOCKET_ERROR() (errno)
#define MKS_SET_SOCKET_ERROR(errcode)		\
		do { errno = (errcode); } while (0)
#define mks_socket_geterror(sock) (errno)
#define MKS_INVALID_SOCKET -1
#endif /** !_WIN32 */

int mks_closesocket(mks_socket_t sock);
int mks_ersatz_socketpair_(int family, int type, int protocol, mks_socket_t fd[2]);
int mks_socketpair(int family, int type, int protocol, mks_socket_t fd[2]);
int mks_make_socket_nonblocking(mks_socket_t fd);
int mks_fast_socket_nonblocking(mks_socket_t fd);

class Pipe {
public:
    using Handler = std::function<void()>;
    uint64_t pre_init_notify_ = 0;
    bool inited_ = false;

    Pipe() {
        memset(pipe_, 0, sizeof(pipe_[0]) * 2);
    }

    ~Pipe() {
        close();
    }

    bool notify() {
        char buf[1] = { '\0' };

        if(!inited_) {
            ++pre_init_notify_;
            MKS_LOG_D("pre-init {}", pre_init_notify_);
            return true;
        }

#ifdef _WIN32
        if (::send(pipe_[1], buf, sizeof(buf), 0) <= 0) {
            return false;
        }
#else
        if (::write(pipe_[1], buf, sizeof(buf)) <= 0) {
            return false;
        }
#endif
        return true;
    }

    void set_callback(const Handler& handler)
    {
        handler_ = handler;
    }

    void set_callback(Handler&& handler)
    {
        handler_ = std::forward<Handler>(handler);
    }


    mks_socket_t wfd() const {
        return pipe_[0];
    }

    bool process() {
        char buf[1];
#ifdef _WIN32
        auto n = ::recv(pipe_[0], buf, sizeof(buf), 0);
#else
        auto n = ::read(pipe_[0], buf, sizeof(buf));
#endif
        if (n == 1) {
            MKS_ASSERT(handler_);
            handler_();
            return true;
        }
        return false;
    }

    bool init() {
        MKS_ASSERT(pipe_[0] == 0);
#ifdef _WIN32
        if (mks_socketpair(AF_UNIX, SOCK_STREAM, 0, pipe_) < 0) {
            int err = errno;
            return false;
        }
        if (mks_make_socket_nonblocking(pipe_[0]) < 0 ||
            mks_make_socket_nonblocking(pipe_[1]) < 0) {
            close();
            return false;
        }
#else
        pipe(pipe_);
#endif
        MKS_ASSERT(pipe_[0] != 0);
        MKS_ASSERT(pipe_[1] != 0);
        inited_ = true;

        if(pre_init_notify_ != 0) {
            MKS_LOG_D("notify pre-init {}", pre_init_notify_);
            for(std::size_t i = 0; i != pre_init_notify_; ++i) {
                auto ret = notify();
                MKS_ASSERT(ret);
            }
            pre_init_notify_ = 0;
        }

        return true;
    }

    void close() {
        if(pipe_[0] > 0) {
            mks_closesocket(pipe_[0]);
            mks_closesocket(pipe_[1]);
            memset(pipe_, 0, sizeof(pipe_[0]) * 2);
        }
        inited_ = false;
    }

private:
    Handler handler_;
    mks_socket_t pipe_[2] = {0, 0}; // Write to pipe_[0] , Read from pipe_[1]
};

}


