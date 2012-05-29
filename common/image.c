#include <common.h>
#include <image.h>
#include <errno.h>

int image_move_unpack(const image_info_t *os, union image_entry_point *ep)
{
    switch (os->comp) {
	case IH_COMP_NONE:
	    if (os->payload_load == os->payload_start) {
		    debug("IMG can execute image in place: type %d load 0x%08lX\n",
			    os->type, os->payload_load);
	    } else {
		    debug("IMG moving image: type %d from 0x%08lX to 0x%08lX\n",
			    os->type, os->payload_start, os->payload_load);
		    memmove((void *)os->payload_load, (void *)os->payload_start,
			    os->payload_len);
	    }
	    break;

	default:
	    printf("IMG only uncompressed images are supported\n");
	    return -EINVAL;
    }

    ep->addr = os->entry;
    return 0;
}

int image_guess_size(ulong addr, size_t *sz)
{
	int ret;
	ret = legacy_image_guess_size(addr, sz);
	if(ret == 0) return 0;
#if 0
	ret = some_othere_image_guess_size(addr, sz);
	if(ret == 0) return 0;
	ret = yet another_image_guess_size(addr, sz);
	if(ret == 0) return 0;
#endif
	return ret;
}

/* Scans for valid image. Returns 0 on success. In case of error, iminfo should
 * not be changed */
int image_scan(ulong addr, struct image_info *iminfo)
{
	int ret;
	ret = legacy_image_scan(addr, iminfo);
	if(ret == 0) return ret;
#if 0
	ret = other_image_scan(addr, iminfo);
	if(ret == 0) return 0;
	ret = yet_another_image_scan(addr, iminfo);
	if(ret == 0) return 0;
#endif
	return ret;
}

