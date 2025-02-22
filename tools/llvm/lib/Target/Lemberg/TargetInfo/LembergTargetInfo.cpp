//===-- LembergTargetInfo.cpp - Lemberg Target Implementation -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Lemberg.h"
#include "llvm/Module.h"
#include "llvm/Target/TargetRegistry.h"

using namespace llvm;

Target llvm::TheLembergTarget;

extern "C" void LLVMInitializeLembergTargetInfo() {
  RegisterTarget<Triple::lemberg> X(TheLembergTarget, "lemberg",
									"Lemberg [experimental]");
}
