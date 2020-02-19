all:
	gcc -o bad_keepalives bad_keepalives.c -lcurl

clean:
	rm bad_keepalives
