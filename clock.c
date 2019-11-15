#include <string.h>
#include <errno.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <X11/Xlib.h>

#define PERIOD 60

Display *dpy;

void set_time(time_t time) {
  struct tm tm;
  char status[256];
  int screen;
  Window root;

  tm = *localtime(&time);
  sprintf(status, "%4d-%02d-%02d %02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min);

  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);
  XStoreName(dpy, root, status);
  XFlush(dpy);
  printf("%s\n", status);
}

int main() {
  struct itimerspec value;
  struct epoll_event ev;
  int res;

  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    printf("Failed to open display");
  }

  res = clock_gettime(CLOCK_REALTIME, &value.it_value);
  if (res < 0) {
    printf("Failed to get time");
    return 1;
  }
  printf("%ld\n", value.it_value.tv_sec);

  int timer = timerfd_create(CLOCK_REALTIME, 0);
  value.it_value.tv_sec = value.it_value.tv_sec + (PERIOD - value.it_value.tv_sec % PERIOD);
  value.it_value.tv_nsec = 0;
  value.it_interval.tv_sec = 0;
  value.it_interval.tv_nsec = 0;

  res = timerfd_settime(timer, TFD_TIMER_ABSTIME, &value, NULL);
  if (res < 0) {
    printf("Failed set timer: %s", strerror(errno));
    return 1;
  }

  int epollfd = epoll_create(1);
  ev.events = EPOLLIN;
  ev.data.fd = timer;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, timer, &ev);
  //timerfd_gettime(timer, &curvalue);
  for(;;) {
    int nfds = epoll_wait(epollfd, &ev, 1, -1);
    if (ev.data.fd == timer) {
      time_t val;
      res = read(timer, &val, sizeof(val));
      clock_gettime(CLOCK_REALTIME, &value.it_value);
      set_time(value.it_value.tv_sec);

      value.it_value.tv_sec += PERIOD - value.it_value.tv_sec % PERIOD;
      timerfd_settime(timer, TFD_TIMER_ABSTIME, &value, NULL);
    }
  }
}
