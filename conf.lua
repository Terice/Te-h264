

--以下废弃

--字节读取类状态：
DE_BITBUF_REFRESH                = 0
--矩阵 开始矩阵，中间矩阵， 结果矩阵
DE_MATRIX_START                  = 0
DE_MATRIX_SINGLEVALUE            = 0
DE_MATRIX_RESULT                 = 0
--16x16残差
DE_RESIDUAL_16X16_ACANDDC        = 0
DE_RESIDUAL_16X16_TRANS4x4       = 0
DE_RESIDUAL_16X16_START          = 0
DE_RESIDUAL_4x4OF16X16_RESULT    = 0
DE_RESIDUAL_16X16_RESULT         = 0


--debug文件的用法：强制全部打印是指的Y Cb Cr

--强制打印所有CABAC的变量
DE_CABAC_CTRL                    = 0
--打印CABAC的运行时状态
DE_CABAC_STATE                   = 0
--打印CABAC的所有计算出来的二进制位，
DE_CABAC_BIN                     = 0
--打印CABAC的计算结果，ae(v)描述子的结果
DE_CABAC_RESULT                  = 0
--打印CABAC计算的残差结果
DE_RESIDUAL_CABAC_RESULT         = 0
--打印残差的变换系数
DE_RESIDUAL_TRANSCOEFF           = 0


--宏块信息和所有打印的控制
DE_MB                            = 0

--残差
--打印 Y Cb Cr 的残差

--强制全部打印
DE_RESIDUAL_RESULT               = 0

DE_RESIDUAL_RESULT_Y             = 0
DE_RESIDUAL_RESULT_Cr            = 0
DE_RESIDUAL_RESULT_Cb            = 0

--预测块

--强制全部打印
DE_PREDICTION_RESULT             = 0

--分别打印
DE_PREDICTION_RESULT_Y           = 0
DE_PREDICTION_RESULT_Cb          = 0
DE_PREDICTION_RESULT_Cr          = 0

--重建图像

--强制全部打印
DE_CONSPIC_RESULT                = 0

DE_CONSPIC_RESULT_Y              = 0
DE_CONSPIC_RESULT_Cb             = 1
DE_CONSPIC_RESULT_Cr             = 1

--运动矢量
DE_INTER_MOVEVECTOR              = 0
--最后的图像字符画
DE_PIC_TERMINATECHAR             = 0

--所有的和时间有关的打印
DE_TIMER                         = 0
--NAL信息的打印
DE_NAL_INFO                      = 0

DE_CONTROL_ALL                   = 0
print(">>conf:")
print("         DE_CABAC_STATE          :"..DE_CABAC_STATE          ) 
print("         DE_CABAC_BIN            :"..DE_CABAC_BIN            ) 
print("         DE_CABAC_RESULT         :"..DE_CABAC_RESULT         ) 
print("         DE_RESIDUAL_CABAC_RESULT:"..DE_RESIDUAL_CABAC_RESULT) 
print("         DE_RESIDUAL_TRANSCOEFF  :"..DE_RESIDUAL_TRANSCOEFF  ) 
print("         DE_MB                   :"..DE_MB                   ) 
print("         DE_RESIDUAL_RESULT      :"..DE_RESIDUAL_RESULT      ) 
print("         DE_RESIDUAL_RESULT_Y    :"..DE_RESIDUAL_RESULT_Y    ) 
print("         DE_RESIDUAL_RESULT_Cr   :"..DE_RESIDUAL_RESULT_Cr   ) 
print("         DE_RESIDUAL_RESULT_Cb   :"..DE_RESIDUAL_RESULT_Cb   ) 
print("         DE_PREDICTION_RESULT    :"..DE_PREDICTION_RESULT    ) 
print("         DE_PREDICTION_RESULT_Y  :"..DE_PREDICTION_RESULT_Y  ) 
print("         DE_PREDICTION_RESULT_Cb :"..DE_PREDICTION_RESULT_Cb ) 
print("         DE_PREDICTION_RESULT_Cr :"..DE_PREDICTION_RESULT_Cr ) 
print("         DE_CONSPIC_RESULT       :"..DE_CONSPIC_RESULT       ) 
print("         DE_CONSPIC_RESULT_Y     :"..DE_CONSPIC_RESULT_Y     ) 
print("         DE_CONSPIC_RESULT_Cb    :"..DE_CONSPIC_RESULT_Cb    ) 
print("         DE_CONSPIC_RESULT_Cr    :"..DE_CONSPIC_RESULT_Cr    ) 
print("         DE_INTER_MOVEVECTOR     :"..DE_INTER_MOVEVECTOR     ) 
print("         DE_PIC_TERMINATECHAR    :"..DE_PIC_TERMINATECHAR    ) 
print("         DE_TIMER                :"..DE_TIMER                ) 
print("         DE_NAL_INFO             :"..DE_NAL_INFO             ) 
print("         DE_CONTROL_ALL          :"..DE_CONTROL_ALL          ) 
if(DE_RESIDUAL_RESULT == 1) then 
    DE_RESIDUAL_RESULT_Y             = 1
    DE_RESIDUAL_RESULT_Cr            = 1
    DE_RESIDUAL_RESULT_Cb            = 1
end
if(DE_PREDICTION_RESULT == 1) then
    DE_PREDICTION_RESULT_Y           = 1
    DE_PREDICTION_RESULT_Cb          = 1
    DE_PREDICTION_RESULT_Cr          = 1
end
if(DE_CONSPIC_RESULT == 1) then
    DE_CONSPIC_RESULT_Y              = 1
    DE_CONSPIC_RESULT_Cb             = 1
    DE_CONSPIC_RESULT_Cr             = 1
end
if(DE_CABAC_CTRL == 1) then
    DE_CABAC_STATE                   = 1
    DE_CABAC_BIN                     = 1
    DE_CABAC_RESULT                  = 1
end

return
DE_CABAC_STATE, 
DE_CABAC_RESULT, 
DE_CABAC_BIN, 

DE_RESIDUAL_CABAC_RESULT,

DE_RESIDUAL_TRANSCOEFF, 

DE_MB,

DE_RESIDUAL_RESULT_Y, 
DE_RESIDUAL_RESULT_Cb, 
DE_RESIDUAL_RESULT_Cr, 

DE_PREDICTION_RESULT_Y,
DE_PREDICTION_RESULT_Cb,
DE_PREDICTION_RESULT_Cr,

DE_CONSPIC_RESULT_Y , 
DE_CONSPIC_RESULT_Cb , 
DE_CONSPIC_RESULT_Cr,

DE_INTER_MOVEVECTOR,
DE_PIC_TERMINATECHAR,
DE_TIMER,
DE_NAL_INFO,
DE_CONTROL_ALL