#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "ext2_shell.h"
typedef struct {
	char * address;
}DISK_MEMORY;
void printFromP2P(char * start, char * end);

#define FSOPRS_TO_FATFS( a )		( EXT2_FILESYSTEM* )a->pdata

int fs_dumpDataSector(DISK_OPERATIONS* disk, int usedSector)
{
	char * start;
	char * end;

	start = ((DISK_MEMORY *)disk->pdata)->address + usedSector * disk->bytesPerSector;
	end = start + disk->bytesPerSector;
	printFromP2P(start, end);
	printf("\n\n");
	PRINTF("dumpDataSector End\n");
	return EXT2_SUCCESS;
}

void printFromP2P(char * start, char * end)
{
	PRINTF("[Enter]: printFromP2P\n");
	int start_int, end_int;
	start_int = (int)start;
	end_int = (int)end;

	printf("start address : %#x , end address : %#x\n\n", start, end - 1);
	start = (char *)(start_int &= ~(0xf));
	end = (char *)(end_int |= 0xf);
	
	while (start <= end)
	{
		PRINTF("TEST1\n");
		if ((start_int & 0xf) == 0)
			fprintf(stdout, "\n%#08x   ", start);
		PRINTF("TEST2\n");
		fprintf(stdout, "%02X  ", *(unsigned char *)start);
		PRINTF("TEST3\n");
		start++;
		start_int++;
	}
	printf("\n\n");

}
void fs_dumpDataPart(DISK_OPERATIONS * disk, SHELL_FS_OPERATIONS * fsOprs, const SHELL_ENTRY * parent, SHELL_ENTRY * entry, const char * name)
{
	EXT2_NODE EXT2Parent;
	EXT2_NODE EXT2Entry;
	INODE node;
	int result;
	char * start, *end;

	shell_entry_to_ext2_entry(parent, &EXT2Parent);
	if (result = ext2_lookup(&EXT2Parent, name, &EXT2Entry)) return result;

	get_inode(EXT2Entry.fs, EXT2Entry.entry.inode, &node);

	start = ((DISK_MEMORY *)disk->pdata)->address + (1 + node.block[0]) * disk->bytesPerSector;
	end = start + disk->bytesPerSector;
	printFromP2P(start, end);
}
void fs_dumpfileinode(DISK_OPERATIONS * disk, SHELL_FS_OPERATIONS * fsOprs, const SHELL_ENTRY * parent, SHELL_ENTRY * entry, const char * name)
{
	EXT2_NODE EXT2Parent;
	EXT2_NODE EXT2Entry;
	INODE node;
	int result, inode, i;


	shell_entry_to_ext2_entry(parent, &EXT2Parent);
	if (result = ext2_lookup(&EXT2Parent, name, &EXT2Entry)) return result;

	get_inode(EXT2Entry.fs, EXT2Entry.entry.inode, &node);
	inode = EXT2Entry.entry.inode;

	printf("inode number : %d\n", inode);
	for (i = 0; i < EXT2_N_BLOCKS; i++) {
		printf("iblock[%.2d] : %u\n", i, node.block[i]);
	}

}
void fs_dumpDataBlockByNum(DISK_OPERATIONS * disk, SHELL_FS_OPERATIONS * fsOprs, const SHELL_ENTRY * parent, SHELL_ENTRY * entry, int num)
{
	char * start, *end;
	start = ((DISK_MEMORY *)disk->pdata)->address + (1 + num) * disk->bytesPerSector;
	end = start + disk->bytesPerSector;
	printFromP2P(start, end);
}
void printf_by_sel(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, const char* name, int sel, int num)
{
	switch (sel) {
	case 1:
		fs_dumpDataSector(disk, 1);
		break;
	case 2:
		fs_dumpDataSector(disk, 2);
		break;
	case 3:
		fs_dumpDataSector(disk, 3);
		break;
	case 4:
		fs_dumpDataSector(disk, 4);
		break;
	case 5:
		fs_dumpDataSector(disk, 5);
		fs_dumpDataSector(disk, 6);
		break;
	case 6:
		fs_dumpDataPart(disk, fsOprs, parent, entry, name);
		break;
	case 7:
		fs_dumpfileinode(disk, fsOprs, parent, entry, name);
		break;
	case 8:
		fs_dumpDataBlockByNum(disk, fsOprs, parent, entry, num);
		break;
	}
}
int fs_format(DISK_OPERATIONS* disk, void* param)
{
	printf("formatting as a %s\n", (char *)param);
	ext2_format(disk);

	return  1;
}

static SHELL_FILE_OPERATIONS g_file =
{
	fs_create,
	fs_remove,
	fs_read,
	fs_write
};

static SHELL_FS_OPERATIONS   g_fsOprs =
{
	fs_read_dir,
	fs_stat, 
	fs_mkdir,
	fs_rmdir,
	fs_lookup,
	&g_file,
	NULL // 
};

int fs_mount(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY* root)
{
	EXT2_FILESYSTEM* fs;
	
	EXT2_NODE ext2_entry;
	int result;

	*fsOprs = g_fsOprs;

	fsOprs->pdata = malloc(sizeof(EXT2_FILESYSTEM));
	fs = FSOPRS_TO_EXT2FS(fsOprs);
	ZeroMemory(fs, sizeof(EXT2_FILESYSTEM));
	fs->disk = disk;

	result = ext2_read_superblock(fs, &ext2_entry);

	if (result == EXT2_SUCCESS)
	{
		printf("number of groups         : %d\n", NUMBER_OF_GROUPS);
		printf("blocks per group         : %d\n", fs->sb.block_per_group);
		printf("bytes per block          : %d\n", disk->bytesPerSector);
		printf("free block count	 : %d\n", fs->sb.free_block_count);
		printf("free inode count	 : %d\n", fs->sb.free_inode_count);
		printf("first non reserved inode : %d\n", fs->sb.first_non_reserved_inode);
		printf("inode structure size	 : %d\n", fs->sb.inode_structure_size);
		printf("first data block number  : %d\n", fs->sb.first_data_block_each_group);
		printf("\n----------------------------------------------\n");
	}

	printf("VOLUME_LABEL : %s\n", ext2_entry.entry.name);
	ext2_entry_to_shell_entry(fs, &ext2_entry, root); //&ext2_entry를 root에 먹임

	return result;
}
void fs_umount(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs)
{
	return;
}

static SHELL_FILESYSTEM g_fat =
{
	"EXT2",
	fs_mount,
	fs_umount,
	fs_format

};

int adder(EXT2_FILESYSTEM* fs, void* list, EXT2_NODE* entry)
{
	SHELL_ENTRY_LIST*   entryList = (SHELL_ENTRY_LIST*)list;
	SHELL_ENTRY         newEntry;

	ext2_entry_to_shell_entry(fs, entry, &newEntry);

	add_entry_list(entryList, &newEntry);

	return EXT2_SUCCESS;
}

// while (g_fsOprs.fileOprs->read(&g_disk, &g_fsOprs, &g_currentDir, &entry, offset, 1024, buf) > 0)
int fs_read(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, unsigned long offset, unsigned long length, const char* buffer)
{
	EXT2_NODE EXT2Entry;

	shell_entry_to_ext2_entry(entry, &EXT2Entry);

	return ext2_read(&EXT2Entry, offset, length, buffer);
}

int fs_write(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, unsigned long offset, unsigned long length, const char* buffer)
{
	EXT2_NODE EXT2Entry;
	EXT2_NODE	EXT2Parent;
	shell_entry_to_ext2_entry(entry, &EXT2Entry);
	shell_entry_to_ext2_entry(parent, &EXT2Parent);

	return ext2_write(&EXT2Entry, &EXT2Parent, offset, length, buffer);
}

void shell_register_filesystem(SHELL_FILESYSTEM* fs)
{
	*fs = g_fat;
}

int	fs_create(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name, SHELL_ENTRY* retEntry)
{
	EXT2_NODE	EXT2Parent;
	EXT2_NODE	EXT2Entry;
	int				result;

	shell_entry_to_ext2_entry(parent, &EXT2Parent);

	result = ext2_create(&EXT2Parent, name, &EXT2Entry);
	PRINTF("----Created Entry----\n");
	PRINTF("Entry inode: %u \n", EXT2Entry.entry.inode);
	PRINTF("Entry name : %s \n", EXT2Entry.entry.name);
	PRINTF("---------------------\n");
	ext2_entry_to_shell_entry(EXT2Parent.fs, &EXT2Entry, retEntry);

	return result;
}

int shell_entry_to_ext2_entry(const SHELL_ENTRY* shell_entry, EXT2_NODE* ext2_entry)
{
	EXT2_NODE* entry = (EXT2_NODE*)shell_entry->pdata;

	//*ext2_entry = *entry;
	memcpy(ext2_entry, entry, sizeof(EXT2_NODE));

	return EXT2_SUCCESS;
}

int ext2_entry_to_shell_entry(EXT2_FILESYSTEM* fs, const EXT2_NODE* ext2_entry, SHELL_ENTRY* shell_entry)
{
	EXT2_NODE* entry = (EXT2_NODE*)shell_entry->pdata;
	INODE inodeBuffer;
	BYTE* str = "/";

	ZeroMemory(shell_entry, sizeof(SHELL_ENTRY));

	int inode = ext2_entry->entry.inode;
	int result = get_inode(fs, inode, &inodeBuffer);

	if (ext2_entry->entry.name[0] != '.' && inode == 2);
	else {
		str = shell_entry->name;
		str = my_strncpy(str, ext2_entry->entry.name, 8);
		if (ext2_entry->entry.name[8] != 0x20)
		{
			str = my_strncpy(str, ".", 1);
			str = my_strncpy(str, &ext2_entry->entry.name[8], 3);
		}
	}
	if (FILE_TYPE_DIR & inodeBuffer.mode)
		shell_entry->isDirectory = 1;
	else
		shell_entry->isDirectory = 0;

	shell_entry->permition = 0x01FF & inodeBuffer.mode;

	shell_entry->size = inodeBuffer.size;

	//*entry = *ext2_entry;
	memcpy(entry, ext2_entry, sizeof(EXT2_NODE));

	return EXT2_SUCCESS;
}

int fs_lookup(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, const char* name)
{
	EXT2_NODE	EXT2Parent;
	EXT2_NODE	EXT2Entry;
	int				result;

	shell_entry_to_ext2_entry(parent, &EXT2Parent);

	if (result = ext2_lookup(&EXT2Parent, name, &EXT2Entry))return result;

	ext2_entry_to_shell_entry(EXT2Parent.fs, &EXT2Entry, entry);

	return result;
}

int fs_read_dir(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY_LIST* list)
{
	EXT2_NODE   entry;

	if (list->count)
		release_entry_list(list);

	shell_entry_to_ext2_entry(parent, &entry);
	ext2_read_dir(&entry, adder, list);

	return EXT2_SUCCESS;
}

int fs_stat( DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, unsigned int* totalSectors, unsigned int* usedSectors )
{
	EXT2_NODE	entry;
	
	return ext2_df( FSOPRS_TO_FATFS( fsOprs ), totalSectors, usedSectors );
}


int my_strnicmp(const char* str1, const char* str2, int length)
{
	char   c1, c2;

	while (((*str1 && *str1 != 0x20) || (*str2 && *str2 != 0x20)) && length-- > 0)
	{
		c1 = toupper(*str1);
		c2 = toupper(*str2);

		if (c1 > c2)
			return -1;
		else if (c1 < c2)
			return 1;

		str1++;
		str2++;
	}

	return 0;
}

int is_exist(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name)
{
	SHELL_ENTRY_LIST      list;
	SHELL_ENTRY_LIST_ITEM*   current;

	init_entry_list(&list);

	fs_read_dir(disk, fsOprs, parent, &list);
	current = list.first;

	while (current)
	{
		if (my_strnicmp((char*)current->entry.name, name, 12) == 0)
		{
			release_entry_list(&list);
			return EXT2_ERROR;
		}

		current = current->next;
	}

	release_entry_list(&list);
	return EXT2_SUCCESS;
}

int fs_mkdir(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name, SHELL_ENTRY* retEntry)
{
	EXT2_FILESYSTEM* ext2;
	EXT2_NODE      EXT2_Parent;
	EXT2_NODE      EXT2_Entry;
	int               result;

	ext2 = (EXT2_FILESYSTEM*)fsOprs->pdata;

	if (is_exist(disk, fsOprs, parent, name))
		return EXT2_ERROR;

	shell_entry_to_ext2_entry(parent, &EXT2_Parent);

	result = ext2_mkdir(&EXT2_Parent, name, &EXT2_Entry);

	ext2_entry_to_shell_entry(ext2, &EXT2_Entry, retEntry);

	return result;
}

int fs_remove( DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name )
{
	EXT2_NODE	EXT2Parent;
	EXT2_NODE	file;

	shell_entry_to_ext2_entry( parent, &EXT2Parent );
	ext2_lookup( &EXT2Parent, name, &file );

	return ext2_remove( &file,&EXT2Parent );
}
int fs_rmdir( DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name )
{
	EXT2_NODE	EXT2Parent;
	EXT2_NODE	dir;

	shell_entry_to_ext2_entry( parent, &EXT2Parent );
	ext2_lookup( &EXT2Parent, name, &dir );

	return ext2_rmdir( &dir,&EXT2Parent );
}