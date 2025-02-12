#include "epollwrapper.h"
#include "server.h"

#include <gmock/gmock.h>

class MockEpollWrapper : public IEpollWrapper {
public:
    MOCK_METHOD(void, add_conn, (int const fd), (noexcept, override));
    MOCK_METHOD(void, remove_conn, (int const fd), (noexcept, override));
    MOCK_METHOD(void, modify_conn, (int const fd, uint32_t const event_flags), (const, noexcept, override));
    MOCK_METHOD(int, wait, (), (noexcept, override));
    MOCK_METHOD(epoll_event&, get_event, (int idx), (override));
    MOCK_METHOD(Connection&, get_connection, (int fd), (override));
};

using ::testing::AtLeast;

TEST(SERVER_TEST, EpollWrapperTracksServerFd) {
    int const DUMMY_PORT{ 8080 };
    auto mock_epoll = std::make_unique<MockEpollWrapper>();
    auto epoll_ptr = mock_epoll.get();

    Server server(DUMMY_PORT, std::move(mock_epoll));

    EXPECT_CALL(*epoll_ptr, add_conn(::testing::_))
        .Times(1)
        .WillOnce([&server]()
            {
                server.stop();
                return 0;
            });

    server.start();
}
