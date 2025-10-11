
//------------------------------------------------------------------------------
// idct.cpp
//------------------------------------------------------------------------------

#include "idct.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

std::array<float, 64> cosTable() {
    std::array<float, 64> table;
    
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            table[x + y * 8] = std::cosf((2.f * x + 1.f) * y * std::numbers::pi / 16.f);
        }
    }
    
    return table;
}

}

image::DataUnit image::idct_float(const DataUnit& du) {
    DataUnit out;
    
    for (size_t y = 0; y < 8; y++) {
        for (size_t x = 0; x < 8; x++) {
            float val = 0.f;
            
            for (size_t u = 0; u < 8; u++) {
                for (size_t v = 0; v < 8; v++) {
                    float cu = (u == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    float cv = (v == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    val += cu * cv * du[u + v*8]
                            * std::cos((2.f * x + 1.f) * u * std::numbers::pi / 16.f)
                            * std::cos((2.f * y + 1.f) * v * std::numbers::pi / 16.f);
                }
            }
            val *= 0.25f;
            
            out[x + y * 8] = static_cast<int>(val);
        }
    }
    
    return out;
}

image::DataUnit image::idct_float_table(const DataUnit& du) {
    static auto table = cosTable();
    DataUnit out;
    
    for (size_t y = 0; y < 8; y++) {
        for (size_t x = 0; x < 8; x++) {
            float val = 0.f;
            
            for (size_t u = 0; u < 8; u++) {
                for (size_t v = 0; v < 8; v++) {
                    float cu = (u == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    float cv = (v == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    val += cu * cv * du[u + v*8]
                            * table[x + u * 8]
                            * table[y + v * 8];
                }
            }
            val *= 0.25f;
            
            out[x + y * 8] = static_cast<int>(val);
        }
    }
    
    return out;
}

std::array<int, 8> image::loeffler_1d_dct(const std::array<int, 8> in) {
    ///
    //stage 1
    ///
    std::array<float, 8> stage1;
    stage1[0] = in[0] + in[7];
    stage1[1] = in[1] + in[6];
    stage1[2] = in[2] + in[5];
    stage1[3] = in[3] + in[4];
    stage1[4] = in[3] - in[4];
    stage1[5] = in[2] - in[5];
    stage1[6] = in[1] - in[6];
    stage1[7] = in[0] - in[7];
    
    ///
    //stage 2
    ///
    std::array<float, 8> stage2;
    stage2[0] = stage1[0] + stage1[3];
    stage2[1] = stage1[1] + stage1[2];
    stage2[2] = stage1[1] - stage1[2];
    stage2[3] = stage1[0] - stage1[3];
    //stage 2, c3
    stage2[4] = stage1[4] * std::cosf(3.f * std::numbers::pi / 16.f)
                + stage1[7] * std::sinf(3.f * std::numbers::pi / 16.f );
    stage2[7] = - stage1[4] * std::sinf(3.f * std::numbers::pi / 16.f)
                + stage1[7] * std::cosf(3.f * std::numbers::pi / 16.f );
    //stage 2, c1
    stage2[5] = stage1[5] * std::cosf(3.f * std::numbers::pi / 16.f)
                + stage1[6] * std::sinf(3.f * std::numbers::pi / 16.f );
    stage2[6] = - stage1[5] * std::sinf(3.f * std::numbers::pi / 16.f)
                + stage1[6] * std::cosf(3.f * std::numbers::pi / 16.f );
    
    ///
    //stage3
    ///
    std::array<float, 8> stage3;
    stage3[0] = stage2[0] + stage2[1];
    stage3[1] = stage2[0] - stage2[1];
    stage3[2] = stage2[2] * std::sqrtf(2.f) * std::cosf(std::numbers::pi / 16.f)
                + stage2[3] * std::sqrtf(2.f) * std::sinf(std::numbers::pi / 16.f );
    stage3[3] = - stage2[2] * std::sqrtf(2.f) * std::sinf(std::numbers::pi / 16.f)
                + stage2[3] * std::sqrtf(2.f) * std::cosf(std::numbers::pi / 16.f );
    stage3[4] = stage2[4] + stage2[6];
    stage3[5] = stage2[7] - stage2[5];
    stage3[6] = stage2[4] - stage2[6];
    stage3[7] = stage2[7] + stage2[5];
    
    ///
    //stage4
    ///
    std::array<int, 8> stage4;
    stage4[0] = stage3[0];
    stage4[1] = stage3[7] + stage3[4];
    stage4[2] = stage3[2];
    stage4[3] = stage3[5] * std::sqrtf(2.f);
    stage4[4] = stage3[1];
    stage4[5] = stage3[6] * std::sqrtf(2.f);
    stage4[6] = stage3[3];
    stage4[7] = stage3[7] - stage3[4];
    
    return stage4;
}

void image::loeffler_1d_idct_row(const std::array<int, 8*8>& du, std::array<float, 8*8>& inter, int offset) {
    ///
    //stage4
    ///
    std::array<float, 8> stage4;
    stage4[0] = du[0 + offset];
    stage4[1] = du[4 + offset];
    stage4[2] = du[2 + offset];
    stage4[3] = du[6 + offset];
    stage4[4] = du[1 + offset] - du[7 + offset];
    stage4[5] = du[3 + offset] * std::sqrtf(2.f);
    stage4[6] = du[5 + offset] * std::sqrtf(2.f);
    stage4[7] = du[1 + offset] + du[7 + offset];
    
    ///
    //stage3
    ///
    std::array<float, 8> stage3;
    stage3[0] = stage4[0] + stage4[1];
    stage3[1] = stage4[0] - stage4[1];
    stage3[2] = stage4[2] * std::sqrtf(2.f) * std::cosf(6.f * std::numbers::pi / 16.f)
                - stage4[3] * std::sqrtf(2.f) * std::sinf(6.f * std::numbers::pi / 16.f );
    stage3[3] = stage4[2] * std::sqrtf(2.f) * std::sinf(6.f * std::numbers::pi / 16.f)
                + stage4[3] * std::sqrtf(2.f) * std::cosf(6.f * std::numbers::pi / 16.f );
    stage3[4] = stage4[4] + stage4[6];
    stage3[5] = stage4[7] - stage4[5];
    stage3[6] = stage4[4] - stage4[6];
    stage3[7] = stage4[7] + stage4[5];
    
    ///
    //stage 2
    ///
    std::array<float, 8> stage2;
    stage2[0] = stage3[0] + stage3[3];
    stage2[1] = stage3[1] + stage3[2];
    stage2[2] = stage3[1] - stage3[2];
    stage2[3] = stage3[0] - stage3[3];
    //stage 2, c3
    stage2[4] = stage3[4] * std::cosf(3.f * std::numbers::pi / 16.f)
                - stage3[7] * std::sinf(3.f * std::numbers::pi / 16.f );
    stage2[7] =  stage3[4] * std::sinf(3.f * std::numbers::pi / 16.f)
                + stage3[7] * std::cosf(3.f * std::numbers::pi / 16.f );
    //stage 2, c1
    stage2[5] = stage3[5] * std::cosf(1.f * std::numbers::pi / 16.f)
                - stage3[6] * std::sinf(1.f * std::numbers::pi / 16.f );
    stage2[6] =  stage3[5] * std::sinf(1.f * std::numbers::pi / 16.f)
                + stage3[6] * std::cosf(1.f * std::numbers::pi / 16.f );
    
    ///
    //stage 1
    ///
    inter[0 + offset] = stage2[0] + stage2[7];
    inter[1 + offset] = stage2[1] + stage2[6];
    inter[2 + offset] = stage2[2] + stage2[5];
    inter[3 + offset] = stage2[3] + stage2[4];
    inter[4 + offset] = stage2[3] - stage2[4];
    inter[5 + offset] = stage2[2] - stage2[5];
    inter[6 + offset] = stage2[1] - stage2[6];
    inter[7 + offset] = stage2[0] - stage2[7];
}

void image::loeffler_1d_idct_col(const std::array<float, 8*8>& du, std::array<int, 8*8>& out,  int offset) {
    ///
    //stage4
    ///
    std::array<float, 8> stage4;
    stage4[0] = du[0 + offset];
    stage4[1] = du[32 + offset];
    stage4[2] = du[16 + offset];
    stage4[3] = du[48 + offset];
    stage4[4] = du[8 + offset] - du[56 + offset];
    stage4[5] = du[24 + offset] * std::sqrtf(2.f);
    stage4[6] = du[40 + offset] * std::sqrtf(2.f);
    stage4[7] = du[8 + offset] + du[56 + offset];
    
    ///
    //stage3
    ///
    std::array<float, 8> stage3;
    stage3[0] = stage4[0] + stage4[1];
    stage3[1] = stage4[0] - stage4[1];
    stage3[2] = stage4[2] * std::sqrtf(2.f) * std::cosf(6.f * std::numbers::pi / 16.f)
                - stage4[3] * std::sqrtf(2.f) * std::sinf(6.f * std::numbers::pi / 16.f );
    stage3[3] = stage4[2] * std::sqrtf(2.f) * std::sinf(6.f * std::numbers::pi / 16.f)
                + stage4[3] * std::sqrtf(2.f) * std::cosf(6.f * std::numbers::pi / 16.f );
    stage3[4] = stage4[4] + stage4[6];
    stage3[5] = stage4[7] - stage4[5];
    stage3[6] = stage4[4] - stage4[6];
    stage3[7] = stage4[7] + stage4[5];
    
    ///
    //stage 2
    ///
    std::array<float, 8> stage2;
    stage2[0] = stage3[0] + stage3[3];
    stage2[1] = stage3[1] + stage3[2];
    stage2[2] = stage3[1] - stage3[2];
    stage2[3] = stage3[0] - stage3[3];
    //stage 2, c3
    stage2[4] = stage3[4] * std::cosf(3.f * std::numbers::pi / 16.f)
                - stage3[7] * std::sinf(3.f * std::numbers::pi / 16.f );
    stage2[7] =  stage3[4] * std::sinf(3.f * std::numbers::pi / 16.f)
                + stage3[7] * std::cosf(3.f * std::numbers::pi / 16.f );
    //stage 2, c1
    stage2[5] = stage3[5] * std::cosf(1.f * std::numbers::pi / 16.f)
                - stage3[6] * std::sinf(1.f * std::numbers::pi / 16.f );
    stage2[6] =  stage3[5] * std::sinf(1.f * std::numbers::pi / 16.f)
                + stage3[6] * std::cosf(1.f * std::numbers::pi / 16.f );
    
    ///
    //stage 1
    ///
    out[0*8 + offset] = (stage2[0] + stage2[7]) / 8.f;
    out[1*8 + offset] = (stage2[1] + stage2[6]) / 8.f;
    out[2*8 + offset] = (stage2[2] + stage2[5]) / 8.f;
    out[3*8 + offset] = (stage2[3] + stage2[4]) / 8.f;
    out[4*8 + offset] = (stage2[3] - stage2[4]) / 8.f;
    out[5*8 + offset] = (stage2[2] - stage2[5]) / 8.f;
    out[6*8 + offset] = (stage2[1] - stage2[6]) / 8.f;
    out[7*8 + offset] = (stage2[0] - stage2[7]) / 8.f;
}

image::DataUnit image::dct_float_loeffler(const DataUnit& du) {
    DataUnit out;
    
    //rows
    for (size_t y = 0; y < 8; y++) {
        std::array<int, 8> input;
        std::copy_n(du.begin() + y*8, 8, input.begin());
        auto output = loeffler_1d_dct(input);
        std::copy(output.begin(), output.end(), out.begin() + y*8);
    }
    
    //columns
    for (size_t x = 0; x < 8; x++) {
        std::array<int, 8> input;
        for (size_t y = 0; y < 8; y++) {
            input[y] = du[x + y*8];
        }
        
        auto column = loeffler_1d_dct(input);
        
        for (size_t y = 0; y < 8; y++) {
            out[x + y*8] = column[y];
        }
    }
        
    return out;
}

void image::idct_float_loeffler(DataUnit& du) {
    std::array<float, 8*8> intermediate;
    
    //rows
    for (int y = 0; y < 64; y+=8) {
        loeffler_1d_idct_row(du, intermediate, y);
    }
    
    //columns
    for (int x = 0; x < 8; x++) {
        loeffler_1d_idct_col(intermediate, du, x);
    }
}

image::DataUnit image::idct_int(const DataUnit& du) {
    DataUnit out;
    
    for (size_t y = 0; y < 8; y++) {
        for (size_t x = 0; x < 8; x++) {
            int val = 0.f;
            
            for (size_t u = 0; u < 8; u++) {
                for (size_t v = 0; v < 8; v++) {
                    float cu = (u == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    float cv = (v == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    val += cu * cv * du[u + v*8]
                            * std::cos((2.f * x + 1.f) * u * std::numbers::pi / 16.f)
                            * std::cos((2.f * y + 1.f) * v * std::numbers::pi / 16.f);
                }
            }
            val *= 0.25f;
            
            out[x + y * 8] = static_cast<int>(val);
        }
    }
    
    return out;
}

image::DataUnit image::idct_int_table(const DataUnit& du) {
    static auto table = cosTable();
    
    DataUnit out;
    
    for (size_t y = 0; y < 8; y++) {
        for (size_t x = 0; x < 8; x++) {
            int val = 0.f;
            
            for (size_t u = 0; u < 8; u++) {
                for (size_t v = 0; v < 8; v++) {
                    float cu = (u == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    float cv = (v == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    val += cu * cv * du[u + v*8]
                            * table[x + u * 8]
                            * table[y + v * 8];
                }
            }
            val *= 0.25f;
            
            out[x + y * 8] = static_cast<int>(val);
        }
    }
    
    return out;
}
