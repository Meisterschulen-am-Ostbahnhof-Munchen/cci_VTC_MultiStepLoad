#ifndef PREPARE_POOL_C36FCA404E774BADA460EC6010EDC239
#define PREPARE_POOL_C36FCA404E774BADA460EC6010EDC239

#include "IsoCommonDef.h"
#ifdef __cplusplus
#include <vector>

namespace PreparePool
{

bool parsePool(const iso_u8* srcPool, iso_u32 srcPoolSize,
    const iso_u8* macroList, iso_u8 macroListSize,
    std::vector<iso_u8>& basePool,
    std::vector<iso_u8>& secondaryPool,
    std::vector<iso_u8>& gAuxPool);

} /* namespace PreparePool */
#endif /* __cplusplus */
#endif /* PREPARE_POOL_C36FCA404E774BADA460EC6010EDC239 */
