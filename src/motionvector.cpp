#include "macroblock.h"



static inline int MallocMotionVector(MotionVector **mv, int MbPart, SubMacroBlock* SubPart)
{
    mv = new MotionVector*[MbPart];
    if(SubPart) //r
        for (int i = 0; i < MbPart; i++)
        {
            mv[i] = new MotionVector[SubPart[i].part];
        }
    else 
        for (int i = 0; i < MbPart; i++)
        {
            mv[i] = new MotionVector[1];
        }
}
static inline int FreeMotionVector(MotionVector **mv, int MbPart, SubMacroBlock* SubPart)
{
    if(SubPart) //r
        for (int i = 0; i < MbPart; i++)
        {
            delete[] mv[i];
        }
    else 
        for (int i = 0; i < MbPart; i++)
        {
            delete[] mv[i];
        }
    delete[] mv;
}
int MallocMotionVectorPkg(MotionVector_info *m, int MbPart, SubMacroBlock* SubPart)
{
    MallocMotionVector(m->mv_l0, MbPart, SubPart);
    MallocMotionVector(m->mv_l1, MbPart, SubPart);
    MallocMotionVector(m->mvp_l0, MbPart, SubPart);
    MallocMotionVector(m->mvp_l0, MbPart, SubPart);
    MallocMotionVector(m->mvd_l0, MbPart, SubPart);
    MallocMotionVector(m->mvd_l0, MbPart, SubPart);
}
int FreeMotionVectorPkg(MotionVector_info *m, int MbPart, SubMacroBlock* SubPart)
{
    FreeMotionVector(m->mv_l0, MbPart, SubPart);
    FreeMotionVector(m->mv_l1, MbPart, SubPart);
    FreeMotionVector(m->mvp_l0, MbPart, SubPart);
    FreeMotionVector(m->mvp_l0, MbPart, SubPart);
    FreeMotionVector(m->mvd_l0, MbPart, SubPart);
    FreeMotionVector(m->mvd_l0, MbPart, SubPart);
}