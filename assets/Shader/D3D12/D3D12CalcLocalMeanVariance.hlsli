#ifndef __D3D12CALCLOCALMEANVARIANCE_HLSLI__
#define __D3D12CALCLOCALMEANVARIANCE_HLSLI__

namespace CalcLocalMeanVariance {
    //-----------------------------------------------------------------------------------------------------------
    //
    // 체커보드 샘플링에서 비활성 픽셀을 아래쪽 활성 픽셀에 대응시킨다.
    //
    // @param pixel : 입력 픽셀 좌표
    //
    // @return 활성 픽셀의 좌표
    //
    //-----------------------------------------------------------------------------------------------------------
    int2 GetActivePixelIndex(int2 pixel) {
        const bool IsEvenPixel = ((pixel.x + pixel.y) & 1) == 0;
        return cbLocalMeanVar.CheckerboardSamplingEnabled && cbLocalMeanVar.EvenPixelActivated != IsEvenPixel ? pixel + int2(0, 1) : pixel;
    }
}

#endif // __D3D12CALCLOCALMEANVARIANCE_HLSLI__