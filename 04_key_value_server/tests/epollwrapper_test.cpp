#include "server.h"

#include <gmock/gmock.h>

class MockEpollWrapper : public IEpollWrapperBase<MockEpollWrapper>
{
public:
    MOCK_METHOD(void, add_conn_impl, (int), (noexcept));
    MOCK_METHOD(void, remove_conn_impl, (int), (noexcept));
    MOCK_METHOD(void, modify_conn_impl, (int, uint32_t), (const, noexcept));
    MOCK_METHOD(int, wait_impl, (), (noexcept));
    MOCK_METHOD(epoll_event&, get_event_impl, (int), ());
    MOCK_METHOD(Connection&, get_connection_impl, (int), ());
};

class MockSocketWrapper : public ISocketWrapperBase<MockSocketWrapper>
{
public:
    MOCK_METHOD(int, socket_impl, (int, int, int), (const));
    MOCK_METHOD(int, bind_impl, (int, const sockaddr*, socklen_t), (const));
    MOCK_METHOD(int, listen_impl, (int, int), (const));
    MOCK_METHOD(int, accept_impl, (int, sockaddr*, socklen_t*), (const));
    MOCK_METHOD(int, close_impl, (int), (const));
    MOCK_METHOD(int, setsockopt_impl, (int), (const));
    MOCK_METHOD(int, fcntl_impl, (int, int), (const));
    MOCK_METHOD(int, fcntl_impl, (int, int, int), (const));
};

#define AnyValue ::testing::_

TEST(SERVER_TEST, EpollWrapperTracksServerFd)
{
    // Arrange
    uint16_t const DUMMY_PORT{ 8080 };
    int expected_fd{ 5 };

    MockEpollWrapper mock_epollwrapper;
    MockSocketWrapper mock_socketwrapper;

    Server<MockSocketWrapper, MockEpollWrapper> server(DUMMY_PORT, mock_socketwrapper, mock_epollwrapper);

    ON_CALL(mock_socketwrapper, socket_impl(AnyValue, AnyValue, AnyValue))
        .WillByDefault(::testing::Return(expected_fd));

    // Act/Assert
    EXPECT_CALL(mock_epollwrapper, add_conn_impl(expected_fd)).Times(1).WillOnce([&server]() { server.stop(); });
    server.start();
}
