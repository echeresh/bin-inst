#include "dwarfparser.h"
#include "debuginfo.h"
#include "config.h"
using namespace std;

static std::string TEST_BIN_PATH = "a.out";

int main()
{
    const char* binPath = TEST_BIN_PATH;
    auto* parser = new dwarf::Parser(binPath);
    parser->cuWalk(bind(&dwarf::Parser::walk, parser, placeholders::_1, placeholders::_2));
    auto dctxt = parser->getDebugContext().toDbg();
    cout << "main exit" << endl;
    return 0;
}
