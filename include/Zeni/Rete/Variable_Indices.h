#ifndef ZENI_RETE_VARIABLE_INDICES_H
#define ZENI_RETE_VARIABLE_INDICES_H

#include "Token_Index.h"

#include <map>

namespace Zeni {

  namespace Rete {

    typedef std::multimap<std::string, Token_Index> Variable_Indices;

  }

}

#endif