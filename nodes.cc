#include "harb.h"
#include "nodes.h"

namespace harb {
namespace node {

boolean TrueVal(true);
boolean * True = &TrueVal;
boolean FalseVal(false);
boolean * False = &FalseVal;
pointer NilVal(NULL);
pointer * Nil = &NilVal;

}
}