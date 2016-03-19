#include <fstream>
#include "dwarflog.h"
#include "common/offsetostream.h"

namespace dwarf
{
    std::ofstream dwarfLogRaw("dwarf.log");
    OffsetOstream dwarfLog(dwarfLogRaw);
} //namespace dwarf
