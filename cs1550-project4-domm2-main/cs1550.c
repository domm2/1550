
#define FUSE_USE_VERSION 26

#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>

#include "cs1550.h"

FILE * fp;
char directory[MAX_FILENAME +1]; 
char filename[MAX_FILENAME+1];
char extension[MAX_EXTENSION+1];


/**
 * Called whenever the system wants to know the file attributes, including
 * simply whether the file exists or not.
 *
 * `man 2 stat` will show the fields of a `struct stat` structure.

 * This function should look up the input path to determine if it is a directory or a file. 
 * If it is a directory, return the appropriate permissions. 
 * If it is a file, return the appropriate permissions as well as the actual size. 
 * This size must be accurate since it is used to determine EOF and thus read may not be called.

 * return 0 on success, with a correctly set structure
 * return -ENOENT if the file is not found
 */
static int cs1550_getattr(const char *path, struct stat *statbuf)
{
	// Clear out `statbuf` first -- this function initializes it.
	memset(statbuf, 0, sizeof(struct stat));
	//return 0,1,2,3
	int result = -1;
	result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
	struct cs1550_root_directory root; 
	struct cs1550_directory_entry dir;

	// Check if the path is the root directory.
	// If it is a directory, return the appropriate permissions. 
	if (strcmp(path, "/") == 0) {
		statbuf->st_mode = S_IFDIR | 0755;
		statbuf->st_nlink = 2;

		return 0; // no error
	}
	// Check if the path is a subdirectory.
	// If it is a directory, return the appropriate permissions. 
	else if (result == 1) { 
		// only if the directory is found
		fseek(fp, 0, SEEK_SET);
		fread(&root, sizeof(struct cs1550_root_directory), 1, fp);
		int rootDir = (int) root.num_directories;
		for(int i =0; i< rootDir; i++){
			if(strncmp(root.directories[i].dname, directory, MAX_FILENAME + 1) == 0){
				statbuf->st_mode = S_IFDIR | 0755; 
				statbuf->st_nlink = 2;
				return 0;
			}
		}

		return -ENOENT;
	}

	// Check if the path is a file.
	// If it is a file, return the appropriate permissions 
	// as well as the actual size.
	else if ((result == 2) || (result == 3)){
		fseek(fp, 0, SEEK_SET);
		fread(&root, sizeof(struct cs1550_root_directory), 1, fp);
		int rootDir = (int) root.num_directories;
		for(int i =0; i< rootDir; i++){
			if(strncmp(root.directories[i].dname, directory, MAX_FILENAME + 1) == 0){
				int start = (int) root.directories[i].n_start_block;
				fseek(fp, BLOCK_SIZE * start, SEEK_SET);

				fread(&dir, sizeof(struct cs1550_directory_entry), 1, fp);
				int filNum = (int) dir.num_files; 

				for(int i =0; i< filNum; i++){
					if(strncmp(dir.files[i].fname, filename, MAX_FILENAME + 1) == 0){
						//check if ext matches
						if(result ==3){
							if(strncmp(dir.files[i].fext, extension, MAX_EXTENSION+1) == 0){
								// Regular file
								statbuf->st_mode = S_IFREG | 0666;
								// Only one hard link to this file
								statbuf->st_nlink = 1;
								// File size  
								statbuf->st_size = dir.files[i].fsize; 
								return 0; // no error
							}
						}
						// no extension and filname matches
						else{
							// Regular file
							statbuf->st_mode = S_IFREG | 0666;
							// Only one hard link to this file
							statbuf->st_nlink = 1;
							// File size 
							statbuf->st_size = dir.files[i].fsize; 
							return 0; // no error
						}
					}
				}	
			}//end if
		}
		return -ENOENT;
	}
	// Otherwise, the path doesn't exist.
	else return -ENOENT;
}

/**
 * Called whenever the contents of a directory are desired. Could be from `ls`,
 * or could even be when a user presses TAB to perform autocompletion.

 * This function should list all subdirectories of the root, 
 * or all files of a subdirectory (depending on the path).

 * return 0 on success
 * -ENOENT if the directory is not found
 */
static int cs1550_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			  off_t offset, struct fuse_file_info *fi)
{
	//printf("offsettttttttt7: %li\n", offset);
	(void) offset;
	(void) fi;
	int result = -1;
	result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
	struct cs1550_root_directory root; 
	struct cs1550_directory_entry dir;
	fseek(fp, 0, SEEK_SET);
	fread(&root, sizeof(struct cs1550_root_directory), 1, fp);
	int rootDir = (int) root.num_directories;

	// The filler function allows us to add entries to the listing.
	

	// Check if the path is the root directory.
	if (strcmp(path, "/") == 0) {
		//printf("offsettttttttt1: %li\n", offset);
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		for(int i =0; i< rootDir; i++){
			filler(buf, root.directories[i].dname, NULL, 0);
		}
		return 0;
	}
	// Check if the path is a subdirectory.
	else if (result == 1) { 
		//printf("offsettttttttt2: %li\n", offset);
		int fdir = 0;
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		for(int i =0; i< rootDir; i++){
			//printf("offsettttttttt3: %li\n", offset);
			if(strncmp(root.directories[i].dname, directory, MAX_FILENAME + 1) == 0){
				//printf("offsettttttttt: %li\n", offset);
				fseek(fp, root.directories[i].n_start_block, SEEK_SET);
				fread(&dir, sizeof(struct cs1550_directory_entry), 1, fp);
				fdir =1;
			}
		}
		if(fdir==0) return -ENOENT;
		else{
			char withExt[(MAX_FILENAME + MAX_EXTENSION + 2)];
			for(size_t d =0; d < dir.num_files; d++) {

				strncpy(withExt, dir.files[d].fname, (MAX_FILENAME+1));
				if(strcmp(dir.files[d].fext, "") != 0){
					strncat(withExt, ".", 2);
					strncat(withExt, dir.files[d].fext, (MAX_EXTENSION+1));
				}
				filler(buf, withExt, NULL, 0);
			}
			return 0;
		}
	}	
	return -ENOENT;
}

/**
 * Creates a directory. Ignore `mode` since we're not dealing with permissions.
 * This function should add the new directory to the root level, 
 * and should update the .disk file appropriately.

 * return 0 on success
 * -ENAMETOOLONG if the name is longer than 8 characters
 * -EPERM if the directory is not under the root directory only
 * -EEXIST if the directory already exists
 */
static int cs1550_mkdir(const char *path, mode_t mode)
{
	(void) mode;
	char* token = NULL;
	//using the path variable, check the bytes between the 2 slashes (the directory). 
	//If it is longer than 8 than we return that error. otherwise we continue
	char copy[strlen(path)];
	strncpy(copy, path +1, strlen(path));
	//Scan a string for the last occurrence of a character.
	token = strtok(copy, "/");
	if(strtok(token, ".") != NULL) token = strtok(token, ".");
	// printf("tokennnnnnn: %s\n", token);
	// printf("lengthhh: %ld\n", strlen(token));
	// printf("pathhhhhhhhhh: %s\n", path);
	if (strlen(token) > MAX_FILENAME) return -ENAMETOOLONG;
	
	int result = -1;
	result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
	struct cs1550_root_directory root; 
	// printf("%lu", sizeof(directory) );
	// printf( " : size of ------------");
	// printf("%d", MAX_FILENAME);
	// printf( " : max size ++++++++++");
	// int duh = 

	// Check if the path is a subdirectory.
	// If it is a directory, return the appropriate permissions. 
	if (result == 1){ 
		// only if the directory is found
		fseek(fp, 0, SEEK_SET);
		fread(&root, sizeof(struct cs1550_root_directory), 1, fp);
		long unsigned int rootDir = (long unsigned int) root.num_directories;
		if(rootDir >= MAX_DIRS_IN_ROOT) return -ENOSPC;
		for(long unsigned int i =0; i< rootDir; i++){
			if(strncmp(root.directories[i].dname, directory, MAX_FILENAME + 1) == 0){
				return -EEXIST;
			}
		}
		//Copy the directory name from the path to the directory name at entry at index num directories
		strncpy(root.directories[rootDir].dname, directory, MAX_FILENAME+1); 
		//Use the last allocated block+1 as the block number for the new directory 
		//and increment last allocated block
		root.directories[rootDir].n_start_block = root.last_allocated_block+1;
		root.last_allocated_block += 1; 
		//increment num_directories 
		root.num_directories += 1;
		//seek to the beginning of the file and write the root directory back to the file
		fseek(fp, 0, SEEK_SET);
		fwrite(&root, sizeof(struct cs1550_root_directory), 1, fp);
		return 0;
	}
	else return -EPERM;
}

/**
 * Removes a directory.
 */
static int cs1550_rmdir(const char *path)
{
	(void) path;
	return 0;
}

/**
 * Does the actual creation of a file. `mode` and `dev` can be ignored.
 * add a new file to a subdirectory, and should 
 * update the .disk file appropriately with the modified directory entry structure.

 * return 0 on success
 * -ENAMETOOLONG if the name is beyond 8.3 characters
 * -EPERM if the file is created in the root directory
 * -EEXIST if the file already exist
 */
static int cs1550_mknod(const char *path, mode_t mode, dev_t dev)
{
	(void) mode;
	(void) dev;
	/*  testmount2/dir_0/file_i.txt   
	    testmount4/file0.dat         */
	int dirfound = 0; int filNum = 0; int start = 0;
	//check the length is 8.3 or less 
	char* token = NULL;
	char* token2 = NULL;
	char copy[strlen(path)];
	strncpy(copy, path +1, strlen(path));
	//Scan a string for the last occurrence of a character.
	token = strrchr(copy, (int)'/'); /* /file_i.txt or /file_i   */	
	if(token==NULL) return -EPERM;
	token2 = strrchr(copy, (int)'.'); /* .txt or null  */	
	if(token2 != NULL) { //have ext
		if (strlen(token2) > MAX_EXTENSION+1) return -ENAMETOOLONG; // +1 for .
		token = strtok(token, "."); // /file_i
	}
	if (strlen(token) > MAX_FILENAME+1) return -ENAMETOOLONG; // +1 for /

	// printf("tokennnnnnn: %s\n", token);
	// printf("tokennnnnnn2: %s\n", token2);
	// printf("lengthhh: %ld\n", strlen(token));
	// printf("lengthhh2: %ld\n", strlen(token2));
	// printf("pathhhhhhhhhh: %s\n", path);	

	//if already exist return otherwise create 
	int result = -1;
	result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
	/*  testmount2                            /dir_0     /file_i      .txt   */
	struct cs1550_root_directory root; 
	struct cs1550_directory_entry dir;

	if(result < 2) return -EPERM;
	if (result == 2 || result ==3){ 
		fseek(fp, 0, SEEK_SET);
		fread(&root, sizeof(struct cs1550_root_directory), 1, fp);
		long unsigned int rootDir = (long unsigned int) root.num_directories;
		//check if it exists already
		for(long unsigned int i =0; i< rootDir; i++){
			if(strncmp(root.directories[i].dname, directory, MAX_FILENAME + 1) == 0){
				dirfound = 1;
				start = (int) root.directories[i].n_start_block;
		
				fseek(fp, BLOCK_SIZE * start, SEEK_SET);
				fread(&dir, sizeof(struct cs1550_directory_entry), 1, fp);

				filNum = (int) dir.num_files; 
				for(int i =0; i< filNum; i++){
					if(strncmp(dir.files[i].fname, filename, MAX_FILENAME + 1) == 0){
						//check if ext matches
						if(result ==3){
							if(strncmp(dir.files[i].fext, extension, MAX_EXTENSION+1) == 0){
								return -EEXIST;
							}
						}
						// path no extension and filname matches. check if filename has no ext too
						else{ if(dir.files[i].fext == NULL) return -EEXIST; }
					}
				}//end for	
				if((long unsigned int)filNum >= MAX_FILES_IN_DIR) return -ENOSPC;
			}
		}
		if(dirfound ==0) return -ENOENT;
		//Copy the file name from the path to the file name in dir at index num files
		strncpy(dir.files[filNum].fname, filename, MAX_FILENAME+1); 
		if(result==3) 
			strncpy(dir.files[filNum].fext, extension, MAX_EXTENSION+1); 

		//Use the last allocated block+1 from the root block 
		//as the block number for the index block
		//and increment last allocated block
		dir.files[filNum].n_index_block = root.last_allocated_block+1;
		root.last_allocated_block += 1; 
		//increment numfiles 


		//seek to and read the index block into a var 
		struct cs1550_index_block index;
		fseek(fp, (dir.files[filNum].n_index_block) * BLOCK_SIZE, SEEK_SET);
		fread(&index, sizeof(struct cs1550_index_block), 1, fp);

		//write the root directory back to the file
		// Write the value of the last_allocated_block+1 as the 1st entry in the index block array 
		//increment last_allocated_block for the first data block of the file
		index.entries[0] = root.last_allocated_block+1; //might be without the +1 already incremented
		//root.last_allocated_block +=1;
		dir.num_files += 1;

		fseek(fp, 0, SEEK_SET);
		fwrite(&root, sizeof(struct cs1550_root_directory), 1, fp);

		fseek(fp, BLOCK_SIZE * start, SEEK_SET);
		fwrite(&dir, sizeof(struct cs1550_directory_entry), 1, fp);

		fseek(fp, BLOCK_SIZE * dir.files[filNum].n_index_block, SEEK_SET);
		fwrite(&index, sizeof(struct cs1550_index_block), 1, fp);

		return 0;
	}
	else return -EPERM;
}

/**
 * Deletes a file.
 */
static int cs1550_unlink(const char *path)
{
	(void) path;
	return 0;
}

static size_t MIN(size_t x, size_t y) { 
	if(x > y) return y;
	else return x;
	}
/**
 * Read `size` bytes from file into `buf`, starting from `offset`.

 * This function reads size bytes from the file into buf, starting at offset.

 * return Number of bytes read on success
 * -ENOENT if the file is not found
 * -EISDIR if the path is a directory
 */
static int cs1550_read(const char *path, char *buf, size_t size, off_t offset,
		       struct fuse_file_info *fi)
{
	// size = 0;
	// (void) path;
	// (void) buf;
	// (void) offset;
	(void) fi;
	// return size;

	int inblock = 0; int readin =0;
	int result = -1; int dirfound = 0; int filNum = 0; int start = 0;
	result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
	/*  testmount2                            /dir_0     /file_i      .txt   */
	struct cs1550_root_directory root; 
	struct cs1550_directory_entry dir;
	struct cs1550_index_block index;

	if(result < 2) return -EISDIR;
	fseek(fp, 0, SEEK_SET);
	fread(&root, sizeof(struct cs1550_root_directory), 1, fp);
	long unsigned int rootDir = (long unsigned int) root.num_directories;
	//check if it exists already
	for(long unsigned int i =0; i< rootDir; i++){
		if(strncmp(root.directories[i].dname, directory, MAX_FILENAME + 1) == 0){
			dirfound = 1;
			start = (int) root.directories[i].n_start_block;
			break;
		}
	}
	if(dirfound ==0) return -ENOENT;

	fseek(fp, BLOCK_SIZE * start, SEEK_SET);
	fread(&dir, sizeof(struct cs1550_directory_entry), 1, fp);

	filNum = (int) dir.num_files; 
	int filfound = 0;
	int i =0;
	for(i =0; i< filNum; i++){
		if(strncmp(dir.files[i].fname, filename, MAX_FILENAME + 1) == 0){
			//check if ext matches
			if(result ==3 && strncmp(dir.files[i].fext, extension, MAX_EXTENSION+1) == 0){
				inblock =  dir.files[i].n_index_block;
				filfound = 1;
				break;
			}
			else if(dir.files[i].fext == NULL){
				inblock =  dir.files[i].n_index_block;
				filfound = 1;
				break;
			}
		}
	}
	if(filfound ==0) return -ENOENT;

	//found file
	if(offset >= (long int) dir.files[i].fsize) return 0;
	fseek(fp, inblock * BLOCK_SIZE, SEEK_SET);
	fread(&index, sizeof(struct cs1550_index_block), 1, fp);
	
	int oldlen = strlen(buf);
	int in = offset / BLOCK_SIZE;
	int inn = offset % BLOCK_SIZE;
	// long unsigned int finalesize = MIN((dir.files[i].fsize - offset), size);
	// long unsigned int esize = MIN((dir.files[i].fsize - offset), size);
	int track = 0;
	while(size!=0){
		struct cs1550_data_block block;
		fseek(fp, index.entries[in + track] * BLOCK_SIZE, SEEK_SET);
		fread(&block, sizeof(struct cs1550_data_block), 1, fp);

		long unsigned int esize = 0;
		esize = MIN((dir.files[i].fsize - offset), size);
		memcpy(buf+oldlen, &block.data[inn], esize);	
		// int readin = strlen(buf) - oldlen;
		oldlen = strlen(block.data);
		size -= esize;
		readin += esize; 
		inn =0; offset =0;
		track++;
	}

	// while(esize>0){
	// 	inn = 0;
	// 	oldlen = strlen(buf);
	// 	offset =0;
	// 	fseek(fp, index.entries[in] * BLOCK_SIZE, SEEK_SET);
	// 	fread(&block, sizeof(struct cs1550_data_block), 1, fp);
	// 	memcpy(&block.data[inn], buf, esize);
	// 	readin = strlen(buf) - oldlen;
	// 	esize -= readin;
	// }
	return readin;
}

/**
 * Write `size` bytes from `buf` into file, starting from `offset`.

 * This function writes size bytes from buf into the file, starting at offset.

 * return Number of bytes written on success
 * -ENOENT if the file is not found
 * -EISDIR if the path is a directory
 */
static int cs1550_write(const char *path, const char *buf, size_t size,
			off_t offset, struct fuse_file_info *fi)
{
	// size = 0;
	// (void) path;
	// (void) buf;
	// (void) offset;
	(void) fi;
	// return size;
	//update inodes allocate indexes 
	//when u write to multiple blocls index.entries[0] = root.last_allocated_block+1;

	//write to data blocks
	int inblock = 0; 
	int result = -1; int dirfound = 0; int filNum = 0; int start = 0;
	int wrote =0; int track = 0; int in = 0;
	result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
	/*  testmount2                            /dir_0     /file_i      .txt   */
	struct cs1550_root_directory root; 
	struct cs1550_directory_entry dir;
	struct cs1550_index_block index;

	if(result < 2) return -EISDIR;
	fseek(fp, 0, SEEK_SET);
	fread(&root, sizeof(struct cs1550_root_directory), 1, fp);
	long unsigned int rootDir = (long unsigned int) root.num_directories;
	//check if it exists already
	for(long unsigned int i =0; i< rootDir; i++){
		if(strncmp(root.directories[i].dname, directory, MAX_FILENAME + 1) == 0){
			dirfound = 1;
			start = (int) root.directories[i].n_start_block;
			break;
		}
	}
	if(dirfound ==0) return -ENOENT;

	fseek(fp, BLOCK_SIZE * start, SEEK_SET);
	fread(&dir, sizeof(struct cs1550_directory_entry), 1, fp);

	filNum = (int) dir.num_files; 
	int filfound = 0;
	int i =0;
	for(i =0; i< filNum; i++){
		if(strncmp(dir.files[i].fname, filename, MAX_FILENAME + 1) == 0){
			//check if ext matches
			if(result ==3 && strncmp(dir.files[i].fext, extension, MAX_EXTENSION+1) == 0){
				inblock =  dir.files[i].n_index_block;
				filfound = 1;
				break;
			}
			else if(dir.files[i].fext == NULL){
				inblock =  dir.files[i].n_index_block;
				filfound = 1;
				break;
			}
		}
	}
	if(filfound ==0) return -ENOENT;

	//found file
	//increase file size
	long unsigned int newsize = offset + size; 
	if(newsize > dir.files[i].fsize) {
		dir.files[i].fsize = newsize;
	}

	if(offset >= (long int) dir.files[i].fsize) return 0;
	fseek(fp, inblock * BLOCK_SIZE, SEEK_SET);
	fread(&index, sizeof(struct cs1550_index_block), 1, fp);
	
	
	in = offset / BLOCK_SIZE;
	int inn = offset % BLOCK_SIZE;
	//int oldlen = 0;
	//long unsigned int finalesize = MIN((dir.files[i].fsize - offset), size);
	//long unsigned int esize = MIN((dir.files[i].fsize - offset), size);
	while(size != 0){ 
		if(track > 0) index.entries[in + track] = ++root.last_allocated_block;

		struct cs1550_data_block block;
		fseek(fp, index.entries[in + track] * BLOCK_SIZE, SEEK_SET);
		fread(&block, sizeof(struct cs1550_data_block), 1, fp);
		//oldlen = strlen(block.data);
		long unsigned int esize = 0;
		esize = MIN((dir.files[i].fsize - offset), size);
		memcpy(&block.data[inn], buf, esize);
		
		// int wrotein = strlen(block.data) - oldlen;
		// esize -= wrotein; 
	
		size -= esize;
		wrote += esize;
		track++;
		inn =0; offset =0;
	}
	// wrotein = strlen(block.data) - oldlen;
	// wrote += wrotein;
	// esize -= wrotein;
	//track++;

	// while(esize>0){
	// 	index.entries[in + track] = ++root.last_allocated_block;
	// 	inn = 0;
	// 	oldlen = strlen(buf);
	// 	offset =0;
	// 	fseek(fp, index.entries[in + track] * BLOCK_SIZE, SEEK_SET);
	// 	fread(&block, sizeof(struct cs1550_data_block), 1, fp);
	// 	memcpy(&block.data[inn], buf, esize);
	// 	wrotein = strlen(buf) - oldlen;
	// 	wrote += wrotein;
	// 	esize -= wrotein;
	// 	track++;
	// }
	/*
		if you need to read more start from 'in'+1 and inn = 0
		until buff size == esize 
	*/

	fseek(fp, 0, SEEK_SET);
	fwrite(&root, sizeof(struct cs1550_root_directory), 1, fp);

	fseek(fp, BLOCK_SIZE * start, SEEK_SET);
	fwrite(&dir, sizeof(struct cs1550_directory_entry), 1, fp);

	fseek(fp, BLOCK_SIZE * inblock, SEEK_SET);
	fwrite(&index, sizeof(struct cs1550_index_block), 1, fp);

	int x = 0;
    while(x < track){
		struct cs1550_data_block block;
		fseek(fp, index.entries[in + x] * BLOCK_SIZE, SEEK_SET);
		fwrite(&block, sizeof(struct cs1550_data_block), 1, fp);
        x++;
    }
	return wrote;

}

/**
 * Called when a new file is created (with a 0 size) or when an existing file
 * is made shorter. We're not handling deleting files or truncating existing
 * ones, so all we need to do here is to initialize the appropriate directory
 * entry.
 */
static int cs1550_truncate(const char *path, off_t size)
{
	(void) path;
	(void) size;
	return 0;
}

/**
 * Called when we open a file.
 */
static int cs1550_open(const char *path, struct fuse_file_info *fi)
{
	(void) fi;
	//(void) path;
	// return 0;

	if (strcmp(path, "/") == 0) {
		return 0; 
	}
	//if already exist return 0
	int result = -1;
	result = sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
	/*  testmount2                            /dir_0     /file_i      .txt   */
	struct cs1550_root_directory root; 
	struct cs1550_directory_entry dir;

	if (result == 1){ 
		// only if the directory is found
		fseek(fp, 0, SEEK_SET);
		fread(&root, sizeof(struct cs1550_root_directory), 1, fp);
		long unsigned int rootDir = (long unsigned int) root.num_directories;
		for(long unsigned int i =0; i< rootDir; i++){
			if(strncmp(root.directories[i].dname, directory, MAX_FILENAME + 1) == 0){
				return 0;
			}
		}
	}//end if result 1
	if (result == 2 || result ==3){ 
		fseek(fp, 0, SEEK_SET);
		fread(&root, sizeof(struct cs1550_root_directory), 1, fp);
		long unsigned int rootDir = (long unsigned int) root.num_directories;
		//check if it exists already
		for(long unsigned int i =0; i< rootDir; i++){
			if(strncmp(root.directories[i].dname, directory, MAX_FILENAME + 1) == 0){
				int start = (int) root.directories[i].n_start_block;
				fseek(fp, BLOCK_SIZE * start, SEEK_SET);
				fread(&dir, sizeof(struct cs1550_directory_entry), 1, fp);

				int filNum = (int) dir.num_files; 
				for(int i =0; i< filNum; i++){
					if(strncmp(dir.files[i].fname, filename, MAX_FILENAME + 1) == 0){
						//check if ext matches
						if(result ==3){
							if(strncmp(dir.files[i].fext, extension, MAX_EXTENSION+1) == 0){
								return 0;
							}
						}
						// path no extension and filname matches. check if filename has no ext too
						else{ if(dir.files[i].fext == NULL) return 0; }
					}
				}//end for	
			}
		}
	}//end if result 2/3
	// If we can't find the desired file, return an error
	return -ENOENT;
}

/**
 * Called when close is called on a file descriptor, but because it might
 * have been dup'ed, this isn't a guarantee we won't ever need the file
 * again. For us, return success simply to avoid the unimplemented error
 * in the debug log.
 */
static int cs1550_flush(const char *path, struct fuse_file_info *fi)
{
	(void) path;
	(void) fi;
	
	// Success!
	return 0;
}

/**
 * This function should be used to open and/or initialize your `.disk` file.
 */
static void *cs1550_init(struct fuse_conn_info *fi)
{
	// Add your initialization routine here.
	fp = fopen(".disk", "rb+");
	(void) fi;
	return NULL;
}

/**
 * This function should be used to close the `.disk` file.
 */
static void cs1550_destroy(void *args)
{
	(void) args;
	// Add your teardown routine here.
	fclose(fp);
}

/*
 * Register our new functions as the implementations of the syscalls.
 */
static struct fuse_operations cs1550_oper = {
	.getattr	= cs1550_getattr,
	.readdir	= cs1550_readdir,
	.mkdir		= cs1550_mkdir,
	.rmdir		= cs1550_rmdir,
	.read		= cs1550_read,
	.write		= cs1550_write,
	.mknod		= cs1550_mknod,
	.unlink		= cs1550_unlink,
	.truncate	= cs1550_truncate,
	.flush		= cs1550_flush,
	.open		= cs1550_open,
	.init		= cs1550_init,
	.destroy	= cs1550_destroy,
};

/*
 * Don't change this.
 */
int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &cs1550_oper, NULL);
}