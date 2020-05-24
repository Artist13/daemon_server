#include "server/server.hpp"

int main(int argc, char const *argv[])
{
  Server local;
  local.bind(5999);
  local.listen();

  return 0;
}
