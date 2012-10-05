# include "config.h"
#include "beecrypt/entropy.h"
#include "beecrypt/endianness.h"
#  include <sys/time.h>
#  include <sys/stat.h>
#  include <time.h>
#include "beecrypt/fips186.h"
# include <fcntl.h>
#include <errno.h>

/*!\addtogroup ES_urandom_m
 * \{
 */
static const char* name_dev_urandom = "/dev/urandom";
static int dev_urandom_fd = -1;
# ifdef _REENTRANT
#  if HAVE_THREAD_H && HAVE_SYNCH_H
static mutex_t dev_urandom_lock = DEFAULTMUTEX;
#  elif HAVE_PTHREAD_H
static pthread_mutex_t dev_urandom_lock = PTHREAD_MUTEX_INITIALIZER;
#  else
#   error Need locking mechanism
#  endif
# endif
/*!\}
 */

static int statdevice(const char *device)
{
	struct stat s;

	if (stat(device, &s) < 0)
	{
		#if HAVE_ERRNO_H && HAVE_STRING_H
		fprintf(stderr, "cannot stat %s: %s\n", device, strerror(errno));
		#endif
		return -1;
	}
	if (!S_ISCHR(s.st_mode))
	{
		fprintf(stderr, "%s is not a device\n", device);
		return -1;
	}
	return 0;
}

static int opendevice(const char *device)
{
	register int fd;

	if ((fd = open(device, O_RDONLY)) < 0)
	{
		#if HAVE_ERRNO_H && HAVE_STRING_H
		fprintf(stderr, "open of %s failed: %s\n", device, strerror(errno));
		#endif
		return fd;
	}
	
	return fd;
}


int entropy_dev_urandom(byte* data, size_t size);

static entropySource entropySourceList[] =
{
	{ "urandom", entropy_dev_urandom },
};

#define ENTROPYSOURCES (sizeof(entropySourceList) / sizeof(entropySource))


int randomGeneratorContextInit(randomGeneratorContext* ctxt, const randomGenerator* rng)
{
	if (ctxt == (randomGeneratorContext*) 0)
		return -1;

	if (rng == (randomGenerator*) 0)
		return -1;

	ctxt->rng = rng;

	if (rng->paramsize)
	{
		ctxt->param = (randomGeneratorParam*) calloc(rng->paramsize, 1);
		if (ctxt->param == (randomGeneratorParam*) 0)
			return -1;
	}
	else
		ctxt->param = (randomGeneratorParam*) 0;

	return ctxt->rng->setup(ctxt->param);
}

const randomGenerator* randomGeneratorDefault()
{
  return &fips186prng;
}

int entropyGatherNext(byte* data, size_t size)
{
  register int index;
  
  for (index = 0; index < ENTROPYSOURCES; index++) {
    if (entropySourceList[index].next(data, size) == 0)
      return 0;
  }
  return -1;
}


static int entropy_randombits(int fd, int timeout, byte* data, size_t size)
{
	register int rc;

	#if ENABLE_AIO
	struct aiocb my_aiocb;
	const struct aiocb* my_aiocb_list = &my_aiocb;
	# if HAVE_TIME_H
	struct timespec my_aiocb_timeout;
	# else
	#  error
	# endif

	memset(&my_aiocb, 0, sizeof(struct aiocb));

	my_aiocb.aio_fildes = fd;
	my_aiocb.aio_sigevent.sigev_notify = SIGEV_NONE;
	#endif

	while (size)
	{
		#if ENABLE_AIO
		my_aiocb.aio_buf = data;
		my_aiocb.aio_nbytes = size;

		rc = aio_read(&my_aiocb);
		#else
		rc = read(fd, data, size);
		#endif

		if (rc < 0)
			return -1;

		#if ENABLE_AIO
		my_aiocb_timeout.tv_sec = (timeout / 1000);
		my_aiocb_timeout.tv_nsec = (timeout % 1000) * 1000000;

		rc = aio_suspend(&my_aiocb_list, 1, &my_aiocb_timeout);

		if (rc < 0)
		{
			#if HAVE_ERRNO_H
			if (errno == EAGAIN)
			{
				/* certain linux glibc versions are buggy and don't aio_suspend properly */
				nanosleep(&my_aiocb_timeout, (struct timespec*) 0);

				my_aiocb_timeout.tv_sec = 0;
				my_aiocb_timeout.tv_nsec = 0;

				/* and try again */
				rc = aio_suspend(&my_aiocb_list, 1, &my_aiocb_timeout);
			}
			#endif
		}

		if (rc < 0)
		{
			/* cancel any remaining reads */
			while (rc != AIO_ALLDONE)
			{
				rc = aio_cancel(fd, (struct aiocb*) 0);

				if (rc == AIO_NOTCANCELED)
				{
					my_aiocb_timeout.tv_sec = (timeout / 1000);
					my_aiocb_timeout.tv_nsec = (timeout % 1000) * 1000000;

					nanosleep(&my_aiocb_timeout, (struct timespec*) 0);
				}

				if (rc < 0)
					break;
			}

			return -1;
		}

		rc = aio_error(&my_aiocb);

		if (rc < 0)
			return -1;

		rc = aio_return(&my_aiocb);

		if (rc < 0)
			return -1;
		#endif

		data += rc;
		size -= rc;
	}
	return 0;
}


int entropy_dev_urandom(byte* data, size_t size) {
  const char* timeout_env = getenv("BEECRYPT_ENTROPY_URANDOM_TIMEOUT");

  register int rc;

#ifdef _REENTRANT
# if HAVE_THREAD_H && HAVE_SYNCH_H
  if (mutex_lock(&dev_urandom_lock))
    return -1;
# elif HAVE_PTHREAD_H
  if (pthread_mutex_lock(&dev_urandom_lock))
    return -1;
# endif
#endif

#if HAVE_SYS_STAT_H
  if ((rc = statdevice(name_dev_urandom)) < 0)
    goto dev_urandom_end;
#endif
  
  if ((rc = dev_urandom_fd = opendevice(name_dev_urandom)) < 0)
    goto dev_urandom_end;

  /* collect entropy, with timeout */
  rc = entropy_randombits(dev_urandom_fd, timeout_env ? atoi(timeout_env) : 1000, data, size);
  
  close(dev_urandom_fd);

 dev_urandom_end:
#ifdef _REENTRANT
# if HAVE_THREAD_H && HAVE_SYNCH_H
  mutex_unlock(&dev_urandom_lock);
# elif HAVE_PTHREAD_H
  pthread_mutex_unlock(&dev_urandom_lock);
# endif
#endif
  return rc;
}

