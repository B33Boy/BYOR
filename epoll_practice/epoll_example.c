#define MAX_EVENTS 5
#define READ_SIZE 10

#include <stdio.h>     // for fprintf()
#include <unistd.h>    // for close(), read()
#include <sys/epoll.h> // for epoll_create1(), epoll_ctl(), struct epoll_event
#include <string.h>    // for strncmp

/**
* In edge triggered mode we will only receive events when the state of the watched file descriptors change
* In level triggered mode we will continue to receive events until the fd is not in ready state.
* This is example of level triggered
**/

int main()
{
    struct epoll_event event = { .events = EPOLLIN, .data.fd = 0 };
    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1)
    {
        fprintf(stderr, "failed to create epoll fd\n");
        return 1;
    }

    // 0 is the standard input file descriptor
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event))
    {
        fprintf(stderr, "Failed to add file descriptor to epoll\n");
        close(epoll_fd);
        return 1;
    }

    int running = 1;

    char read_buffer[READ_SIZE + 1];
    struct epoll_event events[MAX_EVENTS];

    while (running)
    {
        printf("\npolling for input...\n");

        // Returns num of fds ready for the requested IO operation within timeout period
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 30000);
        printf("%d ready events\n", event_count);

        for (int i = 0; i < event_count; i++)
        {
            printf("Reading fd '%d' -- ", events[i].data.fd);
            size_t bytes_read = read(events[i].data.fd, read_buffer, READ_SIZE);
            printf("%zd bytes read.\n", bytes_read);
            read_buffer[bytes_read] = '\0'; // add termination 
            printf("Read '%s'\n", read_buffer);
            if (!strncmp(read_buffer, "stop\n", 5))
                running = 0;
        }
    }


    if (close(epoll_fd) != 0)
    {
        fprintf(stderr, "failed to close epoll fd\n");
        return 1;
    }


    return 0;
}