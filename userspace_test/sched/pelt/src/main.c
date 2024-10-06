#include<stdio.h>

static const unsigned int runnable_avg_yN_inv[] = {
	0xffffffff, 0xfa83b2da, 0xf5257d14, 0xefe4b99a, 0xeac0c6e6, 0xe5b906e6,
	0xe0ccdeeb, 0xdbfbb796, 0xd744fcc9, 0xd2a81d91, 0xce248c14, 0xc9b9bd85,
	0xc5672a10, 0xc12c4cc9, 0xbd08a39e, 0xb8fbaf46, 0xb504f333, 0xb123f581,
	0xad583ee9, 0xa9a15ab4, 0xa5fed6a9, 0xa2704302, 0x9ef5325f, 0x9b8d39b9,
	0x9837f050, 0x94f4efa8, 0x91c3d373, 0x8ea4398a, 0x8b95c1e3, 0x88980e80,
	0x85aac367, 0x82cd8698,
};

#define LOAD_AVG_PERIOD 32
#define LOAD_AVG_MAX 47742
static inline unsigned long mul_u64_u32_shr(unsigned long a, unsigned int mul, unsigned int shift)
{
	return (unsigned long)(((unsigned __int128)a * mul) >> shift);
}

static unsigned long decay_load(unsigned long val, unsigned long n)
{
        unsigned int local_n;

        //if (n > LOAD_AVG_PERIOD * 63)
         //       return 0;

        /* after bounds checking we can collapse to 32-bit */
        local_n = n;

        /*
         * As y^PERIOD = 1/2, we can combine
         *    y^n = 1/2^(n/PERIOD) * y^(n%PERIOD)
         * With a look-up table which covers y^n (n<PERIOD)
         *
         * To achieve constant time decay_load.
         */
        if (local_n >= LOAD_AVG_PERIOD) {
                val >>= local_n / LOAD_AVG_PERIOD;
                local_n %= LOAD_AVG_PERIOD;
        }

        val = mul_u64_u32_shr(val, runnable_avg_yN_inv[local_n], 32);
        return val;
}

static unsigned int __accumulate_pelt_segments(unsigned long periods, unsigned int d1, unsigned int d3)
{
        unsigned int c1, c2, c3 = d3; /* y^0 == 1 */

        /*
         * c1 = d1 y^p
         */
        c1 = decay_load((unsigned long)d1, periods);

        /*
         *            p-1
         * c2 = 1024 \Sum y^n
         *            n=1
         *
         *              inf        inf
         *    = 1024 ( \Sum y^n - \Sum y^n - y^0 )
         *              n=0        n=p
         */
        c2 = LOAD_AVG_MAX - decay_load(LOAD_AVG_MAX, periods) - 1024;

        return c1 + c2 + c3;
}

static void test_decay_load()
{
	unsigned long val;
	int i;
	printf("test period decay max period 32\n");
	for (i = 0; i < 32; ++i) {
        val = mul_u64_u32_shr(LOAD_AVG_MAX, runnable_avg_yN_inv[i], 32);
		printf("zz %s val:%ld  periods:%d\n",__func__, (unsigned long)val, i);
	}

	printf("test period decay less period 32\n");
	for (i = 32; i < LOAD_AVG_PERIOD * 63 ; i++) {
        val = decay_load(LOAD_AVG_MAX, i);
		printf("zz %s val:%ld  periods:%d\n",__func__, (unsigned long)val, i);
		if (val == 0)
			break;
	}

}

static void test_pelt_segmen()
{
	unsigned long val;
	int i;
	printf("test period decay segment max period 32\n");
	for (i = 0; i < 1024; ++i) {
		val = __accumulate_pelt_segments(i, 0, 0);
		printf("zz %s val:%ld  segment periods:%d\n",__func__, (unsigned long)val, i);
	}
};

int main(int argc, char *argv[])
{
	unsigned long ret;	
	test_decay_load();
	test_pelt_segmen();
	//ret = __accumulate_pelt_segments(0, 0, 0);
	//printf("zz %s ret:%lx \n",__func__, (unsigned long)ret);
	return 0;
}
