/*
 * Copyright 2008-2017 Various Authors
 * Copyright 2004-2005 Timo Hirvonen
 *
 * obsd.c by Jakob Jungmann <jungmann0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "op.h"
#include "sf.h"
#include "xmalloc.h"

static sample_format_t obsd_sf;

static int obsd_fd = -1;
static int obsdctl_fd = -1;

static char *obsd_audio_device = NULL;

static int obsd_reset(void)
{
	if (ioctl(obsdctl_fd, AUDIO_STOP, NULL) == -1)
		return -1;

	return 0;
}

static int obsd_set_sf(sample_format_t sf)
{
	struct audio_swpar ap;

	AUDIO_INITPAR(&ap);

	obsd_reset();
	obsd_sf = sf;

	ap.pchan = sf_get_channels(obsd_sf);
	ap.rate = sf_get_rate(obsd_sf);

	switch (sf_get_bits(obsd_sf)) {
	case 16:
		ap.bits = 16;

		if (sf_get_signed(obsd_sf))
			ap.sig = 1;
		else
			ap.sig = 0;
		if (sf_get_bigendian(obsd_sf))
			ap.le = 0;
		else
			ap.le = 1;
		break;
	case 8:
		ap.bits = 8;

		if (sf_get_signed(obsd_sf))
			ap.sig = 1;
		else
			ap.sig = 0;
		break;
	default:
		return -1;
	}

	ap.nblks = 8;

	if (ioctl(obsd_fd, AUDIO_SETPAR, &ap) == -1)
		return -1;

	if (ioctl(obsd_fd, AUDIO_GETPAR, &ap) == -1)
		return -1;

	/* FIXME: check if sample rate is supported */
	return 0;
}

static int obsd_device_exists(const char *dev)
{
	struct stat s;

	if (stat(dev, &s))
		return 0;
	return 1;
}

static int obsd_init(void)
{
	const char *audio_dev = "/dev/audio";

	if (!obsd_device_exists("/dev/audioctl"))
		return -1;

	if (obsd_audio_device != NULL) {
		if (obsd_device_exists(obsd_audio_device))
			return 0;
		free(obsd_audio_device);
		obsd_audio_device = NULL;
		return -1;
	}
	if (obsd_device_exists(audio_dev)) {
		obsd_audio_device = xstrdup(audio_dev);
		return 0;
	}

	return -1;
}

static int obsd_exit(void)
{
	if (obsd_audio_device != NULL) {
		free(obsd_audio_device);
		obsd_audio_device = NULL;
	}

	return 0;
}

static int obsd_close(void)
{
	if (obsd_fd != -1) {
		close(obsd_fd);
		obsd_fd = -1;
	}

	if (obsdctl_fd != -1) {
		close(obsdctl_fd);
		obsdctl_fd = -1;
	}

	return 0;
}

static int obsd_open(sample_format_t sf, const channel_position_t *channel_map)
{
	obsdctl_fd = open("/dev/audioctl", O_WRONLY);
	obsd_fd = open(obsd_audio_device, O_WRONLY);
	if (obsdctl_fd == -1)
		return -1;
	if (obsd_fd == -1)
		return -1;
	if (obsd_set_sf(sf) == -1) {
		obsd_close();
		return -1;
	}

	return 0;
}

static int obsd_write(const char *buf, int cnt)
{
	const char *t;

	t = buf;
	while (cnt > 0) {
		int rc = write(obsd_fd, buf, cnt);
		if (rc == -1) {
			if (errno == EINTR)
				continue;
			else
				return rc;
		}
		buf += rc;
		cnt -= rc;
	}

	return (buf - t);
}

static int obsd_pause(void)
{
	if (ioctl(obsdctl_fd, AUDIO_STOP, NULL) == -1)
		return -1;

	return 0;
}

static int obsd_unpause(void)
{
	struct audio_swpar ap;

        AUDIO_INITPAR(&ap);

	if (ioctl(obsdctl_fd, AUDIO_START, NULL) == -1)
		return -1;

	return 0;
}

static int obsd_buffer_space(void)
{
	int sp;
	struct audio_swpar ap;
                        
        AUDIO_INITPAR(&ap);

	if (ioctl(obsdctl_fd, AUDIO_GETPAR, &ap) == -1)
		return -1;

	struct audio_pos pos;
	if (ioctl(obsd_fd, AUDIO_GETPOS, &pos) == -1)
		return -1;

	sp = ap.nblks * ap.round * ap.bps;

	return sp;
}

static int op_obsd_set_device(const char *val)
{
	free(obsd_audio_device);
	obsd_audio_device = xstrdup(val);
	return 0;
}

static int op_obsd_get_device(char **val)
{
	if (obsd_audio_device)
		*val = xstrdup(obsd_audio_device);
	return 0;
}

const struct output_plugin_ops op_pcm_ops = {
	.init = obsd_init,
	.exit = obsd_exit,
	.open = obsd_open,
	.close = obsd_close,
	.write = obsd_write,
	.pause = obsd_pause,
	.unpause = obsd_unpause,
	.buffer_space = obsd_buffer_space,
};

const struct output_plugin_opt op_pcm_options[] = {
	OPT(op_obsd, device),
	{ NULL },
};

const int op_priority = 0;
const unsigned op_abi_version = OP_ABI_VERSION;
