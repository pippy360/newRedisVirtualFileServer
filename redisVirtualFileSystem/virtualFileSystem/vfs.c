#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include "./hiredis/hiredis.h"
#define MAX_FILENAME_SIZE 10000

//NO STRING PARSING SHOULD BE DONE IN THIS FILE

void vfs_connect(redisContext **c) {
	const char *hostname = "127.0.0.1";
	int port = 6379;

	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	*c = redisConnectWithTimeout(hostname, port, timeout);
	if (*c == NULL || (*c)->err) {
		if (*c) {
			printf("Connection error: %s\n", (*c)->errstr);
			redisFree(*c);
		} else {
			printf("Connection error: can't allocate redis context\n");
		}
		exit(1);
	}
}

long getNewId(redisContext *context) {
	redisReply *reply;
	reply = redisCommand(context, "GET current_id_count");

	//fixme:
	if (reply->str == NULL) {
		return -1;
	}

	long result = strtol(reply->str, NULL, 10);
	freeReplyObject(reply);
	reply = redisCommand(context, "INCR current_id_count");
	freeReplyObject(reply);
	return result;
}

void __mkdir(redisContext *context, long parentId, long id, const char *name) {
	redisReply *reply;
	reply = redisCommand(context,
			"HMSET FOLDER_%lu_info  parent %lu name \"%s\" createdData \"%s\"",
			id, parentId, name, "march 7th");
	freeReplyObject(reply);
}

//Error handling is taken care of in the path parsing, so this function contains NO error handling
//returns dir id
long vfs_mkdir(redisContext *context, long parentId, const char *name) {
	long id = getNewId(context);
	redisReply *reply;
	reply = redisCommand(context, "LPUSH FOLDER_%lu_folders %lu", parentId, id);
	freeReplyObject(reply);
	__mkdir(context, parentId, id, name);
	return id;
}

void __createFile(redisContext *context, long id, const char *name, long size,
		const char *googleId, const char *webUrl, const char *apiUrl) {
	redisReply *reply;
	reply =
			redisCommand(context,
					"HMSET FILE_%lu_info name \"%s\" size \"%lu\" createdData \"%s\" id \"%s\" webUrl \"%s\" apiUrl \"%s\" ",
					id, name, size, "march 7th", googleId, webUrl, apiUrl);
	freeReplyObject(reply);
}

void __addFileToFileList(redisContext *context, long folderId, long fileId) {
	redisReply *reply;
	reply = redisCommand(context, "LPUSH FOLDER_%lu_files %lu", folderId,
			fileId);
	freeReplyObject(reply);
}

void __addDirToFolderList(redisContext *context, long folderId, long fileId) {
	redisReply *reply;
	reply = redisCommand(context, "LPUSH FOLDER_%lu_folders %lu", folderId,
			fileId);
	freeReplyObject(reply);
}

void __removeIdFromFileList(redisContext *context, long dirId, long removeId) {
	redisReply *reply;
	reply = redisCommand(context, "LREM FOLDER_%ld_files 0 %ld", dirId,
			removeId);
	freeReplyObject(reply);
}

void __removeIdFromFolderList(redisContext *context, long dirId, long removeId) {
	redisReply *reply;
	reply = redisCommand(context, "LREM FOLDER_%ld_folders 0 %ld", dirId,
			removeId);
	freeReplyObject(reply);
}

long vfs_createFile(redisContext *context, long parentId, const char *name, long size,
		const char *googleId, const char *webUrl, const char *apiUrl) {
	//add it to the file list of the dir
	long id = getNewId(context);
	__addFileToFileList(context, parentId, id);
	__createFile(context, id, name, size, googleId, webUrl, apiUrl);
	return id;
}

void __deleteFile(redisContext *context, long fileId) {

}

void __deleteDir(redisContext *context, long dirId) {

}

//FIXME: return error codes
void vfs_delete(redisContext *context, char *path) {

}

void __mv(redisContext *context, long fileId, long oldParentId,
		long newParentId, char *newFileName) {
	if (newFileName == NULL) {

	}
}

void vfs_setFileName(redisContext *context, long id, char *nameBuffer,
		int nameBufferLength) {
	redisReply *reply;
	//FIXME: using %.*s doesn't work when used with redisCommand....weird...
	//so we have to do this hack below
	char *tempPtr = malloc(nameBufferLength + 1);
	memcpy(tempPtr, nameBuffer, nameBufferLength);
	tempPtr[nameBufferLength] = '\0';

	reply = redisCommand(context, "HSET FILE_%lu_info name \"%s\"", id,
			tempPtr);
	freeReplyObject(reply);
	free(tempPtr);
}

//  ####  ###### ##### ##### ###### #####   ####
// #    # #        #     #   #      #    # #
// #      #####    #     #   #####  #    #  ####
// #  ### #        #     #   #      #####       #
// #    # #        #     #   #      #   #  #    #
//  ####  ######   #     #   ###### #    #  ####

//returns the reply for the name resquest
//so use freeReplayObject() to free
void vfs_getFileName(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FILE_%lu_info name", id);
	sprintf(outputNameBuffer, "%.*s", (int) strlen(reply->str) - 2,
			reply->str + 1);
	freeReplyObject(reply);
}

//FIXME: FIXME: temporary fix here, change APIURL TO WEBURL
void vfs_getFileWebUrl(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FILE_%lu_info apiUrl", id);
	sprintf(outputNameBuffer, "%.*s", (int) strlen(reply->str) - 2,
			reply->str + 1);
	freeReplyObject(reply);
}

long vfs_getFileSizeFromId(redisContext *context, long id) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FILE_%lu_info size", id);
	if (reply->str == NULL) {
		freeReplyObject(reply);
		return -1;
	}
	//printf("we asked for the size of id: %lu and got: %s\n", id, reply->str);
	long size = strtol(reply->str + 1, NULL, 10);//FIXME: the +1 is here because we have the "'s, get rid of that
	return size;
}

//^see get file name
void vfs_getFolderName(redisContext *context, long id, char *outputNameBuffer,
		int outputNameBufferLength) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FOLDER_%lu_info name", id);
	sprintf(outputNameBuffer, "%.*s", (int) strlen(reply->str) - 2,
			reply->str + 1);
	freeReplyObject(reply);
}

//this only works with folders for the moment
long vfs_getDirParent(redisContext *context, long cwdId) {
	redisReply *reply;
	reply = redisCommand(context, "HGET FOLDER_%lu_info parent", cwdId);
	//printf("the command we ran HGET FOLDER_%lu_info parent\n", cwdId);
	long newId = strtol(reply->str, NULL, 10);
	freeReplyObject(reply);
	//printf("the result %ld\n", newId);
	return newId;
}

void vfs_setDirParent(redisContext *context, long dirId, long newParent) {
	redisReply *reply;
	reply = redisCommand(context, "HSET FOLDER_%lu_info parent %ld", dirId,
			newParent);
	freeReplyObject(reply);
}

void vfs_ls(redisContext *context, long dirId) {
	int j = 0;
	long id;
	redisReply *reply;
	char name[MAX_FILENAME_SIZE];
	reply = redisCommand(context, "LRANGE FOLDER_%lu_folders 0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (j = 0; j < reply->elements; j++) {
			id = strtol(reply->element[j]->str, NULL, 10);
			vfs_getFolderName(context, id, name, MAX_FILENAME_SIZE);
			printf("ls: %s\n", name);
		}
	}
	freeReplyObject(reply);
	reply = redisCommand(context, "LRANGE FOLDER_%lu_files   0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (j = 0; j < reply->elements; j++) {
			long id = strtol(reply->element[j]->str, NULL, 10);
			vfs_getFileName(context, id, name, MAX_FILENAME_SIZE);
			printf("ls: %s\n", name);
		}
	}
	freeReplyObject(reply);
}

//return's id if successful, -1 otherwise
long vfs_findFileNameInDir(redisContext *context, long dirId, const char *fileName,
		int fileNameLength) {
	long tempId, matchId = -1;
	int i;
	redisReply *reply;
	char nameBuffer[MAX_FILENAME_SIZE];
	reply = redisCommand(context, "LRANGE FOLDER_%lu_files 0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (i = 0; i < reply->elements; i++) {
			tempId = strtol(reply->element[i]->str, NULL, 10);
			vfs_getFileName(context, tempId, nameBuffer, MAX_FILENAME_SIZE);
			int length = (fileNameLength > strlen(nameBuffer))? fileNameLength: strlen(nameBuffer);
			if (strncmp(fileName, nameBuffer, length) == 0) {
				matchId = tempId;
			}
		}
	}
	freeReplyObject(reply);
	return matchId;
}

//return's id if successful, -1 otherwise
//works for '.' and '..'
long vfs_findDirNameInDir(redisContext *context, long dirId, char *dirName,
		int dirNameLength) {
	long tempId, matchId = -1;
	int i;
	redisReply *reply;
	char nameBuffer[MAX_FILENAME_SIZE];

	if (dirNameLength == 1 && strncmp(".", dirName, 1) == 0) {
		return dirId;
	} else if (dirNameLength == 2 && strncmp("..", dirName, 2) == 0) {
		return vfs_getDirParent(context, dirId);
	}

	reply = redisCommand(context, "LRANGE FOLDER_%lu_folders 0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (i = 0; i < reply->elements; i++) {
			tempId = strtol(reply->element[i]->str, NULL, 10);
			vfs_getFolderName(context, tempId, nameBuffer, MAX_FILENAME_SIZE);
			// printf("comparing against: %s , %s\n", nameBuffer, dirName);
			if (strncmp(dirName, nameBuffer, dirNameLength) == 0) {
				matchId = tempId;
				break;
			}
		}
	}
	freeReplyObject(reply);
	return matchId;
}

//FIXME: SO MUCH REPEATED CODE HERE, CLEAN THIS UP
//-1 if not found, id otherwise

char *vfs_listUnixStyle(redisContext *context, long dirId) {
	//get the folder
	//get all the id's, go over them and print sprintf the contents

	//get the folder
	//go through all the folders
	char *line = malloc(20000);
	line[0] = '\0';

	int j = 0;
	redisReply *reply;
	char name[MAX_FILENAME_SIZE];
	reply = redisCommand(context, "LRANGE FOLDER_%lu_folders 0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (j = 0; j < reply->elements; j++) {
			long innerDirId = strtol(reply->element[j]->str, NULL, 10);
			vfs_getFolderName(context, innerDirId, name, MAX_FILENAME_SIZE);
			sprintf(line + strlen(line),
					"%s   1 %s %s %10lu Jan  1  1980 %s\r\n", "drwxrwxr-x",
					"linux", "linux", (long) 1, name);
		}
	}
	freeReplyObject(reply);

	reply = redisCommand(context, "LRANGE FOLDER_%lu_files   0 -1", dirId);
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (j = 0; j < reply->elements; j++) {
			long fileId = strtol(reply->element[j]->str, NULL, 10);
			vfs_getFileName(context, fileId, name, MAX_FILENAME_SIZE);
			sprintf(line + strlen(line),
					"%s   1 %s %s %10lu Jan  1  1980 %s\r\n", "-rwxrwxr-x",
					"linux", "linux",
					(long) vfs_getFileSizeFromId(context, fileId), name);
		}
	}
	freeReplyObject(reply);

	return line;
}

//FIXME: MOVE THIS TO THE PARSER
//FIXME: clean up the use of buffers here, buffer2 is kind of a hack
void vfs_getDirPathFromId(redisContext *context, long inputId,
		char *outputBuffer, int outputBufferLength) {
	long currentId = inputId;
	redisReply *parentIdReply, *nameReply;
	char buffer[outputBufferLength];
	char buffer2[outputBufferLength];
	outputBuffer[0] = '\0';

	while (currentId != 0) {
		//get the name of the current id
		vfs_getFolderName(context, currentId, buffer2, outputBufferLength);

		//FIXME: check if it'll fit !
		//if(strlen(outputBuffer) > outputBufferLength){
		//}
		sprintf(buffer, "/%s%s", buffer2, outputBuffer);
		strcpy(outputBuffer, buffer);

		//get it's parent's id
		parentIdReply = redisCommand(context, "HGET FOLDER_%lu_info parent",
				currentId);
		currentId = strtol(parentIdReply->str + 1, NULL, 10);
		freeReplyObject(parentIdReply);
	}
	strcat(outputBuffer, "/");
}

//id 0 == root
void vfs_buildDatabase(redisContext *context) {
	//wipe it and set the first id to 0 and crate a root folder
	redisReply *reply;
	reply = redisCommand(context, "FLUSHALL");
	freeReplyObject(reply);
	__mkdir(context, 0, 0, "root");
	reply = redisCommand(context, "SET current_id_count 1");
	reply = redisCommand(context, "SET isVirtualFileSystemCreated yes");
	freeReplyObject(reply);
}

int isVirtualFileSystemCreated(redisContext *context) {
	redisReply *reply;
	reply = redisCommand(context, "EXISTS isVirtualFileSystemCreated");
	long newId = (int) reply->integer;
	freeReplyObject(reply);
	return newId;
}

//FIXME:
void buildDatabaseIfRequired(redisContext *context) {
	if (!isVirtualFileSystemCreated(context)) {
		vfs_buildDatabase(context);
	}
}


