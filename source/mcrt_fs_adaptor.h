/*
 * mcrt_fs_adaptor.h
 *
 *  Created on: Apr 13, 2022
 *      Author: mg
 */

#ifndef MCRT_FS_ADAPTOR_H_
#define MCRT_FS_ADAPTOR_H_

#define MCRT_FS_ADAPTOR

#include "fsl_shell.h"

static shell_status_t lfs_mount_handler(int32_t argc, char **argv);
static shell_status_t lfs_unmount_handler(int32_t argc, char **argv);
static shell_status_t lfs_ls_handler(int32_t argc, char **argv);
static shell_status_t lfs_cd_handler(int32_t argc, char **argv);
static shell_status_t lfs_rm_handler(int32_t argc, char **argv);
static shell_status_t lfs_mkdir_handler(int32_t argc, char **argv);
static shell_status_t lfs_write_handler(int32_t argc, char **argv);
static shell_status_t lfs_cat_handler(int32_t argc, char **argv);

static status_t lfs_test();


uint8_t mcrt_load_application(uint8_t* application_buffer, uint32_t* application_len);
uint8_t mcrt_record_application(uint8_t* application_buffer, uint32_t application_len);
void init_fs();

#endif /* MCRT_FS_ADAPTOR_H_ */
