/*
 * Complexity.h
 *
 *  Created on: Oct 23, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef COMPLEXITY_H_
#define COMPLEXITY_H_

#include "ModelObjects.h"

int calcMcCabeComplexity(ModelStatements const &stmts);
int calcOovComplexity(ModelOperation const &oper);

#endif /* COMPLEXITY_H_ */
