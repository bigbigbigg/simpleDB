#include "file.h"
#include "storage.h"
#include "buffer.h"
#include "log.h"

//-1表示文件创建失败
int file_newFile(struct Storage *DB,int type, long NeededPageNum){
	if(DB->dbMeta.currFileNum>=MAX_FILE_NUM||DB->dbMeta.blockFree<NeededPageNum){
		printf("空闲空间不足，文件创建失败！/n");
		exit(0);	
	}
	int id = DB->dbMeta.currFileNum;
	DB->dbMeta.currFileNum++;
	long NewPages = page_requestPage( DB,NeededPageNum);
	if(NewPages>=0){
		int i,j;
		for(i=0,j =NewPages;i<NeededPageNum,j<(DB->dbMeta.blockNum);i++,j++){
			struct PageMeta pagemeta;
			pagemeta.recordNum = 0;
			pagemeta.pageNo = j;
			if(i==0){
				pagemeta.prePageNo=-1;//-1表示没有前页
				pagemeta.nextPageNo=j+1;
			}
			else{
				pagemeta.prePageNo=j-1;
				if(j==NewPages+NeededPageNum-1){
					pagemeta.nextPageNo=-1;//-1表示没有后页
					
				}
				else{
					pagemeta.nextPageNo = j+1;
				}
			}
			pagemeta.freeSpace = PAGE_SIZE - sizeof(pagemeta);
			rewind(DB->dbFile);
			fseek(DB->dbFile,DB->dbMeta.dataAddr+pagemeta.pageNo*PAGE_SIZE,SEEK_SET);
			fwrite(&pagemeta,sizeof(pagemeta),1,DB->dbFile);
		}
		for( i = 0;i<MAX_FILE_NUM;i++){
			if(DB->dbMeta.fileMeta[0].segList[i].id<0){
				break;
			}
		}
		DB->dbMeta.fileMeta[0].segList[i].id=id;
		DB->dbMeta.fileMeta[0].segList[i].type=type;
		DB->dbMeta.fileMeta[0].segList[i].firstPageNo=NewPages;
		DB->dbMeta.fileMeta[0].segList[i].pageNum=NeededPageNum;
		DB->dbMeta.blockFree=DB->dbMeta.blockFree-NeededPageNum;
		file_print_freepace(DB);
		
	}
	else{
		printf("未有足够的连续存储空间，文件创建失败！/n");
		return -1;//-1表示创建失败
	}
	return id;
	
}

void file_writeFile(struct Storage *DB, int FileID, int length,char *str){
	int querypage=-1;
	int i;
	for( i=0;i<MAX_FILE_NUM;i++){                                               //这一块是查找文件是否存在
		if(DB->dbMeta.fileMeta[0].segList[i].id==FileID){						//
			querypage=DB->dbMeta.fileMeta[0].segList[i].firstPageNo;			//
			break;																//
		}																		//
	}
	if(querypage==-1){
		printf("该文件id对应的文件不存在！");
		exit(0);
	}

	long CurpageNo = DB->dbMeta.fileMeta[0].segList[i].firstPageNo;				
	long pagenum = DB->dbMeta.fileMeta[0].segList[i].pageNum;
	int fileno = i;
	int sizeofpagehead = sizeof(struct PageMeta);
	int sizeofrecord = sizeof(struct OffsetInPage);									//读取该文件的信息
	rewind(DB->dbFile);					
	bool isfound = false;
	struct PageMeta pagehead;
	struct BufTag buftag = Buf_GenerateTag(CurpageNo);
	memcpy(&pagehead,Buf_ReadBuffer(buftag),sizeofpagehead);						//读取第一页的内容并存放在pagehead里
	OffsetInPage preoffset,curoffset;							//页里的记录索引的结构体，定义在paga.h里
	long currecordpos,curoffsetpos;								//前一个是指当前记录索引的位置，第二个是指当前记录的位置
	for(int i=0;i<pagenum;i++){									//该循环是为了遍历所有的页找出能存放该记录的页
		if(pagehead.freeSpace<=length+sizeofrecord){
			if(pagehead.nextPageNo==-1){
				break;
			}
			CurpageNo = pagehead.nextPageNo;
			buftag = Buf_GenerateTag(CurpageNo);
			memcpy(&pagehead,Buf_ReadBuffer(buftag),sizeofpagehead);
			continue;	
		}
		else{
			memcpy(&preoffset,Buf_ReadBuffer(buftag)+sizeofpagehead,sizeofrecord);
			isfound = true;
			if(pagehead.recordNum==0){
				curoffset.recordID = 0;
				curoffset.offset = length;
				curoffset.isDeleted = false;
				currecordpos = sizeofpagehead;
				curoffsetpos =  PAGE_SIZE - length;
				
				
			}
			else{
				memcpy(&preoffset,Buf_ReadBuffer(buftag)+sizeofpagehead+(pagehead.recordNum-1)*sizeofrecord,sizeofrecord);
				curoffset.recordID = pagehead.recordNum;
				curoffset.offset = preoffset.offset+length;
				curoffset.isDeleted = false;
				currecordpos = sizeofpagehead + sizeofrecord*pagehead.recordNum;
				curoffsetpos = PAGE_SIZE - preoffset.offset-length;
			}
			
		}
		pagehead.recordNum++;
		pagehead.freeSpace=pagehead.freeSpace-length-sizeofrecord;
		memcpy(Buf_ReadBuffer(buftag),&pagehead,sizeofpagehead);
		memcpy(Buf_ReadBuffer(buftag)+currecordpos,&curoffset,sizeofrecord);
		memcpy(Buf_ReadBuffer(buftag)+curoffsetpos,str,length);
		break;						//找到后就break
	}
	if(!isfound){					//若遍历完没有页就新申请一个页。
		long pagenumber = page_requestPage(DB,1);
		if(pagenumber>=0){
			DB->dbMeta.blockFree=DB->dbMeta.blockFree-1;
			file_print_freepace(DB);
			struct PageMeta pagemeta; //pagehead就是未申请前最后一个页
			pagemeta.nextPageNo=-1;
			pagemeta.prePageNo=pagehead.pageNo;				
			pagemeta.pageNo=pagenumber;
			pagehead.nextPageNo = pagenumber;		//将这页加在这个文件中				
			pagemeta.recordNum = 1;
			pagemeta.freeSpace = PAGE_SIZE - length - sizeofpagehead - sizeofrecord;
			curoffsetpos = PAGE_SIZE-length;
			currecordpos = sizeofpagehead;
			curoffset.recordID = 0;
			curoffset.offset = length;
			curoffset.isDeleted = false;
			buftag = Buf_GenerateTag(pagenum);
			memcpy(Buf_ReadBuffer(buftag),&pagemeta,sizeofpagehead);
			memcpy(Buf_ReadBuffer(buftag)+currecordpos,&curoffset,sizeofrecord);
			memcpy(Buf_ReadBuffer(buftag)+curoffsetpos,str,length);
			memcpy(Buf_ReadBuffer(buftag),&pagehead,sizeofpagehead);

			DB->dbMeta.fileMeta[0].segList[fileno].pageNum++;
		}
	}
	
}
void file_readFile(struct Storage *DB,int FileID,char *str){
	int i;
	for(i=0;i<MAX_FILE_NUM;i++){
		if(DB->dbMeta.fileMeta[0].segList[i].id==FileID){
			break;
		}
	}
	int sizeofpagehead = sizeof(struct PageMeta);
	int sizeofrecord = sizeof(struct OffsetInPage);
	long pagenum = DB->dbMeta.fileMeta[0].segList[i].pageNum;
	long CurpageNo = DB->dbMeta.fileMeta[0].segList[i].firstPageNo;
	OffsetInPage preoffset,curoffset;
	struct PageMeta pagehead;
	for(i=0;i<pagenum;i++){					
		struct BufTag buftag = Buf_GenerateTag(CurpageNo);		//根据页号从缓冲区调取页的内容
		memcpy(&pagehead,Buf_ReadBuffer(buftag),sizeofpagehead);//打印页的基本信息
		printf("第%d号文件中的第%d个页面\n",FileID,i+1);
		printf("页号：%ld\n",pagehead.pageNo);
		printf("前继页号：%ld\n",pagehead.prePageNo);
		printf("后继页号：%ld\n",pagehead.nextPageNo);
		printf("记录个数；%d\n",pagehead.recordNum);
		printf("空闲空间：%ld\n",pagehead.freeSpace);
		if(pagehead.recordNum>0){
			for(int j=0;j<pagehead.recordNum;j++){
				int readlength;
				if(j==0){																	//打印页的每条记录
					memcpy(&curoffset,Buf_ReadBuffer(buftag)+sizeofpagehead,sizeofrecord);
					readlength = curoffset.offset;
					memcpy(str,Buf_ReadBuffer(buftag)+PAGE_SIZE-curoffset.offset,readlength);
					str[readlength] = '\0';
					printf("该页面中第%d记录\n",j+1);
					printf("%s\n",str);
				}
				else{
					preoffset = curoffset;
					memcpy(&curoffset,Buf_ReadBuffer(buftag)+sizeofpagehead+sizeofrecord*j,sizeofrecord);
					readlength = curoffset.offset-preoffset.offset;
					memcpy(str,Buf_ReadBuffer(buftag)+PAGE_SIZE-curoffset.offset,readlength);
					str[readlength] = '\0';
					printf("该页面中第%d记录\n",j+1);
					printf("%s\n",str);
				}
			}
		}
		long nextPno = pagehead.nextPageNo;
		if(nextPno==-1){
			break;
		}
		else{
			CurpageNo = nextPno;
		}
	}
}

void file_deleteFile(struct Storage *DB,int FileID){
	int i;
	for(i=0;i<MAX_FILE_NUM;i++){
		if(DB->dbMeta.fileMeta[0].segList[i].id==FileID){			//找到文件对应的页
			break;
		}
	}
	long pagenum = DB->dbMeta.fileMeta[0].segList[i].pageNum;			//读取第一页的信息
	long CurpageNo = DB->dbMeta.fileMeta[0].segList[i].firstPageNo;
	long pageAddr = DB->dbMeta.dataAddr +CurpageNo * PAGE_SIZE;
	int sizeofpagehead = sizeof(struct PageMeta);
	int sizeofrecord = sizeof(struct OffsetInPage);
	long nextPage = -1;
	struct PageMeta pagehead;
	for(long j=0;j<pagenum;j++){							//遍历每一页
		rewind(DB->dbFile);
		fseek(DB->dbFile,pageAddr,SEEK_SET);
		fread(&pagehead,sizeofpagehead,1,DB->dbFile);			//读取这一页的内容
		nextPage = pagehead.nextPageNo;
		page_recove_onepage(DB,pagehead.pageNo);				//删除这一页
		if(nextPage>0){
			pageAddr = DB->dbMeta.dataAddr +nextPage * PAGE_SIZE;	//获取新的一页的地址
		}
		else{
			break;
		}
	}
	DB->dbMeta.blockFree += pagenum;
	DB->dbMeta.currFileNum--;
	DB->dbMeta.fileMeta[0].segList[i].type = -1;
	DB->dbMeta.fileMeta[0].segList[i].id = -1;
	DB->dbMeta.fileMeta[0].segList[i].firstPageNo = -1;
	DB->dbMeta.fileMeta[0].segList[i].pageNum = -1;
	
}
void file_read_sd(struct Storage *DB,long pageno,char *bufferpath){
	rewind(DB->dbFile);
	fseek(DB->dbFile,DB->dbMeta.dataAddr+pageno*PAGE_SIZE,SEEK_SET);
	fread(bufferpath,PAGE_SIZE,1,DB->dbFile);
}
void file_write_sd(struct Storage *DB,long pageno,char *bufferpath){
	rewind(DB->dbFile);
	fseek(DB->dbFile,DB->dbMeta.dataAddr+pageno*PAGE_SIZE,SEEK_SET);
	fwrite(bufferpath,PAGE_SIZE,1,DB->dbFile);
}
void file_print_freepace(struct Storage *DB){
	printf("已经用了%ld块，还空闲%ld块\n",DB->dbMeta.blockNum-DB->dbMeta.blockFree,DB->dbMeta.blockFree);
	
}