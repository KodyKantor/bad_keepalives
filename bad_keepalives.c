#include <curl/curl.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>

typedef struct cookie {
	unsigned long long bdone; /* bytes done */
	unsigned long long ndone; /* buffers done */
	void *buf;
	int verbose;
} cookie_t;

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
	cookie_t *cookie = (cookie_t *) userdata;
	if (cookie->verbose) {
		printf("[write_cb] size: %u, nmemb: %u, done: %u\n",
		    size, nmemb, cookie->bdone);
	}

	cookie->bdone += size * nmemb;
	cookie->ndone += 1;
	return (size * nmemb);
}

/* Copies from the random buffer to the buffer provided by libcurl.
 *
 * By default libcurl only gives us a 64k buffer, and we don't advance the
 * pointer into the random buffer that we allocate. That means we basically
 * write the same 64k randomness over and over again instead of writing the
 * entire random buffer that we allocate (if it's more than 64k). This fact
 * doesn't matter for illustrating the nginx bug.
 */
size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
	cookie_t *cookie = (cookie_t *) userdata;
	if (cookie->verbose) {
		printf("[read_cb] size: %u, nitems: %u, done: %u\n",
		    size, nitems, cookie->bdone);
	}

	memcpy(buffer, cookie->buf, size * nitems);
	cookie->bdone += size * nitems;
	cookie->ndone += 1;
	return (size * nitems);
}

int main(int argc, char *argv[]) {
	char *target = argv[1]; /* The nginx server. */
	curl_off_t uploadsize = atoi(argv[2]); /* Upload size, in bytes. */

	CURL *curl = curl_easy_init();
	cookie_t cookie;

	int rand = 0;
	int verbose = 0;
	ssize_t nread = 0;
	char *buf = NULL;
	char url[128];

	rand = open("/dev/urandom", O_RDONLY);
	if (rand <= 0) {
		printf("could not open urandom\n");
		return (-1);
	}

	buf = malloc(uploadsize);
	if (buf == NULL) {
		printf("failed to allocate space for random buffer\n");
		return (-1);
	}

	nread = read(rand, buf, uploadsize);
	(void) close(rand);
	if (nread != uploadsize) {
		printf("read from urandom did not match upload size\n");
		return (-1);
	}

	(void) snprintf(url, 128, "http://%s/chum/target_object", target);

	if (curl) {
		CURLcode res;
		curl_easy_setopt(curl, CURLOPT_URL, url);

		/* Upload a seed object. */
		bzero(&cookie, sizeof(cookie_t));
		cookie.buf = buf;
		cookie.verbose = verbose;

		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl, CURLOPT_READDATA, &cookie);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, uploadsize);

		printf("Upload: begin\n");
		res = curl_easy_perform(curl);
		if (res != 0) {
			printf("problem uploading: %d\n", res);
			return (-1);
		}
		printf("Upload: end. Uploaded %llu bytes, %llu chunks\n",
		    cookie.bdone, cookie.ndone);

		/* Need to clear the curl settings - the handle is reused. */
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, NULL);
		curl_easy_setopt(curl, CURLOPT_READDATA, NULL);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, NULL);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cookie);

		while (1) {
			/* Download the seed object. */
			bzero(&cookie, sizeof(cookie_t));
			cookie.verbose = verbose;

			printf("Download: begin\n");
			res = curl_easy_perform(curl);
			if (res != 0) {
				printf("problem downloading: %d\n", res);
				return (-1);
			}
			printf("Download: end. Downloaded %llu bytes, "
				"%llu chunks\n", cookie.bdone, cookie.ndone);
			sleep(1);
		}
		curl_easy_cleanup(curl);
	}
}
