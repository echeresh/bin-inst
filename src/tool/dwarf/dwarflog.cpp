#include <fstream>
#include "dwarflog.h"

namespace dwarf
{
    std::ofstream dwarfLogRaw("dwarf.log");
    streamutils::OffsetOstream dwarfLog(dwarfLogRaw);
} //namespace dwarf
