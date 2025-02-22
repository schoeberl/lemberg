//===--- Triple.cpp - Target triple helper class --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Triple.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Twine.h"
#include <cassert>
#include <cstring>
using namespace llvm;

//

const char *Triple::getArchTypeName(ArchType Kind) {
  switch (Kind) {
  case InvalidArch: return "<invalid>";
  case UnknownArch: return "unknown";

  case alpha:   return "alpha";
  case arm:     return "arm";
  case bfin:    return "bfin";
  case cellspu: return "cellspu";
  case mips:    return "mips";
  case mipsel:  return "mipsel";
  case msp430:  return "msp430";
  case ppc64:   return "powerpc64";
  case ppc:     return "powerpc";
  case sparc:   return "sparc";
  case sparcv9: return "sparcv9";
  case systemz: return "s390x";
  case tce:     return "tce";
  case thumb:   return "thumb";
  case x86:     return "i386";
  case x86_64:  return "x86_64";
  case xcore:   return "xcore";
  case mblaze:  return "mblaze";
  case ptx:     return "ptx";
  case lemberg: return "lemberg";
  }

  return "<invalid>";
}

const char *Triple::getArchTypePrefix(ArchType Kind) {
  switch (Kind) {
  default:
    return 0;

  case alpha:   return "alpha";

  case arm:
  case thumb:   return "arm";

  case bfin:    return "bfin";

  case cellspu: return "spu";

  case ppc64:
  case ppc:     return "ppc";

  case mblaze:  return "mblaze";

  case sparcv9:
  case sparc:   return "sparc";

  case x86:
  case x86_64:  return "x86";

  case xcore:   return "xcore";

  case ptx:     return "ptx";

  case lemberg: return "lemberg";
  }
}

const char *Triple::getVendorTypeName(VendorType Kind) {
  switch (Kind) {
  case UnknownVendor: return "unknown";

  case Apple: return "apple";
  case PC: return "pc";
  }

  return "<invalid>";
}

const char *Triple::getOSTypeName(OSType Kind) {
  switch (Kind) {
  case UnknownOS: return "unknown";

  case AuroraUX: return "auroraux";
  case Cygwin: return "cygwin";
  case Darwin: return "darwin";
  case DragonFly: return "dragonfly";
  case FreeBSD: return "freebsd";
  case Linux: return "linux";
  case Lv2: return "lv2";
  case MinGW32: return "mingw32";
  case NetBSD: return "netbsd";
  case OpenBSD: return "openbsd";
  case Psp: return "psp";
  case Solaris: return "solaris";
  case Win32: return "win32";
  case Haiku: return "haiku";
  case Minix: return "minix";
  }

  return "<invalid>";
}

const char *Triple::getEnvironmentTypeName(EnvironmentType Kind) {
  switch (Kind) {
  case UnknownEnvironment: return "unknown";
  case GNU: return "gnu";
  case GNUEABI: return "gnueabi";
  case EABI: return "eabi";
  case MachO: return "macho";
  }

  return "<invalid>";
}

Triple::ArchType Triple::getArchTypeForLLVMName(StringRef Name) {
  if (Name == "alpha")
    return alpha;
  if (Name == "arm")
    return arm;
  if (Name == "bfin")
    return bfin;
  if (Name == "cellspu")
    return cellspu;
  if (Name == "mips")
    return mips;
  if (Name == "mipsel")
    return mipsel;
  if (Name == "msp430")
    return msp430;
  if (Name == "ppc64")
    return ppc64;
  if (Name == "ppc")
    return ppc;
  if (Name == "mblaze")
    return mblaze;
  if (Name == "sparc")
    return sparc;
  if (Name == "sparcv9")
    return sparcv9;
  if (Name == "systemz")
    return systemz;
  if (Name == "tce")
    return tce;
  if (Name == "thumb")
    return thumb;
  if (Name == "x86")
    return x86;
  if (Name == "x86-64")
    return x86_64;
  if (Name == "xcore")
    return xcore;
  if (Name == "ptx")
    return ptx;
  if (Name == "lemberg")
    return lemberg;

  return UnknownArch;
}

Triple::ArchType Triple::getArchTypeForDarwinArchName(StringRef Str) {
  // See arch(3) and llvm-gcc's driver-driver.c. We don't implement support for
  // archs which Darwin doesn't use.

  // The matching this routine does is fairly pointless, since it is neither the
  // complete architecture list, nor a reasonable subset. The problem is that
  // historically the driver driver accepts this and also ties its -march=
  // handling to the architecture name, so we need to be careful before removing
  // support for it.

  // This code must be kept in sync with Clang's Darwin specific argument
  // translation.

  if (Str == "ppc" || Str == "ppc601" || Str == "ppc603" || Str == "ppc604" ||
      Str == "ppc604e" || Str == "ppc750" || Str == "ppc7400" ||
      Str == "ppc7450" || Str == "ppc970")
    return Triple::ppc;

  if (Str == "ppc64")
    return Triple::ppc64;

  if (Str == "i386" || Str == "i486" || Str == "i486SX" || Str == "pentium" ||
      Str == "i586" || Str == "pentpro" || Str == "i686" || Str == "pentIIm3" ||
      Str == "pentIIm5" || Str == "pentium4")
    return Triple::x86;

  if (Str == "x86_64")
    return Triple::x86_64;

  // This is derived from the driver driver.
  if (Str == "arm" || Str == "armv4t" || Str == "armv5" || Str == "xscale" ||
      Str == "armv6" || Str == "armv7")
    return Triple::arm;

  if (Str == "ptx")
    return Triple::ptx;

  return Triple::UnknownArch;
}

// Returns architecture name that is understood by the target assembler.
const char *Triple::getArchNameForAssembler() {
  if (getOS() != Triple::Darwin && getVendor() != Triple::Apple)
    return NULL;

  StringRef Str = getArchName();
  if (Str == "i386")
    return "i386";
  if (Str == "x86_64")
    return "x86_64";
  if (Str == "powerpc")
    return "ppc";
  if (Str == "powerpc64")
    return "ppc64";
  if (Str == "mblaze" || Str == "microblaze")
    return "mblaze";
  if (Str == "arm")
    return "arm";
  if (Str == "armv4t" || Str == "thumbv4t")
    return "armv4t";
  if (Str == "armv5" || Str == "armv5e" || Str == "thumbv5"
      || Str == "thumbv5e")
    return "armv5";
  if (Str == "armv6" || Str == "thumbv6")
    return "armv6";
  if (Str == "armv7" || Str == "thumbv7")
    return "armv7";
  if (Str == "ptx")
    return "ptx";
  return NULL;
}

//

Triple::ArchType Triple::ParseArch(StringRef ArchName) {
  if (ArchName.size() == 4 && ArchName[0] == 'i' &&
      ArchName[2] == '8' && ArchName[3] == '6' &&
      ArchName[1] - '3' < 6) // i[3-9]86
    return x86;
  else if (ArchName == "amd64" || ArchName == "x86_64")
    return x86_64;
  else if (ArchName == "bfin")
    return bfin;
  else if (ArchName == "powerpc")
    return ppc;
  else if ((ArchName == "powerpc64") || (ArchName == "ppu"))
    return ppc64;
  else if (ArchName == "mblaze")
    return mblaze;
  else if (ArchName == "arm" ||
           ArchName.startswith("armv") ||
           ArchName == "xscale")
    return arm;
  else if (ArchName == "thumb" ||
           ArchName.startswith("thumbv"))
    return thumb;
  else if (ArchName.startswith("alpha"))
    return alpha;
  else if (ArchName == "spu" || ArchName == "cellspu")
    return cellspu;
  else if (ArchName == "msp430")
    return msp430;
  else if (ArchName == "mips" || ArchName == "mipsallegrex")
    return mips;
  else if (ArchName == "mipsel" || ArchName == "mipsallegrexel" ||
           ArchName == "psp")
    return mipsel;
  else if (ArchName == "sparc")
    return sparc;
  else if (ArchName == "sparcv9")
    return sparcv9;
  else if (ArchName == "s390x")
    return systemz;
  else if (ArchName == "tce")
    return tce;
  else if (ArchName == "xcore")
    return xcore;
  else if (ArchName == "ptx")
    return ptx;
  else if (ArchName == "lemberg")
    return lemberg;
  else
    return UnknownArch;
}

Triple::VendorType Triple::ParseVendor(StringRef VendorName) {
  if (VendorName == "apple")
    return Apple;
  else if (VendorName == "pc")
    return PC;
  else
    return UnknownVendor;
}

Triple::OSType Triple::ParseOS(StringRef OSName) {
  if (OSName.startswith("auroraux"))
    return AuroraUX;
  else if (OSName.startswith("cygwin"))
    return Cygwin;
  else if (OSName.startswith("darwin"))
    return Darwin;
  else if (OSName.startswith("dragonfly"))
    return DragonFly;
  else if (OSName.startswith("freebsd"))
    return FreeBSD;
  else if (OSName.startswith("linux"))
    return Linux;
  else if (OSName.startswith("lv2"))
    return Lv2;
  else if (OSName.startswith("mingw32"))
    return MinGW32;
  else if (OSName.startswith("netbsd"))
    return NetBSD;
  else if (OSName.startswith("openbsd"))
    return OpenBSD;
  else if (OSName.startswith("psp"))
    return Psp;
  else if (OSName.startswith("solaris"))
    return Solaris;
  else if (OSName.startswith("win32"))
    return Win32;
  else if (OSName.startswith("haiku"))
    return Haiku;
  else if (OSName.startswith("minix"))
    return Minix;
  else
    return UnknownOS;
}

Triple::EnvironmentType Triple::ParseEnvironment(StringRef EnvironmentName) {
  if (EnvironmentName.startswith("eabi"))
    return EABI;
  else if (EnvironmentName.startswith("gnueabi"))
    return GNUEABI;
  else if (EnvironmentName.startswith("gnu"))
    return GNU;
  else if (EnvironmentName.startswith("macho"))
    return MachO;
  else
    return UnknownEnvironment;
}

void Triple::Parse() const {
  assert(!isInitialized() && "Invalid parse call.");

  Arch = ParseArch(getArchName());
  Vendor = ParseVendor(getVendorName());
  OS = ParseOS(getOSName());
  Environment = ParseEnvironment(getEnvironmentName());

  assert(isInitialized() && "Failed to initialize!");
}

std::string Triple::normalize(StringRef Str) {
  // Parse into components.
  SmallVector<StringRef, 4> Components;
  for (size_t First = 0, Last = 0; Last != StringRef::npos; First = Last + 1) {
    Last = Str.find('-', First);
    Components.push_back(Str.slice(First, Last));
  }

  // If the first component corresponds to a known architecture, preferentially
  // use it for the architecture.  If the second component corresponds to a
  // known vendor, preferentially use it for the vendor, etc.  This avoids silly
  // component movement when a component parses as (eg) both a valid arch and a
  // valid os.
  ArchType Arch = UnknownArch;
  if (Components.size() > 0)
    Arch = ParseArch(Components[0]);
  VendorType Vendor = UnknownVendor;
  if (Components.size() > 1)
    Vendor = ParseVendor(Components[1]);
  OSType OS = UnknownOS;
  if (Components.size() > 2)
    OS = ParseOS(Components[2]);
  EnvironmentType Environment = UnknownEnvironment;
  if (Components.size() > 3)
    Environment = ParseEnvironment(Components[3]);

  // Note which components are already in their final position.  These will not
  // be moved.
  bool Found[4];
  Found[0] = Arch != UnknownArch;
  Found[1] = Vendor != UnknownVendor;
  Found[2] = OS != UnknownOS;
  Found[3] = Environment != UnknownEnvironment;

  // If they are not there already, permute the components into their canonical
  // positions by seeing if they parse as a valid architecture, and if so moving
  // the component to the architecture position etc.
  for (unsigned Pos = 0; Pos != array_lengthof(Found); ++Pos) {
    if (Found[Pos])
      continue; // Already in the canonical position.

    for (unsigned Idx = 0; Idx != Components.size(); ++Idx) {
      // Do not reparse any components that already matched.
      if (Idx < array_lengthof(Found) && Found[Idx])
        continue;

      // Does this component parse as valid for the target position?
      bool Valid = false;
      StringRef Comp = Components[Idx];
      switch (Pos) {
      default:
        assert(false && "unexpected component type!");
      case 0:
        Arch = ParseArch(Comp);
        Valid = Arch != UnknownArch;
        break;
      case 1:
        Vendor = ParseVendor(Comp);
        Valid = Vendor != UnknownVendor;
        break;
      case 2:
        OS = ParseOS(Comp);
        Valid = OS != UnknownOS;
        break;
      case 3:
        Environment = ParseEnvironment(Comp);
        Valid = Environment != UnknownEnvironment;
        break;
      }
      if (!Valid)
        continue; // Nope, try the next component.

      // Move the component to the target position, pushing any non-fixed
      // components that are in the way to the right.  This tends to give
      // good results in the common cases of a forgotten vendor component
      // or a wrongly positioned environment.
      if (Pos < Idx) {
        // Insert left, pushing the existing components to the right.  For
        // example, a-b-i386 -> i386-a-b when moving i386 to the front.
        StringRef CurrentComponent(""); // The empty component.
        // Replace the component we are moving with an empty component.
        std::swap(CurrentComponent, Components[Idx]);
        // Insert the component being moved at Pos, displacing any existing
        // components to the right.
        for (unsigned i = Pos; !CurrentComponent.empty(); ++i) {
          // Skip over any fixed components.
          while (i < array_lengthof(Found) && Found[i]) ++i;
          // Place the component at the new position, getting the component
          // that was at this position - it will be moved right.
          std::swap(CurrentComponent, Components[i]);
        }
      } else if (Pos > Idx) {
        // Push right by inserting empty components until the component at Idx
        // reaches the target position Pos.  For example, pc-a -> -pc-a when
        // moving pc to the second position.
        do {
          // Insert one empty component at Idx.
          StringRef CurrentComponent(""); // The empty component.
          for (unsigned i = Idx; i < Components.size();) {
            // Place the component at the new position, getting the component
            // that was at this position - it will be moved right.
            std::swap(CurrentComponent, Components[i]);
            // If it was placed on top of an empty component then we are done.
            if (CurrentComponent.empty())
              break;
            // Advance to the next component, skipping any fixed components.
            while (++i < array_lengthof(Found) && Found[i])
              ;
          }
          // The last component was pushed off the end - append it.
          if (!CurrentComponent.empty())
            Components.push_back(CurrentComponent);

          // Advance Idx to the component's new position.
          while (++Idx < array_lengthof(Found) && Found[Idx]) {}
        } while (Idx < Pos); // Add more until the final position is reached.
      }
      assert(Pos < Components.size() && Components[Pos] == Comp &&
             "Component moved wrong!");
      Found[Pos] = true;
      break;
    }
  }

  // Special case logic goes here.  At this point Arch, Vendor and OS have the
  // correct values for the computed components.

  // Stick the corrected components back together to form the normalized string.
  std::string Normalized;
  for (unsigned i = 0, e = Components.size(); i != e; ++i) {
    if (i) Normalized += '-';
    Normalized += Components[i];
  }
  return Normalized;
}

StringRef Triple::getArchName() const {
  return StringRef(Data).split('-').first;           // Isolate first component
}

StringRef Triple::getVendorName() const {
  StringRef Tmp = StringRef(Data).split('-').second; // Strip first component
  return Tmp.split('-').first;                       // Isolate second component
}

StringRef Triple::getOSName() const {
  StringRef Tmp = StringRef(Data).split('-').second; // Strip first component
  Tmp = Tmp.split('-').second;                       // Strip second component
  return Tmp.split('-').first;                       // Isolate third component
}

StringRef Triple::getEnvironmentName() const {
  StringRef Tmp = StringRef(Data).split('-').second; // Strip first component
  Tmp = Tmp.split('-').second;                       // Strip second component
  return Tmp.split('-').second;                      // Strip third component
}

StringRef Triple::getOSAndEnvironmentName() const {
  StringRef Tmp = StringRef(Data).split('-').second; // Strip first component
  return Tmp.split('-').second;                      // Strip second component
}

static unsigned EatNumber(StringRef &Str) {
  assert(!Str.empty() && Str[0] >= '0' && Str[0] <= '9' && "Not a number");
  unsigned Result = Str[0]-'0';

  // Eat the digit.
  Str = Str.substr(1);

  // Handle "darwin11".
  if (Result == 1 && !Str.empty() && Str[0] >= '0' && Str[0] <= '9') {
    Result = Result*10 + (Str[0] - '0');
    // Eat the digit.
    Str = Str.substr(1);
  }

  return Result;
}

/// getDarwinNumber - Parse the 'darwin number' out of the specific target
/// triple.  For example, if we have darwin8.5 return 8,5,0.  If any entry is
/// not defined, return 0's.  This requires that the triple have an OSType of
/// darwin before it is called.
void Triple::getDarwinNumber(unsigned &Maj, unsigned &Min,
                             unsigned &Revision) const {
  assert(getOS() == Darwin && "Not a darwin target triple!");
  StringRef OSName = getOSName();
  assert(OSName.startswith("darwin") && "Unknown darwin target triple!");

  // Strip off "darwin".
  OSName = OSName.substr(6);

  Maj = Min = Revision = 0;

  if (OSName.empty() || OSName[0] < '0' || OSName[0] > '9')
    return;

  // The major version is the first digit.
  Maj = EatNumber(OSName);
  if (OSName.empty()) return;

  // Handle minor version: 10.4.9 -> darwin8.9.
  if (OSName[0] != '.')
    return;

  // Eat the '.'.
  OSName = OSName.substr(1);

  if (OSName.empty() || OSName[0] < '0' || OSName[0] > '9')
    return;

  Min = EatNumber(OSName);
  if (OSName.empty()) return;

  // Handle revision darwin8.9.1
  if (OSName[0] != '.')
    return;

  // Eat the '.'.
  OSName = OSName.substr(1);

  if (OSName.empty() || OSName[0] < '0' || OSName[0] > '9')
    return;

  Revision = EatNumber(OSName);
}

void Triple::setTriple(const Twine &Str) {
  Data = Str.str();
  Arch = InvalidArch;
}

void Triple::setArch(ArchType Kind) {
  setArchName(getArchTypeName(Kind));
}

void Triple::setVendor(VendorType Kind) {
  setVendorName(getVendorTypeName(Kind));
}

void Triple::setOS(OSType Kind) {
  setOSName(getOSTypeName(Kind));
}

void Triple::setEnvironment(EnvironmentType Kind) {
  setEnvironmentName(getEnvironmentTypeName(Kind));
}

void Triple::setArchName(StringRef Str) {
  // Work around a miscompilation bug for Twines in gcc 4.0.3.
  SmallString<64> Triple;
  Triple += Str;
  Triple += "-";
  Triple += getVendorName();
  Triple += "-";
  Triple += getOSAndEnvironmentName();
  setTriple(Triple.str());
}

void Triple::setVendorName(StringRef Str) {
  setTriple(getArchName() + "-" + Str + "-" + getOSAndEnvironmentName());
}

void Triple::setOSName(StringRef Str) {
  if (hasEnvironment())
    setTriple(getArchName() + "-" + getVendorName() + "-" + Str +
              "-" + getEnvironmentName());
  else
    setTriple(getArchName() + "-" + getVendorName() + "-" + Str);
}

void Triple::setEnvironmentName(StringRef Str) {
  setTriple(getArchName() + "-" + getVendorName() + "-" + getOSName() +
            "-" + Str);
}

void Triple::setOSAndEnvironmentName(StringRef Str) {
  setTriple(getArchName() + "-" + getVendorName() + "-" + Str);
}
