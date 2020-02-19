# bad_keepalives

A simple libcurl program to illustrate bad nginx keepalive behavior. See
MANTA-4994 for more information.

## Building

This was only tested on smartos using image
f3a6e1a2-9d71-11e9-9bd2-e7e5b4a5c141.

```
make
```

Tough stuff, I know. This just links to libcurl.

## Running

Make sure you have keepalives enabled in nginx. In my nginx.conf I have:
```
keepalive_timeout 10000;
keepalive_requests 10000;
```
in the `http` block. You should be able to set these to most biggish numbers.

```
./bad_keepalives ip_address_of_nginx 1048576
```

This should print something like this and hang:
```
$ ./bad_keepalives 172.29.3.13 1048576
Upload: begin
Upload: end. Uploaded 1048576 bytes, 16 chunks
Download: begin
Download: end. Downloaded 1048576 bytes, 139 chunks
Download: begin
```

If you figure out what's going wrong with nginx, please to let me know. This was
tested using Joyent's nginx fork, so nginx is pretty far out of date. The latest
upstream commit is 8c8c70b27b9a3d5a20c7ccb2f7e2f61dedf6fc47 from Oct 2016 as of
this writing.
