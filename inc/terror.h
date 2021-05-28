#ifndef TERROR_H__
#define TERROR_H__

#include <string>
#define DE_FUNC(name)\
bool name()\
{\
    if(de.control_all)\
    return de.name;\
    else\
    return false;\
};


typedef struct DebugControl__
{
    char cabac_state_running            ;
    char cabac_result_ae                ;
    char cabac_result_bin               ;
    char cabac_result_residual          ;
    char residual_transcoeff            ;
    char macroblock                     ;
    char picture_mbcomplete             ;
    char residual_result_Y              ;
    char residual_result_Cb             ;
    char residual_result_Cr             ;
    char prediction_result_Y            ;
    char prediction_result_Cb           ;
    char prediction_result_Cr           ;
    char conspic_result_Y               ;
    char conspic_result_Cb              ;
    char conspic_result_Cr              ;
    char inter_movevector               ;
    char pic_terminalpic                ;
    char timer                          ;
    char nal_info                       ;
    char control_all                    ;
}DebugControl;

class terror
{
private:
    int index;
    std::string msg;
    void tconf(std::string conf);
public:
    DebugControl de;
    // 退出程序
    void texit(int);
    void temsg(std::string s);
    void temsg(char *s);

    // 抛出一个错误并且停止程序
    // 用在发生致命的错误后面
    /// @param str [发生错误的对象](发生了怎样的错误)
    void error(std::string str);
    // 
    DE_FUNC(cabac_state_running   );
    DE_FUNC(cabac_result_ae       );
    DE_FUNC(cabac_result_bin      );
    DE_FUNC(cabac_result_residual );
    DE_FUNC(residual_transcoeff   );
    DE_FUNC(macroblock            );
    DE_FUNC(residual_result_Y     );
    DE_FUNC(residual_result_Cb    );
    DE_FUNC(residual_result_Cr    );
    DE_FUNC(prediction_result_Y   );
    DE_FUNC(prediction_result_Cb  );
    DE_FUNC(prediction_result_Cr  );
    DE_FUNC(conspic_result_Y      );
    DE_FUNC(conspic_result_Cb     );
    DE_FUNC(conspic_result_Cr     );
    DE_FUNC(inter_movevector      );
    DE_FUNC(pic_terminalpic       );
    DE_FUNC(timer                 );
    DE_FUNC(nal_info              );
    DE_FUNC(control_all           );
    DE_FUNC(picture_mbcomplete    );
    terror(/* args */);
    ~terror();
};


#endif