#include "file.h"
#include "storage.h"


//-1表示文件创建失败
void file_newFile(struct Storage *DB,int type, long NeededPageNum){
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
			rewind(DB->dataPath);
			fseek(DB->dataPath,DB->dbMeta.dataAddr+pagemeta.pageNo*PAGE_SIZE,SEEK_SET);
			fwrite(&pagemeta,sizeof(pagemeta),1,DB->dataPath);
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
		
		
	}
	else{
		printf("未有足够的连续存储空间，文件创建失败！/n");
		exit(0);
	}
	
}

void file_writeFile(struct Storage *DB,int length,char *str,int FileID){
	int querypage=-1;
	int i;
	for( i=0;i<MAX_FILE_NUM;i++){
		if(DB->dbMeta.fileMeta[0].segList[i].id==FileID){
			querypage=DB->dbMeta.fileMeta[0].segList[i].firstPageNo;
			break;
		}
	}
	if(querypage==-1){
		printf("该文件id对应的文件不存在！");
		exit(0);
	}

	int mapNo = page_requestPage(DB,querypage);
	long CurpageNo = DB->dbMeta.fileMeta[0].segList[i].firstPageNo;
	long pagenum = DB->dbMeta.fileMeta[0].segList[i].pageNum;

	int sizeofpagehead = sizeof(struct PageMeta);
	int sizeofrecord = sizeof(struct OffsetInPage);
	rewind(DB->dataPath);
	bool isfound = false;
	struct PageMeta pagehead;
	memcpy(&pagehead,DB->bufPool.data[mapNo],sizeofpagehead);
	OffsetInPage preoffset,curoffset;
	long currecordpos,curoffsetpos;
	for(int i=0;i<pagenum;i++){
		if(pagehead.freeSpace<=length+sizeofrecord){
			if(pagehead.nextPageNo==-1){
				break;
			}
			CurpageNo = pagehead.nextPageNo;

			mapNo = page_requestPage(DB,CurpageNo);
			memcpy(&pagehead,DB->bufPool.data[mapNo],sizeofpagehead);

			continue;	
		}
		else{
			memcpy(&preoffset,DB->bufPool.data[mapNo]+sizeofpagehead,sizeofrecord);
			isfound = true;
			if(pagehead.recordNum==0){
				curoffset.recordID = 0;
				curoffset.offset = length;
				curoffset.isDeleted = false;
				currecordpos = sizeofpagehead;
				curoffsetpos =  PAGE_SIZE - length;
				
			}
			else{
				memcpy(&preoffset,DB->bufPool.data[mapNo]+sizeofpagehead+(pagehead.recordNum-1)*sizeof,sizeofrecord);
				curoffset.recordID = pagehead.recordNum;
				curoffset.offset = preoffset.offset+length;
				curoffset.isDeleted = false;
				currecordpos = sizeofpagehead + sizeofrecord*pagehead.recordNum;
				curoffsetpos = PAGE_SIZE - preoffset.offset-length;
				
			}
			
		}
		pagehead.recordNum++;
		pagehead.freeSpace=pagehead.freeSpace-length;
		memcpy(DB->bufPool.data[mapNo],&pagehead,sizeofpagehead);
		memcpy(DB->bufPool.data[mapNo]+currecordpos,&curoffset,sizeofrecord);
		memcpy(DB->bufPool.data[mapNo]+curoffsetpos,str,length);
		DB->bufPool.data[mapNo].isChanged = true;
		break;
	}
	if(!isfound){
		long pagenumber = page_requestPage(DB,1);
		if(pagenumber>=0){
			struct PageMeta pagemeta;
			pagemeta.nextPageNo=-1;
			pagemeta.prePageNo=pagehead.pageNo;
			pagemeta.pageNo=pagenumber;
			pagehead.nextPageNo = pagenumber;
			pagemeta.recordNum = 1;
			pagemeta.freeSpace = PAGE_SIZE - length - sizeofpagehead - sizeofrecord;
			curoffsetpos = PAGE_SIZE-length;
			currecordpos = sizeofpagehead;
			curoffset.recordID = 0;
			curoffset.offset = length;
			curoffset.isChanged = false;

			mapNo = page_requestPage(DB,pagenumber);
			memcpy(DB->bufPool.data[mapNo],&pagemeta,sizeofpagehead);
			memcpy(DB->bufPool.data[mapNo]+currecordpos,&curoffset,sizeofrecord);
			memcpy(DB->bufPool.data[mapNo]+curoffsetpos,str,length);
			DB->bufPool.data[mapNo].isChanged = true;
			mapNo = page_requestPage(DB,pagehead.pageNo);
			memcpy(DB->bufPool.data[mapNo],&pagehead,sizeofpagehead);
			
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
		int mapNo = page_requestPage(DB,CurpageNo);
		memcpy(&pagehead,DB->bufPool.data[mapNo],sizeofpagehead);

		printf("第%d号文件中的第%d个页面\n",FileID,i+1);
		printf("页号：%ld\n",pagehead.pageNo);
		printf("前继页号：%ld\n",pagehead.prePageNo);
		printf("后继页号：%ld\n",pagehead.nextPageNo);
		printf("记录个数；%d\n",pagehead.recordNum);
		printf("空闲空间：%ld\n",pagehead.freeSpace);
		if(pagehead.recordNum>0){
			for(int j=0;j<pagehead.recordNum;j++){
				int readlength;
				if(j==0){
					memcpy(&curoffset,DB->bufPool.data[mapNo]+sizeofpagehead,sizeofrecord);
					readlength = curoffset.offset;
					memcpy(str,DB->bufPool.data[mapNo]+PAGE_SIZE-curoffset.offset,readlength);
					str[readlength] = '\0';
					printf("该页面中第%d记录\n",j+1);
					printf("%s\n",str);
				}
				else{
					preoffset = curoffset;
					memcpy(&curoffset,DB->bufPool.data[mapNo]+sizeofpagehead+sizeofrecord*j,sizeofrecord);
					readlength = curoffset.offset-preoffset.offset;
					memcpy(str,DB->bufPool.data[mapNo]+PAGE_SIZE-curoffset.offset,readlength);
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
		if(DB->dbMeta.fileMeta[0].segList[i].id==FileID){
			break;
		}
	}
	long pagenum = DB->dbMeta.fileMeta[0].segList[i].pageNum;
	long CurpageNo = DB->dbMeta.fileMeta[0].segList[i].firstPageNo;
	long pageAddr = DB->dbMeta.dataAddr +CurpageNo * PAGE_SIZE;
	int sizeofpagehead = sizeof(struct PageMeta);
	int sizeofrecord = sizeof(struct OffsetInPage);
	long nextPage = -1;
	struct PageMeta pagehead;
	for(long j=0;j<pagenum;j++){
		rewind(DB->dataPath);
		fseek(DB->dataPath,pageAddr,SEEK_SET);
		fread(&pagehead,sizeofpagehead,1,DB->dataPath);
		nextPage = pagehead.nextPageNo;
		page_recove_onepage(DB,pagehead.pageNo);
		if(nextPage>0){
			pageAddr = DB->dbMeta.dataAddr +nextPage * PAGE_SIZE;
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
