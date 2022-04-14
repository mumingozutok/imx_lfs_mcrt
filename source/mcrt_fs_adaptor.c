/*
 * mcrt_fs_adaptor.c
 *
 *  Created on: Apr 13, 2022
 *      Author: mg
 */

//little fs includes
#include "lfs.h"
#include "lfs_mflash.h"

#include "fsl_debug_console.h"
#include "mcrt_os_adaptor.h"
#include "mcrt_fs_adaptor.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

static lfs_t lfs;
static struct lfs_config cfg;
static int lfs_mounted;

static char* application_filename = "appfile.app";
static char* last_application_filename = "last_appfile.app";
static char* default_application_filename = "default_appfile.app";

static shell_status_t lfs_mount_handler(int32_t argc, char **argv)
{
    int res;

    if (lfs_mounted)
    {
    	PRINTF("LFS already mounted\r\n");
        return kStatus_SHELL_Success;
    }

    res = lfs_mount(&lfs, &cfg);
    if (res)
    {
        PRINTF("\rError mounting LFS\r\n");
    }
    else
    {
        lfs_mounted = 1;
    }

    return kStatus_SHELL_Success;
}

static shell_status_t lfs_unmount_handler(int32_t argc, char **argv)
{
    int res;

    if (!lfs_mounted)
    {
    	PRINTF("LFS not mounted\r\n");
        return kStatus_SHELL_Success;
    }

    res = lfs_unmount(&lfs);
    if (res)
    {
        PRINTF("\rError unmounting LFS: %i\r\n", res);
    }

    lfs_mounted = 0;
    return kStatus_SHELL_Success;
}

static shell_status_t lfs_ls_handler(int32_t argc, char **argv)
{
    int res;
    char *path;
    lfs_dir_t dir;
    struct lfs_info info;
    int files;
    int dirs;

    if (!lfs_mounted)
    {
    	PRINTF("LFS not mounted\r\n");
        return kStatus_SHELL_Success;
    }

    if (argc > 2)
    {
    	PRINTF("Invalid number of parameters\r\n");
        return kStatus_SHELL_Success;
    }

    if (argc < 2)
    {
        path = "/";
    }
    else
    {
        path = argv[1];
    }

    /* open the directory */
    res = lfs_dir_open(&lfs, &dir, path);
    if (res)
    {
        PRINTF("\rError opening directory: %i\r\n", res);
        return kStatus_SHELL_Success;
    }

    PRINTF(" Directory of %s\r\n", path);
    files = 0;
    dirs  = 0;

    /* iterate until end of directory */
    while ((res = lfs_dir_read(&lfs, &dir, &info)) != 0)
    {
        if (res < 0)
        {
            /* break the loop in case of an error */
            PRINTF("\rError reading directory: %i\r\n", res);
            break;
        }

        if (info.type == LFS_TYPE_REG)
        {
        	PRINTF("%8d %s\r\n", info.size, info.name);
            files++;
        }
        else if (info.type == LFS_TYPE_DIR)
        {
            PRINTF("%     DIR %s\r\n", info.name);
            dirs++;
        }
        else
        {
            PRINTF("%???\r\n");
        }
    }

    res = lfs_dir_close(&lfs, &dir);
    if (res)
    {
        PRINTF("\rError closing directory: %i\r\n", res);
        return kStatus_SHELL_Success;
    }

    PRINTF(" %d File(s), %d Dir(s)\r\n", files, dirs);

    return kStatus_SHELL_Success;
}

static shell_status_t lfs_cd_handler(int32_t argc, char **argv)
{
	PRINTF(
        "There is no concept of current directory in this example.\r\nPlease always specify the full path.\r\n");
    return kStatus_SHELL_Success;
}

static shell_status_t lfs_rm_handler(int32_t argc, char **argv)
{
    int res;

    if (!lfs_mounted)
    {
    	PRINTF("LFS not mounted\r\n");
        return kStatus_SHELL_Success;
    }

    res = lfs_remove(&lfs, argv[1]);

    if (res)
    {
        PRINTF("\rError while removing: %i\r\n", res);
    }

    return kStatus_SHELL_Success;
}

static shell_status_t lfs_mkdir_handler(int32_t argc, char **argv)
{
    int res;

    if (!lfs_mounted)
    {
    	PRINTF("LFS not mounted\r\n");
        return kStatus_SHELL_Success;
    }

    res = lfs_mkdir(&lfs, argv[1]);

    if (res)
    {
        PRINTF("\rError creating directory: %i\r\n", res);
    }

    return kStatus_SHELL_Success;
}

static shell_status_t lfs_write_handler(int32_t argc, char **argv)
{
    int res;
    lfs_file_t file;

    if (!lfs_mounted)
    {
    	PRINTF("LFS not mounted\r\n");
        return kStatus_SHELL_Success;
    }

    res = lfs_file_open(&lfs, &file, argv[1], LFS_O_WRONLY | LFS_O_APPEND | LFS_O_CREAT);
    if (res)
    {
        PRINTF("\rError opening file: %i\r\n", res);
        return kStatus_SHELL_Success;
    }

    res = lfs_file_write(&lfs, &file, argv[2], argv[3]);
    /*if (res > 0)
        res = lfs_file_write(&lfs, &file, "\r\n", 2);*/

    if (res < 0)
    {
        PRINTF("\rError writing file: %i\r\n", res);
    }

    res = lfs_file_close(&lfs, &file);
    if (res)
    {
        PRINTF("\rError closing file: %i\r\n", res);
    }

    return kStatus_SHELL_Success;
}

static shell_status_t lfs_cat_handler(int32_t argc, char **argv)
{
    int res, i, read_cntr = 0, ret = 1;
    uint8_t* application_buffer = argv[2];
    uint32_t* application_len = argv[3];

    lfs_file_t file;
    uint8_t buf[16];

    if (!lfs_mounted)
    {
    	PRINTF("LFS not mounted\r\n");
        return 0;
    }

    res = lfs_file_open(&lfs, &file, argv[1], LFS_O_RDONLY);
    if (res)
    {
        PRINTF("\rError opening file: %i\r\n", res);
        return 0;
    }

    do
    {
        res = lfs_file_read(&lfs, &file, buf, sizeof(buf));
        if (res < 0)
        {
            PRINTF("\rError reading file: %i\r\n", res);
            ret = 0;
            break;
        }
        //SHELL_Write(s_shellHandle, (char *)buf, res);
        for(i=0;i<res;i++){
        	application_buffer[*application_len] = buf[i];
        	*application_len = *application_len + 1;
        }
    } while (res);

    res = lfs_file_close(&lfs, &file);
    if (res)
    {
    	ret = 0;
        PRINTF("\rError closing file: %i\r\n", res);
    }

    return ret;
}

status_t lfs_test(){
	char* path[] = {"ls", "/"};
	char* mount[] = {"mount"};

	lfs_mount_handler(1,mount);

	lfs_ls_handler(2,path);

	lfs_unmount_handler(0,0);
}

//Application Record and Load Functions
uint8_t mcrt_load_application(uint8_t* application_buffer, uint32_t* application_len)
{
	uint8_t ret = 0;
	char* mount[] = {"mount"};
	char* read_file_cmd[] = {"read", 0, application_buffer, application_len};
	char* ls[] = {"ls", "/"};

	read_file_cmd[1] = application_filename;

	*application_len = 0;

	lfs_mount_handler(1,mount);
	//lfs_ls_handler(2,ls);
	ret = lfs_cat_handler(4, read_file_cmd);
	lfs_unmount_handler(0,0);

	return ret;
}

uint8_t mcrt_record_application(uint8_t* application_buffer, uint32_t application_len)
{
	char* remove_file_cmd[] = {"rm", 0};
	char* write_file_cmd[]={"write", 0,0,0};
	char* mount[] = {"mount"};
	char* ls[] = {"ls", "/"};

	remove_file_cmd[1] = application_filename;
	write_file_cmd[1] = application_filename;

	write_file_cmd[2] = application_buffer;
	write_file_cmd[3] = application_len;


	enter_critical_section_adaptor();

	lfs_mount_handler(1,mount);
	lfs_rm_handler(2, remove_file_cmd);
	//PRINTF("\rRemoved deployment file\r\n");
	//lfs_ls_handler(2,ls);
	lfs_write_handler(4, write_file_cmd);
	//PRINTF("\rCreated deployment file\r\n");
	//lfs_ls_handler(2,ls);
	lfs_unmount_handler(0,0);

	exit_critical_section_adaptor();

	return 0;
}

void init_fs()
{
	status_t status;

    lfs_get_default_config(&cfg);

    status = lfs_storage_init(&cfg);
    if (status != kStatus_Success)
    {
        //PRINTF("LFS storage init failed: %i\r\n", status);
        return status;
    }

    lfs_test();
}
