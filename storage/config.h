#ifndef STORAGE_CONFIG_H
#define STORAGE_CONFIG_H

#include <iostream>
#include <cstdio>
#include <stdlib.h>
#include <time.h>
#include <vector> 
#include <fstream>
#include <string>
#include <string.h>
using namespace std; 

// configuration配置参数

// 磁盘中的数据库文件 配置参数
#define BLOCK_SIZE (4*1024)            // 磁盘中数据块的大小，也等于buffer中的PAGE_SIZE
#define FILE_DATA_SIZE (1024*1024*1024)// 文件数据区大小1G，单位字节
#define FILE_DATA_ADDR (BIT_MAP_ADDR + BIT_MAP_SIZE)         // 数据区起始位置
#define BIT_MAP_SIZE (FILE_DATA_SIZE / BLOCK_SIZE / 8) // 位示图大小
#define BIT_MAP_ADDR 1024               // 位示图起始位置 
#define MAX_NAME_LENGTH 50              //表名,属性名等名字的最大长度
#define MAX_ATTRIBUTE_NUM 20            //表中属性个数上限
//TODO: 需测试确保dbMeta不会超出1024，否则位示图数据将会与之重叠，会出现未知错误

// file 配置参数
#define MAX_FILE_NUM     100  //最大文件数量
#define MAP_FILE         0
#define TABLE_FILE      1    //表文件
#define DATA_DICT_FILE   2    //数据字典文件
#define INDEX_FILE_BTREE 3

// page 配置参数
#define PAGE_SIZE BLOCK_SIZE//页大小==文件块大小
#define SIZE_OF_LONG sizeof(long)  //long类型的长度，使得该程序可以适用于不同位数cpu的机器

// buffer 配置参数
#define BUFFER_NUM 2048     //缓冲区数量,每个4k，总大小8M

// segment
#define SEGMENT_NUM 5       //文件的段数

typedef enum{
	INT_TYPE = 0,
	LONG_TYPE = 1,
	FLOAT_TYPE = 2,
	DOUBLE_TYPE = 3,
	CHAR_TYPE = 4,
	VARCHAR_TYPE = 5,
	DATE_TYPE = 6,
}DATA_TYPE;

const static char dataTypeName[7][10]={"INT","LONG","FLOAT","DOUBLE","CHAR","VARCHAR","DATE"};

#endif