//
// Created by michalks on 05.09.2020.
//

#include "pipe.h"

namespace mks {


#ifdef _WIN32
int mks_socket_geterror(mks_socket_t sock) {
    int optval, optvallen=sizeof(optval);
    int err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK && sock >= 0) {
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&optval), &optvallen))
            return err;
        if (optval)
            return optval;
    }
    return err;
}
#endif


int
mks_closesocket(mks_socket_t sock)
{
#ifndef _WIN32
    return close(sock);
#else
    return closesocket(sock);
#endif
}

int
mks_ersatz_socketpair_(int family, int type, int protocol,
                       mks_socket_t fd[2])
{
    /* This code is originally from Tor.  Used with permission. */

    /* This socketpair does not work when localhost is down. So
     * it's really not the same thing at all. But it's close enough
     * for now, and really, when localhost is down sometimes, we
     * have other problems too.
     */
#ifdef _WIN32
#define ERR(e) WSA##e
#else
#define ERR(e) e
#endif
    mks_socket_t listener = -1;
    mks_socket_t connector = -1;
    mks_socket_t acceptor = -1;
    struct sockaddr_in listen_addr{};
    struct sockaddr_in connect_addr{};
    mks_socklen_t size;
    int saved_errno = -1;
    int family_test;

    family_test = family != AF_INET;
#ifdef AF_UNIX
    family_test = family_test && (family != AF_UNIX);
#endif
    if (protocol || family_test) {
        MKS_SET_SOCKET_ERROR(ERR(EAFNOSUPPORT));
        return -1;
    }

    if (!fd) {
        MKS_SET_SOCKET_ERROR(ERR(EINVAL));
        return -1;
    }

    listener = socket(AF_INET, type, 0);
#ifdef _WIN32
    if (listener == INVALID_SOCKET)
        return -1;
#else
    if (listener < 0)
        return -1;
#endif
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    listen_addr.sin_port = 0;	/* kernel chooses port.	 */
    if (bind(listener, (struct sockaddr *) &listen_addr, sizeof (listen_addr))
        == -1)
        goto tidy_up_and_fail;
    if (listen(listener, 1) == -1)
        goto tidy_up_and_fail;

    connector = socket(AF_INET, type, 0);
    if (connector < 0)
        goto tidy_up_and_fail;

    memset(&connect_addr, 0, sizeof(connect_addr));

    /* We want to find out the port number to connect to.  */
    size = sizeof(connect_addr);
    if (getsockname(listener, (struct sockaddr *) &connect_addr, &size) == -1)
        goto tidy_up_and_fail;
    if (size != sizeof (connect_addr))
        goto abort_tidy_up_and_fail;
    if (connect(connector, (struct sockaddr *) &connect_addr,
                sizeof(connect_addr)) == -1)
        goto tidy_up_and_fail;

    size = sizeof(listen_addr);
    acceptor = accept(listener, (struct sockaddr *) &listen_addr, &size);
    if (acceptor < 0)
        goto tidy_up_and_fail;
    if (size != sizeof(listen_addr))
        goto abort_tidy_up_and_fail;
    /* Now check we are talking to ourself by matching port and host on the
       two sockets.	 */
    if (getsockname(connector, (struct sockaddr *) &connect_addr, &size) == -1)
        goto tidy_up_and_fail;
    if (size != sizeof (connect_addr)
        || listen_addr.sin_family != connect_addr.sin_family
        || listen_addr.sin_addr.s_addr != connect_addr.sin_addr.s_addr
        || listen_addr.sin_port != connect_addr.sin_port)
        goto abort_tidy_up_and_fail;
    mks_closesocket(listener);
    fd[0] = connector;
    fd[1] = acceptor;

    return 0;

abort_tidy_up_and_fail:
    saved_errno = ERR(ECONNABORTED);
    tidy_up_and_fail:
    if (saved_errno < 0)
        saved_errno = MKS_SOCKET_ERROR();
    if (listener != -1)
        mks_closesocket(listener);
    if (connector != -1)
        mks_closesocket(connector);
    if (acceptor != -1)
        mks_closesocket(acceptor);

    MKS_SET_SOCKET_ERROR(saved_errno);
    return -1;
#undef ERR
}

int
mks_socketpair(int family, int type, int protocol, mks_socket_t fd[2])
{
#ifndef _WIN32
    return socketpair(family, type, protocol, fd);
#else
    return mks_ersatz_socketpair_(family, type, protocol, fd);
#endif
}

int
mks_make_socket_nonblocking(mks_socket_t fd)
{
#ifdef _WIN32
    {
        unsigned long nonblocking = 1;
        if (ioctlsocket(fd, FIONBIO, &nonblocking) == SOCKET_ERROR) {
            //event_sock_warn(fd, "fcntl(%d, F_GETFL)", (int)fd);
            return -1;
        }
    }
#else
    {
		int flags;
		if ((flags = fcntl(fd, F_GETFL, nullptr)) < 0) {
			//event_warn("fcntl(%d, F_GETFL)", fd);
			return -1;
		}
		if (!(flags & O_NONBLOCK)) {
			if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                //event_warn("fcntl(%d, F_SETFL)", fd);
				return -1;
			}
		}
	}
#endif
    return 0;
}

/* Faster version of mks_make_socket_nonblocking for internal use.
 *
 * Requires that no F_SETFL flags were previously set on the fd.
 */
int
mks_fast_socket_nonblocking(mks_socket_t fd) {
#ifdef _WIN32
    return mks_make_socket_nonblocking(fd);
#else
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		//event_warn("fcntl(%d, F_SETFL)", fd);
		return -1;
	}
	return 0;
#endif
}

}