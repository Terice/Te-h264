#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct fstate__
{
    FILE* fp;
    char state;
    char name[256];
}fstate;


typedef struct teana__
{
    unsigned long line_cur;
    FILE* fp_cur;//当前正在使用的文件指针
    int idx_cur;// 当前正在使用的索引号
    fstate* files[20];
    int files_l;
    char str_tmp[256];// 临时字符串缓冲
}teana;

// 初始化一个
teana *InitTeana()
{
    teana * te = (teana*)malloc(sizeof(teana));
    te->fp_cur = NULL;
    te->idx_cur = 0;
    te->files_l = 0;
    te->line_cur = 0;

    return te;
}
// 删除一个
void DeltTeana(teana *te)
{
    for (int i = 0; i < te->files_l; i++)
    {
        free(te->files[i]);
    }
    free(te);
}
// 打印多行，指针不动，只能向后多行
void MulLine(teana *te)
{
	FILE* fp = te->fp_cur;
	unsigned long position = ftell(fp);
	fgets(te->str_tmp, 256, fp);
    printf("%6d |%s", te->line_cur + 0, te->str_tmp);
	fgets(te->str_tmp, 256, fp);
    printf("%6d |%s", te->line_cur + 1, te->str_tmp);
	fgets(te->str_tmp, 256, fp);
    printf("%6d |%s", te->line_cur + 2, te->str_tmp);
	
    fseek(fp, position, SEEK_SET);
}

// 打印一行，指针不动 - p
void PrtLine(teana *te)
{
    FILE* fp = te->fp_cur;
    unsigned long position = ftell(fp);
    fgets(te->str_tmp, 256, fp);
    fseek(fp, position, SEEK_SET);
    printf("%6d |", te->line_cur);
    puts(te->str_tmp);
}
// 去下一行 - n
void NxtLine(teana *te)
{
    FILE* fp = te->fp_cur;
    while (fgetc(fp)!= '\n'){};
    te->line_cur++;
}
// 去上一行 - u
void PreLine(teana *te)
{
    FILE* fp = te->fp_cur;
    int l = te->line_cur;
    l--;
	fseek(te->fp_cur, 0L, SEEK_SET);
    while(l)
    {
        if(fgetc(te->fp_cur) == '\n')
            l--;
    }
    te->line_cur = te->line_cur == 0 ? 0 : te->line_cur-1;
}
// 去到某一行 - g
void GotLine(teana *te)
{
    int l;
    printf("-> input line: ");
    scanf("%d", &l);getchar();
    te->line_cur = l;
    fseek(te->fp_cur, 0L, SEEK_SET);
    while(l)
    {
        if(fgetc(te->fp_cur) == '\n')
            l--;
    }
}
// 加载一个文件（只读） - o
void LodFile(teana *te)
{
    fstate* files;
    int files_l = te->files_l;
    char *str = te->str_tmp;

    printf("-> input file: ");
    scanf("%s", str);getchar();

    FILE* fp = fopen(str, "r");
    if(fp)
    {
        files = (fstate*)malloc(sizeof(fstate));
        files->fp = fp;
        files->state = 'o';
        strcpy(files->name, str);
        te->files[files_l] = files;
        te->files_l++;
    }
    else fprintf(stderr, "-- open err");
}
void ClsFile(teana *te)
{
    int i;
    printf("-> input indx: ");
    scanf("%d", &i);getchar();
    fclose(te->files[i]->fp);

    te->files[te->idx_cur]->state = 'n';
}
// 查看文件的状态 - c
void SteFile(teana *te)
{
    for (int i = 0; i < te->files_l; i++)
    {
        printf(":: [%2d], %c, %s\n", i, te->files[i]->state, te->files[i]->name);
    }
}
// 选择一个文件 - s
void SelFile(teana *te)
{
    int i;
    SteFile(te);
    printf("-> input file: ");
    scanf("%d", &i);getchar();
    te->files[te->idx_cur]->state = 'o';
    te->idx_cur = i;
    te->files[i]->state = 's';
    te->fp_cur = te->files[i]->fp;
}

// 比较两个文件，找出不同的行，并且打印这两行及其不匹配的地方
// 文件指针停留在某一行，按照命令输出这个区域的周围行
// set line 5 设置宽度为 5
// set file 1 设置当前文件槽 
// gto number 去到某一行
// pre 向上一行
// nxt 向下一行
// prt 打印
// 截断文件前面的几行并且输出到另一个文件


int DetectDifferent(FILE* fp_1, FILE* fp_2)
{

    int line = 0;
    int coln = 0;
    int ch_1 = 0, ch_2 = 0;
    while (ch_1 == ch_2)
    {
        ch_1 = fgetc(fp_1);
        ch_2 = fgetc(fp_2);
        if(ch_1 == '\n')
        {
            line++;
            coln = 0;
        }
        coln++;
    }
    printf("at : row:[%6d], col:[%6d]\n", line, coln);
    fseek(fp_1, -1 * coln + 1, SEEK_CUR);
    fseek(fp_2, -1 * coln + 1, SEEK_CUR);
    char tmp[128];
    fgets(tmp, 128, fp_1);
    printf("1 - %s\n", tmp);
    fgets(tmp, 128, fp_2);
    printf("2 - %s\n", tmp);
	return 0;
}
void ComFile(teana* te)
{
	int findex1, findex2;
	printf("-> input num1 & num2: ");
	scanf("%d %d",&findex1, &findex2);getchar(); 
	rewind(te->files[findex1]->fp);
	rewind(te->files[findex2]->fp);
	DetectDifferent(te->files[findex1]->fp, te->files[findex2]->fp);
}


int cutn(FILE* fp_1, FILE* fp_2, int k)
{
    int line = 0;
    int coln = 0;
    int ch_1 = 0, ch_2 = 0;
    while (k)
    {
        ch_1 = fgetc(fp_1);
        if(ch_1 == '\n')
        {
            k--;
        }
    }
    do
    {
        ch_1 = fgetc(fp_1);
        fputc(ch_1, fp_2);
    } while (ch_1 != EOF);
    	return 0;
}

// 从 fp_in 截取start 到 end 中间的数据并且写出到 fp_out 中去
void CutFile(FILE* fp_in, unsigned long start, unsigned long end, FILE* fp_out)
{
    if(end == 0)
    {
        fseek(fp_in, 0L, SEEK_END);
        end = ftell(fp_in);
        rewind(fp_in);
    }
    else
    {
        if(start >= end) return;
    }
    fseek(fp_in, start, SEEK_SET);
    unsigned long cur = start;
    while (cur++ < end)
    {
        fputc(fgetc(fp_in), fp_out);
    }
}

void AnalyseArgc(int argc, char* argv[])
{
    if(argc == 1) printf("-> Usage: [teana] [command]\n");
    else if(argc == 2)
    {
        if(!strncmp(argv[1], "-h", 3));// help message
        if(!strncmp(argv[1], "-c", 3))// command 模式
        {
            teana *te = InitTeana();
            int ch;
            do
            {
                printf("-> input comd: ");
                ch = getchar();if(ch!= '\n') getchar();
                     if(ch == 'o') LodFile(te);
                else if(ch == 'p') PrtLine(te);
                else if(ch == 'u') PreLine(te);
                else if(ch == 'n') NxtLine(te);
                else if(ch == 's') SelFile(te);
                else if(ch == 'c') SteFile(te);
				else if(ch == 'g') GotLine(te);
				else if(ch == 'a') ComFile(te);
				else if(ch == 'm') MulLine(te);
				else if(ch == 'd') ClsFile(te);
                else if(ch == 'h') 
                printf("\
                o - open\n\
                d - close\n\
                s - select file\n\
                p - print\n\
                m - mulprint\n\
                u - preline\n\
                n - nextline\n\
                g - goto line\n\
                a - cmp two file\n\
                ");
            } while (ch != 'q');
            DeltTeana(te);
        }
    }
    else if(argc == 4)
    {
        FILE* fp_in = fopen(argv[1], "r");
        FILE* fp_out = fopen(argv[2], "w");
        CutFile(fp_in, atoi(argv[3]), 0L, fp_out);
    };
}


#define FILE_INPUT_1 argv[1]
#define FILE_INPUT_2 argv[2]

int main(int argc, char* argv[])
{

	AnalyseArgc(argc, argv);
	return 0;
}
