
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include"b_plus_tree.h"
#include<iostream>
#include<string>
using namespace std;

#define INDEX_DAT "b_plus_tree_index.dat"


int main()
{
	FILE *fp;
	int key_num = 20;
	Record record_insert;
 int x = 1;
    int y = 0;
		fp=fopen(INDEX_DAT,"rb+");
	if (fp == NULL){
		printf("index is null,rebuild index....\n");//�����ļ������ڣ����½�������
		FILE *fp_create;
		fp_create=fopen (INDEX_DAT, "wb");
		fclose(fp_create);
		fp=fopen(INDEX_DAT, "rb+");
		if (fp == NULL){
			printf("fopen error!\n");
			exit(0);
		}
	}
    while (0 != x)
    {
        printf("\n\n");
        printf("    *******************************************************************\n");
        printf("    *           Welcome to BPlusTree Program!                         *\n");
        printf("    *           1.Create B+ Tree                                      *\n");
        printf("    *           2.Search a number form B+ Tree                                  *\n");
        printf("    *           3.Insert a number form B+ Tree                         *\n");
        printf("    *           4.Delete a number form B+ Tree                                  *\n");
		printf("    *           5.Print B+ Tree										   *\n");			
        printf("    *           0.Exit                                          *\n");
        printf("    *******************************************************************\n");
        printf("\n    Select a number:");

       
        scanf("%d", &x);
		int k = 0;
		int w=0;
		Node node;
        switch (x)
        {
        case 1:
	
			rewind(fp);//�൱��fseek���ļ�ָ�룬0,0��������˵������ʹ�ļ��ڵ�ָ���ֽڵ�ָ��ص���ͷ������ָ���һ���ֽ�
			for (k= 0; k<=8; k++)
			{
				record_insert.key = k;		
				record_insert.pos = k* 2;
				insert(fp, record_insert);
			}
			rewind(fp);
		
//			for(w=0;w<node.count;w++)   //
//			{
//		    	printf("%d ,",node.pair[w].key);
//			}
 //          break;
        
        case 2:
			int c;
			printf("input the key you search");
			scanf("%d",&c);
           	search(fp,c);
			printf("search succeed");
			break;

        case 3:
            
			int b;
			printf("input the key you want insert");
			scanf("%d",&b);
			record_insert.key = b;		
			record_insert.pos = b* 2;
           	insert(fp,record_insert);
			printf("insert succeed");
		 
            break;

        case 4:
		
			int a;
			printf("input the key you want delet");
			scanf("%d",&a);
           	del(fp,a);
			printf("delet succeed");
            break;
		case 5:
			Node node;
			getRoot(fp, node);
			int rootOffSet;
			Node rootNode;
			rewind(fp);
			fread(&rootOffSet, 1, sizeof(rootOffSet), fp);
			fseek(fp, rootOffSet, SEEK_SET);
			fread(&node, 1, sizeof(node), fp);
			showTree(fp,node);
			break;

        case 0:
         fclose(fp);
            return 0;
            break;

        default:
            break;
        }
    }
}