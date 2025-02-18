#include "epollwrapper.h"
#include "server.h"

#include <gmock/gmock.h>

class MockEpollWrapper : public IEpollWrapper
{
public:
    MOCK_METHOD(void, add_conn, (int const fd), (noexcept, override));
    MOCK_METHOD(void, remove_conn, (int const fd), (noexcept, override));
    MOCK_METHOD(void, modify_conn, (int const fd, uint32_t const event_flags), (const, noexcept, override));
    MOCK_METHOD(int, wait, (), (noexcept, override));
    MOCK_METHOD(epoll_event&, get_event, (int idx), (override));
    MOCK_METHOD(Connection&, get_connection, (int fd), (override));
};

class MockSocketWrapper : public ISocketWrapper
{
public:
    MOCK_METHOD(int, socket, (int domain, int type, int protocol), (const, override));
    MOCK_METHOD(int, bind, (int fd, sockaddr const* addr, socklen_t addrlen), (const, override));
    MOCK_METHOD(int, listen, (int fd, int backlog), (const, override));
    MOCK_METHOD(int, accept, (int fd, sockaddr* addr, socklen_t* addrlen), (const, override));
    MOCK_METHOD(int, close, (int fd), (const, override));
    MOCK_METHOD(int, setsockopt, (int fd), (const, override));
    MOCK_METHOD(int, fcntl, (int fd, int op), (const, override));
    MOCK_METHOD(int, fcntl, (int fd, int op, int arg), (const, override));
};

TEST(SERVER_TEST, EpollWrapperTracksServerFd)
{
    int const DUMMY_PORT{ 8080 };
    auto mock_epoll = std::make_unique<MockEpollWrapper>();
    auto epoll_ptr = mock_epoll.get();

    auto mock_socket_wrapper = std::make_unique<MockSocketWrapper>();

    Server server(DUMMY_PORT, std::move(mock_socket_wrapper), std::move(mock_epoll));

    EXPECT_CALL(*epoll_ptr, add_conn(::testing::_)).Times(1).WillOnce([&server]() { server.stop(); });

    server.start();
}
