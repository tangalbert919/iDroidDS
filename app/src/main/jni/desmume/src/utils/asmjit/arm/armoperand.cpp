// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if defined(ASMJIT_BUILD_ARM)

// [Dependencies]
#include "../arm/armoperand.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::ArmOperand - Test]
// ============================================================================

#if defined(ASMJIT_TEST)
UNIT(arm_operand) {
  Label L;

  INFO("Checking if arm::reg(...) matches built-in IDs");
  EXPECT(arm::w(5) == arm::w5);
  EXPECT(arm::x(5) == arm::x5);

  INFO("Checking GP register properties");
  EXPECT(ArmGp().isReg() == false);
  EXPECT(arm::w0.isReg() == true);
  EXPECT(arm::x0.isReg() == true);
  EXPECT(arm::w0.getId() == 0);
  EXPECT(arm::w31.getId() == 31);
  EXPECT(arm::x0.getId() == 0);
  EXPECT(arm::x31.getId() == 31);
  EXPECT(arm::w0.getSize() == 4);
  EXPECT(arm::x0.getSize() == 8);
  EXPECT(arm::w0.getType() == ArmReg::kRegGpw);
  EXPECT(arm::x0.getType() == ArmReg::kRegGpx);
  EXPECT(arm::w0.getGroup() == ArmReg::kGroupGp);
  EXPECT(arm::x0.getGroup() == ArmReg::kGroupGp);
}
#endif

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_ARM
