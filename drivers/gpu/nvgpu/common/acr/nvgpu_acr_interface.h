/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef NVGPU_ACR_INTERFACE_H
#define NVGPU_ACR_INTERFACE_H

/* BLOB construct interface */

/*
 * Light Secure WPR Content Alignments
 */
#define LSF_WPR_HEADER_ALIGNMENT        (256U)
#define LSF_SUB_WPR_HEADER_ALIGNMENT    (256U)
#define LSF_LSB_HEADER_ALIGNMENT        (256U)
#define LSF_BL_DATA_ALIGNMENT           (256U)
#define LSF_BL_DATA_SIZE_ALIGNMENT      (256U)
#define LSF_BL_CODE_SIZE_ALIGNMENT      (256U)
#define LSF_DATA_SIZE_ALIGNMENT         (256U)
#define LSF_CODE_SIZE_ALIGNMENT         (256U)

#define LSF_UCODE_DATA_ALIGNMENT 4096U

/*
 * Maximum WPR Header size
 */
#define LSF_WPR_HEADERS_TOTAL_SIZE_MAX	\
	(ALIGN_UP(((u32)sizeof(struct lsf_wpr_header) * FALCON_ID_END), \
		LSF_WPR_HEADER_ALIGNMENT))
#define LSF_LSB_HEADER_TOTAL_SIZE_MAX	(\
	ALIGN_UP(sizeof(struct lsf_lsb_header), LSF_LSB_HEADER_ALIGNMENT))

#ifdef CONFIG_NVGPU_DGPU
/* Maximum SUB WPR header size */
#define LSF_SUB_WPR_HEADERS_TOTAL_SIZE_MAX	(ALIGN_UP( \
	(sizeof(struct lsf_shared_sub_wpr_header) * \
	LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_MAX), \
	LSF_SUB_WPR_HEADER_ALIGNMENT))

/* MMU excepts sub_wpr sizes in units of 4K */
#define SUB_WPR_SIZE_ALIGNMENT	(4096U)

/* Defined for 1MB alignment */
#define SHIFT_4KB	(12U)

/* shared sub_wpr use case IDs */
enum {
	LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_FRTS_VBIOS_TABLES	= 1,
	LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_PLAYREADY_SHARED_DATA = 2
};

#define LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_MAX \
	LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_PLAYREADY_SHARED_DATA

#define LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_INVALID	(0xFFFFFFFFU)

#define MAX_SUPPORTED_SHARED_SUB_WPR_USE_CASES	\
	LSF_SHARED_DATA_SUB_WPR_USE_CASE_ID_MAX

/* Static sizes of shared subWPRs */
/* Minimum granularity supported is 4K */
/* 1MB in 4K */
#define LSF_SHARED_DATA_SUB_WPR_FRTS_VBIOS_TABLES_SIZE_IN_4K	(0x100U)
/* 4K */
#define LSF_SHARED_DATA_SUB_WPR_PLAYREADY_SHARED_DATA_SIZE_IN_4K	(0x1U)
#endif

/*Light Secure Bootstrap header related defines*/
#define NV_FLCN_ACR_LSF_FLAG_LOAD_CODE_AT_0_FALSE       0U
#define NV_FLCN_ACR_LSF_FLAG_LOAD_CODE_AT_0_TRUE        BIT32(0)
#define NV_FLCN_ACR_LSF_FLAG_DMACTL_REQ_CTX_FALSE       0U
#define NV_FLCN_ACR_LSF_FLAG_DMACTL_REQ_CTX_TRUE        BIT32(2)
#define NV_FLCN_ACR_LSF_FLAG_FORCE_PRIV_LOAD_TRUE       BIT32(3)
#define NV_FLCN_ACR_LSF_FLAG_FORCE_PRIV_LOAD_FALSE      0U

/*
 * Image Status Defines
 */
#define LSF_IMAGE_STATUS_NONE                           (0U)
#define LSF_IMAGE_STATUS_COPY                           (1U)
#define LSF_IMAGE_STATUS_VALIDATION_CODE_FAILED         (2U)
#define LSF_IMAGE_STATUS_VALIDATION_DATA_FAILED         (3U)
#define LSF_IMAGE_STATUS_VALIDATION_DONE                (4U)
#define LSF_IMAGE_STATUS_VALIDATION_SKIPPED             (5U)
#define LSF_IMAGE_STATUS_BOOTSTRAP_READY                (6U)

struct lsf_wpr_header {
	u32 falcon_id;
	u32 lsb_offset;
	u32 bootstrap_owner;
	u32 lazy_bootstrap;
	u32 bin_version;
	u32 status;
};

struct lsf_ucode_desc {
	u8  prd_keys[2][16];
	u8  dbg_keys[2][16];
	u32 b_prd_present;
	u32 b_dbg_present;
	u32 falcon_id;
	u32 bsupports_versioning;
	u32 version;
	u32 dep_map_count;
	u8  dep_map[FALCON_ID_END * 2 * 4];
	u8  kdf[16];
};

struct lsf_lsb_header {
	struct lsf_ucode_desc signature;
	u32 ucode_off;
	u32 ucode_size;
	u32 data_size;
	u32 bl_code_size;
	u32 bl_imem_off;
	u32 bl_data_off;
	u32 bl_data_size;
	u32 app_code_off;
	u32 app_code_size;
	u32 app_data_off;
	u32 app_data_size;
	u32 flags;
};

struct flcn_bl_dmem_desc {
	u32    reserved[4];        /*Should be the first element..*/
	u32    signature[4];        /*Should be the first element..*/
	u32    ctx_dma;
	struct falc_u64 code_dma_base;
	u32    non_sec_code_off;
	u32    non_sec_code_size;
	u32    sec_code_off;
	u32    sec_code_size;
	u32    code_entry_point;
	struct falc_u64 data_dma_base;
	u32    data_size;
	u32 argc;
	u32 argv;
};

/* ACR HS ucode interface */

/*
 * Supporting maximum of 2 regions.
 * This is needed to pre-allocate space in DMEM
 */
#define NVGPU_FLCN_ACR_MAX_REGIONS                (2U)
#define LSF_BOOTSTRAP_OWNER_RESERVED_DMEM_SIZE    (0x200U)

/*
 * start_addr     - Starting address of region
 * end_addr       - Ending address of region
 * region_id      - Region ID
 * read_mask      - Read Mask
 * write_mask     - WriteMask
 * client_mask    - Bit map of all clients currently using this region
 */
struct flcn_acr_region_prop {
	u32 start_addr;
	u32 end_addr;
	u32 region_id;
	u32 read_mask;
	u32 write_mask;
	u32 client_mask;
	u32 shadowmMem_startaddress;
};

/*
 * no_regions   - Number of regions used.
 * region_props - Region properties
 */
struct flcn_acr_regions {
	u32 no_regions;
	struct flcn_acr_region_prop region_props[NVGPU_FLCN_ACR_MAX_REGIONS];
};

/*
 * reserved_dmem-When the bootstrap owner has done bootstrapping other falcons,
 *                and need to switch into LS mode, it needs to have its own
 *                actual DMEM image copied into DMEM as part of LS setup. If
 *                ACR desc is at location 0, it will definitely get overwritten
 *                causing data corruption. Hence we are reserving 0x200 bytes
 *                to give room for any loading data. NOTE: This has to be the
 *                first member always
 * signature     - Signature of ACR ucode.
 * wpr_region_id - Region ID holding the WPR header and its details
 * wpr_offset    - Offset from the WPR region holding the wpr header
 * regions       - Region descriptors
 * nonwpr_ucode_blob_start -stores non-WPR start where kernel stores ucode blob
 * nonwpr_ucode_blob_end   -stores non-WPR end where kernel stores ucode blob
 */
struct flcn_acr_desc {
	union {
		u32 reserved_dmem[(LSF_BOOTSTRAP_OWNER_RESERVED_DMEM_SIZE/4)];
	} ucode_reserved_space;
	u32 signatures[4];
	/*Always 1st*/
	u32 wpr_region_id;
	u32 wpr_offset;
	u32 mmu_mem_range;
	struct flcn_acr_regions regions;
	u32 nonwpr_ucode_blob_size;
	u64 nonwpr_ucode_blob_start;
	u32 dummy[4];  /* ACR_BSI_VPR_DESC */
};

#endif /* NVGPU_ACR_INTERFACE_H */