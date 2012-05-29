#include <image.h>
#include <errno.h>
#include <common.h>

#define uimage_to_cpu(x)		be32_to_cpu(x)
#define cpu_to_uimage(x)		cpu_to_be32(x)

struct legacy_image_header {
	uint32_t	ih_magic;	/* Image Header Magic Number	*/
	uint32_t	ih_hcrc;	/* Image Header CRC Checksum	*/
	uint32_t	ih_time;	/* Image Creation Timestamp	*/
	uint32_t	ih_size;	/* Image Data Size		*/
	uint32_t	ih_load;	/* Data	 Load  Address		*/
	uint32_t	ih_ep;		/* Entry Point Address		*/
	uint32_t	ih_dcrc;	/* Image Data CRC Checksum	*/
	uint8_t		ih_os;		/* Operating System		*/
	uint8_t		ih_arch;	/* CPU architecture		*/
	uint8_t		ih_type;	/* Image Type			*/
	uint8_t		ih_comp;	/* Compression Type		*/
	uint8_t		ih_name[IH_NMLEN];	/* Image Name		*/
};

typedef struct legacy_image_header legacy_image_header_t;

#define image_get_hdr_l(f) \
	static inline uint32_t limage_get_##f(const legacy_image_header_t *hdr) \
	{ \
		return uimage_to_cpu (hdr->ih_##f); \
	}
image_get_hdr_l (magic)		/* image_get_magic */
image_get_hdr_l (hcrc)		/* image_get_hcrc */
image_get_hdr_l (time)		/* image_get_time */
image_get_hdr_l (size)		/* image_get_size */
image_get_hdr_l (load)		/* image_get_load */
image_get_hdr_l (ep)		/* image_get_ep */
image_get_hdr_l (dcrc)		/* image_get_dcrc */

#define image_get_hdr_b(f) \
	static inline uint8_t limage_get_##f(const legacy_image_header_t *hdr) \
	{ \
		return hdr->ih_##f; \
	}
image_get_hdr_b (os)		/* image_get_os */
image_get_hdr_b (arch)		/* image_get_arch */
image_get_hdr_b (type)		/* image_get_type */
image_get_hdr_b (comp)		/* image_get_comp */

#define image_set_hdr_l(f) \
	static inline void limage_set_##f(legacy_image_header_t *hdr, uint32_t val) \
	{ \
		hdr->ih_##f = cpu_to_uimage (val); \
	}
image_set_hdr_l (magic)		/* image_set_magic */
image_set_hdr_l (hcrc)		/* image_set_hcrc */
image_set_hdr_l (time)		/* image_set_time */
image_set_hdr_l (size)		/* image_set_size */
image_set_hdr_l (load)		/* image_set_load */
image_set_hdr_l (ep)		/* image_set_ep */
image_set_hdr_l (dcrc)		/* image_set_dcrc */

#define image_set_hdr_b(f) \
	static inline void limage_set_##f(legacy_image_header_t *hdr, uint8_t val) \
	{ \
		hdr->ih_##f = val; \
	}
image_set_hdr_b (os)		/* image_set_os */
image_set_hdr_b (arch)		/* image_set_arch */
image_set_hdr_b (type)		/* image_set_type */
image_set_hdr_b (comp)		/* image_set_comp */



static inline uint32_t limage_get_header_size (void)
{
	return (sizeof (legacy_image_header_t));
}
static inline uint32_t limage_get_data (const legacy_image_header_t *hdr)
{
	return ((uint32_t)hdr + limage_get_header_size ());
}
static inline uint32_t limage_get_image_size (const legacy_image_header_t *hdr)
{
	return (limage_get_size(hdr) + limage_get_header_size ());
}
static inline uint32_t limage_get_image_end (const legacy_image_header_t *hdr)
{
	return ((ulong)hdr + limage_get_image_size (hdr));
}
static inline uint32_t limage_get_data_size (const legacy_image_header_t *hdr)
{
	return limage_get_size (hdr);
}

static inline int limage_check_magic (const legacy_image_header_t *hdr)
{
	return (limage_get_magic (hdr) == IH_MAGIC);
}
static inline int limage_check_type (const legacy_image_header_t *hdr, uint8_t type)
{
	return (limage_get_type (hdr) == type);
}
static inline int limage_check_arch (const legacy_image_header_t *hdr, uint8_t arch)
{
	return (limage_get_arch (hdr) == arch);
}
static inline int limage_check_os (const legacy_image_header_t *hdr, uint8_t os)
{
	return (limage_get_os (hdr) == os);
}

int limage_check_hcrc(const legacy_image_header_t *hdr, ulong *pcrc)
{
	ulong hcrc;
	legacy_image_header_t header;
	ulong len = limage_get_header_size();

	/* Copy header so we can blank CRC field for re-calculation */
	memmove(&header, (char *)hdr, len);
	limage_set_hcrc(&header, 0);

	hcrc = crc32 (0, (unsigned char *)&header, len);
	if(pcrc) *pcrc = hcrc;

	return (hcrc == limage_get_hcrc(hdr));
}

int limage_check_dcrc (const legacy_image_header_t *hdr, ulong* pdcrc)
{
	ulong data = limage_get_data (hdr);
	ulong len = limage_get_data_size (hdr);
	ulong dcrc = crc32(0, (unsigned char *)data, len);
	if(*pdcrc) *pdcrc = dcrc;
	return (dcrc == limage_get_dcrc (hdr));
}

static inline int limage_check_target_arch (const legacy_image_header_t *hdr)
{
#if defined(__ARM__)
	if (!limage_check_arch (hdr, IH_ARCH_ARM))
#elif defined(__avr32__)
	if (!limage_check_arch (hdr, IH_ARCH_AVR32))
#elif defined(__bfin__)
	if (!limage_check_arch (hdr, IH_ARCH_BLACKFIN))
#elif defined(__I386__)
	if (!limage_check_arch (hdr, IH_ARCH_I386))
#elif defined(__M68K__)
	if (!limage_check_arch (hdr, IH_ARCH_M68K))
#elif defined(__microblaze__)
	if (!limage_check_arch (hdr, IH_ARCH_MICROBLAZE))
#elif defined(__mips__)
	if (!limage_check_arch (hdr, IH_ARCH_MIPS))
#elif defined(__nios2__)
	if (!limage_check_arch (hdr, IH_ARCH_NIOS2))
#elif defined(__PPC__)
	if (!limage_check_arch (hdr, IH_ARCH_PPC))
#elif defined(__sh__)
	if (!limage_check_arch (hdr, IH_ARCH_SH))
#elif defined(__sparc__)
	if (!limage_check_arch (hdr, IH_ARCH_SPARC))
#else
# error Unknown CPU type
#endif
		return 0;

	return 1;
}

int is_legacy_image(ulong addr)
{
	legacy_image_header_t *hdr = (legacy_image_header_t *)addr;
	return (limage_get_magic(hdr) == IH_MAGIC) ? 1 : 0;
}

legacy_image_header_t* legacy_image_check(ulong addr)
{
	legacy_image_header_t *hdr = (legacy_image_header_t *)addr;
	if (limage_get_magic(hdr) != IH_MAGIC) {
		puts("Bad magic\n");
		return NULL;
	}

	if (!limage_check_hcrc(hdr, NULL)) {
		puts ("Bad Header Checksum\n");
		return NULL;
	}

	if (!limage_check_dcrc(hdr, NULL)) {
		puts ("Bad data crc\n");
		return NULL;
	}

	if (!limage_check_target_arch(hdr)) {
		printf ("Unsupported Architecture 0x%x\n", limage_get_arch (hdr));
		return NULL;
	}
	return hdr;
}

int legacy_image_scan(ulong addr, struct image_info *iminfo)
{
	legacy_image_header_t *hdr;

	hdr = legacy_image_check(addr);
	if (!hdr) {
		printf("Not a legacy image\n");
		return -EINVAL;
	}

	switch (limage_get_type(hdr)) {
		case IH_TYPE_KERNEL:
			iminfo->payload_start = limage_get_data (hdr);
			iminfo->payload_len = limage_get_data_size (hdr);
			iminfo->payload_load = limage_get_load (hdr);
			break;
		default:
			printf ("Only TYPE_KERNEL is supported\n");
			return -EINVAL;
	}

	iminfo->blob_start = addr;
	iminfo->blob_len = limage_get_image_size (hdr);

	iminfo->type = limage_get_type (hdr);
	iminfo->comp = limage_get_comp (hdr);
	iminfo->os = limage_get_os (hdr);

	iminfo->entry = limage_get_ep (hdr);
	return 0;
}

int legacy_image_guess_size(ulong addr, size_t *sz)
{
	legacy_image_header_t *hdr = (legacy_image_header_t *)addr;
	if (limage_get_magic(hdr) != IH_MAGIC) {
		return -EINVAL;
	}

	if (!limage_check_hcrc(hdr, NULL)) {
		return -EINVAL;
	}

	*sz = limage_get_image_size(hdr);
	return 0;
}

