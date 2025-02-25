#include "server.h"

#include <gmock/gmock.h>

// ======================================== Mocks ========================================

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

// ======================================== Util ========================================

#define AnyValue ::testing::_

template <typename T>
auto Return(T&& val)
{
    return ::testing::Return(std::forward<T>(val));
}

template <typename T>
auto ReturnRef(T& val)
{
    return ::testing::ReturnRef(val);
}

// ======================================== Test Fixture ========================================

class ServerTest : public ::testing::Test
{
protected:
    static constexpr uint16_t DUMMY_PORT{ 8080 };
    static constexpr int EXPECTED_SERVER_FD{ 5 };
    static constexpr int EXPECTED_CLIENT_FD{ 6 };
    static constexpr int NUM_EVENTS{ 1 };

    MockEpollWrapper mock_epoll;
    MockSocketWrapper mock_sock;

    Server<MockSocketWrapper, MockEpollWrapper> server;

    ServerTest() : server(DUMMY_PORT, mock_sock, mock_epoll)
    {
    }

    void SetUp() override
    {
        ON_CALL(mock_sock, socket_impl(AnyValue, AnyValue, AnyValue)).WillByDefault(Return(EXPECTED_SERVER_FD));
    }

    void TearDown() override
    {
    }
};

// clang-format off
TEST_F(ServerTest, EpollWrapperTracksServerFd)
{
    // Act/Assert
    EXPECT_CALL(mock_epoll, add_conn_impl(EXPECTED_SERVER_FD))
        .Times(1)
        .WillOnce([this]() { this->server.stop(); });
    server.start();
}

TEST_F(ServerTest, ServerHandlesNewClientConnection)
{

    // Arrange
    EXPECT_CALL(mock_epoll, add_conn_impl(EXPECTED_SERVER_FD))
        .Times(1);

    int const NUM_EVENTS{ 1 };
    EXPECT_CALL(mock_epoll, wait_impl())
        .Times(1)
        .WillOnce(Return(NUM_EVENTS));

    epoll_event NEW_CLIENT_EVENT{};
    NEW_CLIENT_EVENT.data.fd = EXPECTED_SERVER_FD;
    ON_CALL(mock_epoll, get_event_impl(AnyValue))
        .WillByDefault(ReturnRef(NEW_CLIENT_EVENT));

    // Act/Assert
    ON_CALL(mock_sock, accept_impl(EXPECTED_SERVER_FD, AnyValue, AnyValue))
        .WillByDefault(Return(EXPECTED_CLIENT_FD));

    EXPECT_CALL(mock_epoll, add_conn_impl(EXPECTED_CLIENT_FD))
        .Times(1)
        .WillOnce([this]() { this->server.stop(); });

    server.start();
}

// clang-format on
