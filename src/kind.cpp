#include "kind.h"

#include <iostream>

namespace atc {

std::ostream& operator<<(std::ostream& o, Kind k)
{
  switch (k)
  {
    case Kind::NONE: o << "NONE"; break;
    case Kind::TYPE: o << "TYPE"; break;
    case Kind::FUNCTION: o << "FUNCTION"; break;
    case Kind::PROOF: o << "PROOF"; break;
    case Kind::ABSTRACT: o << "ABSTRACT"; break;
    // terms
    case Kind::APPLY: o << "APPLY"; break;
    case Kind::LAMBDA: o << "LAMBDA"; break;
    case Kind::CONST: o << "CONST"; break;
    case Kind::VARIABLE: o << "VARIABLE"; break;
    case Kind::VARIABLE_LIST: o << "VARIABLE_LIST"; break;
    // literals
    case Kind::INTEGER: o << "INTEGER"; break;
    case Kind::DECIMAL: o << "DECIMAL"; break;
    case Kind::HEXADECIMAL: o << "HEXADECIMAL"; break;
    case Kind::BINARY: o << "BINARY"; break;
    case Kind::STRING: o << "STRING"; break;
    default: o << "UnknownKind(" << unsigned(k) << ")"; break;
  }
  return o;
}

}  // namespace atc
