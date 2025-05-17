
namespace baselines {


template <typename word_t>
word_t fujita_step_shifted_words(
    word_t A, word_t B, word_t C, 
    word_t H, word_t I, word_t D,
    word_t G, word_t F, word_t E
) {

    // 1.
    const word_t AB_1 = A & B;
    const word_t AB_0 = A ^ B;
    // 2.
    const word_t CD_1 = C & D;
    const word_t CD_0 = C ^ D;
    // 3.
    const word_t EF_1 = E & F;
    const word_t EF_0 = E ^ F;
    // 4.
    const word_t GH_1 = G & H;
    const word_t GH_0 = G ^ H;
    // 5.
    const word_t AD_0 = AB_0 ^ CD_0;
    // 6.
    const word_t AD_1 = AB_1 ^ CD_1 ^ (AB_0 & CD_0);
    // 7.
    const word_t AD_2 = AB_1 & CD_1;
    // 8.
    const word_t EH_0 = EF_0 ^ GH_0;
    // 9.
    const word_t EH_1 = EF_1 ^ GH_1 ^ (EF_0 & GH_0);
    // 10.
    const word_t EH_2 = EF_1 & GH_1;
    // 11.
    const word_t AH_0 = AD_0 ^ EH_0;
    // 12.
    const word_t X = AD_0 & EH_0;
    // 13.
    const word_t Y = AD_1 ^ EH_1;
    // 14.
    const word_t AH_1 = X ^ Y;
    // 15.
    const word_t AH_23 = AD_2 | EH_2 | (AD_1 & EH_1) | (X & Y);
    // 17. neither of the 2 most significant bits is set and the second least significant bit is set
    const word_t Z = ~AH_23 & AH_1;
    // 18. (two neighbors) the least significant bit is not set and Z
    const word_t I_2 = ~AH_0 & Z;
    // 19. (three neighbors) the least significant bit is set and Z
    const word_t I_3 = AH_0 & Z;
    // 20.
    return (I & I_2) | I_3;   
}

template <typename word_t>
word_t fujita_step_from_neighborhood(
    word_t lt, word_t ct, word_t rt, 
    word_t lc, word_t cc, word_t rc,
    word_t lb, word_t cb, word_t rb
) {
    constexpr int BITS = sizeof(word_t) * 8;

    // the top-left neighbors of the center cell:
    const word_t A = (lc << 1) | (lt >> (BITS - 1));
    // the top neighbors of the center cell:
    const word_t B = (cc << 1) | (ct >> (BITS - 1));
    // the top-right neighbors of the center cell:
    const word_t C = (rc << 1) | (rt >> (BITS - 1));
    // the right neighbors of the center cell:
    const word_t D = rc;
    // the bottom-right neighbors of the center cell:
    const word_t E = (rc >> 1) | (rb << (BITS - 1));
    // the bottom neighbors of the center cell:
    const word_t F = (cc >> 1) | (cb << (BITS - 1));
    // the bottom-left neighbors of the center cell:
    const word_t G = (lc >> 1) | (lb << (BITS - 1));
    // the left neighbors of the center cell:
    const word_t H = lc;
    const word_t I = cc;

    return fujita_step_shifted_words(A, B, C, H, I, D, G, F, E);
}

template <typename word_t>
void compute_using_fujita(word_t* bit_grid, word_t* final_output, std::size_t height, std::size_t width, int iters) {

    auto input = bit_grid;
    auto output = final_output;

    for (int i = 0; i < iters; ++i) {
        for (std::size_t y = 1; y < height - 1; ++y) {
            for (std::size_t x = 1; x < width - 1; ++x) {
                
                auto lt = bit_grid[(y - 1) * width + (x - 1)];
                auto ct = bit_grid[(y - 1) * width + x];
                auto rt = bit_grid[(y - 1) * width + (x + 1)];
                auto lc = bit_grid[y * width + (x - 1)];
                auto cc = bit_grid[y * width + x];
                auto rc = bit_grid[y * width + (x + 1)];
                auto lb = bit_grid[(y + 1) * width + (x - 1)];
                auto cb = bit_grid[(y + 1) * width + x];
                auto rb = bit_grid[(y + 1) * width + (x + 1)];

                word_t next_state = fujita_step_from_neighborhood(lt, ct, rt, lc, cc, rc, lb, cb, rb);

                output[y * width + x] = next_state;
            }
        }

        std::swap(input, output);
    }
}

}