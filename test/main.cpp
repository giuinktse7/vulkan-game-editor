#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#define VME_TEST

#include "../src/config.h"
#include "../src/debug.h"

int main(int argc, char *argv[])
{
    auto configResult = Config::create("12.60.10411");
    if (configResult.isErr())
    {
        ABORT_PROGRAM(configResult.unwrapErr().show());
    }

    Config config = configResult.unwrap();
    config.loadOrTerminate();

    int result = Catch::Session().run(argc, argv);
    return result;
}