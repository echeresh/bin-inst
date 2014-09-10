#include "dwarfparser.h"
#include "debuginfo.h"
using namespace std;

int main()
{
    const char* binPath = "../NPB3.3.1/NPB3.3-OMP/bin/lu.S.x";
    auto* parser = new dwarf::Parser(binPath);
    parser->cuWalk(bind(&dwarf::Parser::walk, parser, placeholders::_1, placeholders::_2));
    auto dctxt = parser->getDebugContext().toDbg();
    cout << "main exit" << endl;
    return 0;
}