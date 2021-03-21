#include "picture.h"

#include "gchart.h"
#include "gvars.h"
#include "terror.h"

#include "parser.h"
#include "decoder.h"
#include "sps.h"
#include "slice.h"
#include "macroblock.h"
#include "pixmap.h"

static int count_pic = 0;

picture::picture(parser* p, decoder* d)
{
    this->pa = p;
    this->de = d;
    this->IdrPicFlag = false;
    count_mb = 0;
    state_ref = Nun_ref;
    
    // 如果开始解picture了，那么说明sps和pps一定都出现过了，
    // 所以这里可以使用这些参数
    
    mb = new array2d<macroblock*>(pa->pV->PicWidthInMbs ,pa->pV->PicHeightInMbs, NULL);
    size.w = pa->pV->PicWidthInMbs  * 16;
    size.h = pa->pV->PicHeightInMbs * 16;

    // 预测值和残差值不再分配统一的空间了，由每个宏块分别储存
    // pred = new array2d<pix16>(size.w, size.h, 0);
    // resi = new array2d<int16>(size.w, size.h, 0);

    // 重建图像的像素值统一管理，宏块使用 pixmap 构造指针来访问这里面的数据
    cons = new array2d <byte>(size.w, size.h);
}
picture::~picture()
{
    macroblock *tofree = NULL;
    for (int i = 0; i < mb->l; i++)
    {
        tofree = mb->get(i);
        if(tofree) delete tofree;
    }
    
    // 删除宏块组
    delete mb;

    // delete pred;
    // delete resi;
    delete cons;

    // 删除slice
    // 放在这里最后一起处理，因为参考标记还需要slice的一些数据
    delete sl;
}

void picture::deocde()
{
    sl = new slice(pa, de, this);
    // 设置解析器当前的slice
    // 由于简化了，所以这里就只有一个slice
    pa->cur_slice = sl;
    de->set_CurrentSlice(sl);
    sl->decode();

    if(sl->type == B)
    {
        if(sl->ps->direct_spatial_mv_pred_flag)
        {
            std::cout << "spatial" << std::endl;
        }
        else
        {
            std::cout << "temporal" << std::endl;
        }
    }

    if(terr.picture_mbcomplete())
    print();

    std::cout << count_pic++ << std::endl;
    drawpic();
}

void detect(macroblock *current, MacroBlockNeighInfo *info, int x, int y, int dx, int dy, picture* pic)
{
    int pos_x = x + dx;
    int pos_y = y + dy;
    if(pos_x < 0 || pos_y < 0 || pos_x >= pic->mb->w || pos_y >= pic->mb->h)
    {
        info->avaiable = false;
        info->pointer = NULL;
        return ;
    }
    macroblock* target = (pic->mb->get(pos_x, pos_y));
    if(!target) 
    {
        info->avaiable = false;
        info->pointer = NULL;
        return ;
    }
    else
    {
        info->avaiable = current->is_avaiable(target);
        info->pointer = target;
    }
    
}
void picture::takein(macroblock *m)
{
    int pos_x = 0;
    int pos_y = 0;
    int curpos_x = m->pos.x;
    int curpos_y = m->pos.y;

    (*mb)[curpos_y][curpos_x] = m;
    count_mb++;
    // std::cout << "[" << curpos_x << "," << curpos_y << "]" << std::endl;
    MacroBlockNeigh *neigh = &m->neighbour;
    detect(m, &m->neighbour.A, curpos_x, curpos_y, -1, 0, this);
    detect(m, &m->neighbour.B, curpos_x, curpos_y,  0,-1, this);
    detect(m, &m->neighbour.C, curpos_x, curpos_y, +1,-1, this);
    detect(m, &m->neighbour.D, curpos_x, curpos_y, -1,-1, this);
    
}

void picture::refidc(int nal_ref_idc)
{
    //解码完毕需要对当前的pic进行标记，
    //在下一次解码的时候会初始化，重排序和修改，而不是这里
    if(nal_ref_idc != 0)
    {
        //第一步：所有的片已经解码
        //不考虑场的问题，到这里就已经是完成解码了

        //第二步：参考队列中的pic的标记
        if(this->IdrPicFlag)
        {
            //I所有参考pic标记为不用于参考
            //II
            if(sl->ps->long_term_reference_flag == 0)
            {
                de->MaxLongTermFrameIdx = -1;
                this->state_ref = Ref_short;
            }
            else
            {
                de->MaxLongTermFrameIdx = 0;
                this->state_ref = Ref_long;
                this->LongTermFrameIdx = 0;
            }
        }
        else
        {
            //标记
            if(sl->ps->adaptive_ref_pic_marking_mode_flag == 0)
            //滑窗标记操作：帧队列满那么去掉最小PicNum的pic
            {
                de->ctrl_FIFO(pa->pS->sps->max_num_ref_frames);
                /*滑窗标记*/
            }
            else//自适应标记在slice头就会完成，
            {
                de->ctrl_MMOC();
                /*自适应标记*/
            }
        }

        //第三步，当前pic的标记
        //自适应内存标记6号就是标记当前pic
        //如果不是     由内存控制标记6标记的长期帧     ，
        //那么标记为短期帧
        if(!this->IdrPicFlag && !(this->is_UsedForLong() && this->memory_management_control_operation == 6))
        {
            this->state_ref = Ref_short;
        }

        //标记完毕，加入到参考队列中并分别加入参考表
        de->add_ReferenPic(this);
    }
}
void picture::print()
{
    // 宏块是否齐全的输出
    // NULL 会用 ～ 标识
    // 非空宏块指针会用其 type 的调试值标识
    printf(">>pic  :\n");
    for (size_t i = 0; i < this->mb->h; i++)
    {
        if(i%4 == 0) 
        {
            printf("%-2lu ", i/4 * 4);
            int width = this->mb->w/4+1;
            for (size_t j = 1; j < width; j++)
            {
                printf("---------%-2d", j*4);
            }
            printf("\n");
        }
        for (size_t j = 0; j < this->mb->w; j++)
        {
            if(j%4 == 0) printf(" | ");
            if((*this->mb)[i][j] != NULL) printf("%2d", mb[0][i][j]->mb_type);
            else printf("~~");
        }
        printf(" |\n");
    }
    printf("--IDX:[%4d] POC:[%4d]\n", count_pic++, POC);
    if(count_mb < mb->w * mb->h)
    {
        printf(">>pic  : [%4d]/[%4d]", mb->w * mb->h, count_mb);
        terr.texit(-1);
    }
}

// 用于将 像素值 转换为字符的转换表
// 大概能看出来像素值的多与少就行了吧
const char out_char[] = 
{
    ' ', '-', '+', ':', '~',
    '*', 'i', 'c', 'x', 'e',
    'm', 'O', '3', '6', '8',
    'E', 'G', 'R', 'N', 'B',
    'M', 'W', '#', '@', '&', 
};

#include <string>
#include <sstream>
void picture::drawpic()
{
    //可以指定的字符画宽高比例
    int hei_scal = 1;
    int wid_scal = 2;
    //
    int out_h = mb->h * 16 / hei_scal;
    int out_w = mb->w * 16 / wid_scal;
    array2d<char> out_CharMatrix(out_w + 1, out_h + 1, 0);// = new array2d<char>(height_mb * 16 / hei_scal + 1, width_mb * 16 / wid_scal + 1, 0);
    
    std::stringstream ss;
    ss << "frame" << count_pic;

    std::string name;
    getline(ss, name);
    // std::cout << "write file: " << name << std::endl;

    FILE *fp = fopen(name.c_str(), "w");
    fwrite(cons->data, cons->w * cons->h, sizeof(char), fp);
    fclose(fp);

    if(count_pic == 10)
        exit(0);

    // char tmp = 0;
    // for (size_t y = 0; y < out_h; y += hei_scal)
    // {
    //     for (size_t x = 0; x < out_w; x+= wid_scal)
    //     {
    //         //除以10拿到字符画单元的范围，25- 表示把亮度的高低值颠倒
    //         tmp = out_char[25 - ((cons[0][y][x]) / 10)];
    //         out_CharMatrix[y / hei_scal][x / wid_scal] = tmp;
    //         printf("%c", tmp);
    //     } 
    //     //每一行的左后一个像素后面加上换行符
    //     out_CharMatrix[y / hei_scal][out_w] = '\n';
    //     printf("\n");
    // }
    // //每一帧的结尾最后一个字符指定为结束符
    // out_CharMatrix[out_h][out_w] = '\0';
    // printf("%s", out_CharMatrix.data);

    return ;
}