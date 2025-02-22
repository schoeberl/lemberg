//===-- LembergPinner.cpp - Lemberg pinner --------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Pass to pin instructions onto clusters
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pinning"
#include "Lemberg.h"
#include "LembergTargetMachine.h"
#include "LembergSubtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

STATISTIC(NumPinned, "Number of pinned instructions");
STATISTIC(NumUnpinned, "Number of unpinned instructions");

namespace {

	static const unsigned ClusterCount = 4;

	static LembergFU::FuncUnit AluItins [ClusterCount][2] = {
		{LembergFU::SLOT0, LembergFU::ALU0},
		{LembergFU::SLOT1, LembergFU::ALU1},
		{LembergFU::SLOT2, LembergFU::ALU2},
		{LembergFU::SLOT3, LembergFU::ALU3}
	};

	static LembergFU::FuncUnit MemItins [ClusterCount][2] = {
		{LembergFU::SLOT0, LembergFU::MEMU},
		{LembergFU::SLOT1, LembergFU::MEMU},
		{LembergFU::SLOT2, LembergFU::MEMU},
		{LembergFU::SLOT3, LembergFU::MEMU}
	};

	static LembergFU::FuncUnit JmpItins [ClusterCount][2] = {
		{LembergFU::SLOT0, LembergFU::JMPU},
		{LembergFU::SLOT1, LembergFU::JMPU},
		{LembergFU::SLOT2, LembergFU::JMPU},
		{LembergFU::SLOT3, LembergFU::JMPU}
	};

	static LembergFU::FuncUnit Fpu0Itins [ClusterCount][3] = {
		{LembergFU::SLOT0, LembergFU::FPU_DEC, LembergFU::FPU_WB},
		{LembergFU::SLOT1, LembergFU::FPU_DEC, LembergFU::FPU_WB},
		{LembergFU::SLOT2, LembergFU::FPU_DEC, LembergFU::FPU_WB},
		{LembergFU::SLOT3, LembergFU::FPU_DEC, LembergFU::FPU_WB}
	};

	static LembergFU::FuncUnit Fpu1Itins [ClusterCount][4] = {
		{LembergFU::SLOT0, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_WB},
		{LembergFU::SLOT1, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_WB},
		{LembergFU::SLOT2, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_WB},
		{LembergFU::SLOT3, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_WB}
	};

	static LembergFU::FuncUnit Fpu2Itins [ClusterCount][5] = {
		{LembergFU::SLOT0, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_WB},
		{LembergFU::SLOT1, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_WB},
		{LembergFU::SLOT2, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_WB},
		{LembergFU::SLOT3, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_WB}
	};

	static LembergFU::FuncUnit Fpu3Itins [ClusterCount][6] = {
		{LembergFU::SLOT0, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_WB},
		{LembergFU::SLOT1, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_WB},
		{LembergFU::SLOT2, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_WB},
		{LembergFU::SLOT3, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_WB}
	};

	static LembergFU::FuncUnit Fpu4Itins [ClusterCount][7] = {
		{LembergFU::SLOT0, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_EX4, 
		 LembergFU::FPU_WB},
		{LembergFU::SLOT1, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_EX4, 
		 LembergFU::FPU_WB},
		{LembergFU::SLOT2, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_EX4, 
		 LembergFU::FPU_WB},
		{LembergFU::SLOT3, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_EX4, 
		 LembergFU::FPU_WB}
	};

	static LembergFU::FuncUnit Fpu6Itins [ClusterCount][9] = {
		{LembergFU::SLOT0, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_EX4,
		 LembergFU::FPU_EX5, LembergFU::FPU_EX6, LembergFU::FPU_WB},
		{LembergFU::SLOT1, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_EX4,
		 LembergFU::FPU_EX5, LembergFU::FPU_EX6, LembergFU::FPU_WB},
		{LembergFU::SLOT2, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_EX4,
		 LembergFU::FPU_EX5, LembergFU::FPU_EX6, LembergFU::FPU_WB},
		{LembergFU::SLOT3, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_EX4,
		 LembergFU::FPU_EX5, LembergFU::FPU_EX6, LembergFU::FPU_WB}
	};

	static LembergFU::FuncUnit Fpu7Itins [ClusterCount][10] = {
		{LembergFU::SLOT0, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_EX4,
		 LembergFU::FPU_EX5, LembergFU::FPU_EX6, LembergFU::FPU_EX7, 
		 LembergFU::FPU_WB},
		{LembergFU::SLOT1, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_EX4,
		 LembergFU::FPU_EX5, LembergFU::FPU_EX6, LembergFU::FPU_EX7, 
		 LembergFU::FPU_WB},
		{LembergFU::SLOT2, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_EX4,
		 LembergFU::FPU_EX5, LembergFU::FPU_EX6, LembergFU::FPU_EX7, 
		 LembergFU::FPU_WB},
		{LembergFU::SLOT3, LembergFU::FPU_DEC, LembergFU::FPU_EX1,
		 LembergFU::FPU_EX2, LembergFU::FPU_EX3, LembergFU::FPU_EX4,
		 LembergFU::FPU_EX5, LembergFU::FPU_EX6, LembergFU::FPU_EX7, 
		 LembergFU::FPU_WB}
	};

	static const TargetRegisterClass *Clusters [ClusterCount] =
		{ Lemberg::L0RegisterClass, Lemberg::L1RegisterClass,
		  Lemberg::L2RegisterClass, Lemberg::L3RegisterClass };
	static const TargetRegisterClass *MulClusters [ClusterCount] =
		{ Lemberg::M0RegisterClass, Lemberg::M1RegisterClass,
		  Lemberg::M2RegisterClass, Lemberg::M3RegisterClass };

	struct Pinner : public MachineFunctionPass {
	private:
		unsigned AluSchedClasses [ClusterCount];
		unsigned MemSchedClasses [ClusterCount];
		unsigned JmpSchedClasses [ClusterCount];
		unsigned Fpu0SchedClasses [ClusterCount];
		unsigned Fpu1SchedClasses [ClusterCount];
		unsigned Fpu2SchedClasses [ClusterCount];
		unsigned Fpu3SchedClasses [ClusterCount];
		unsigned Fpu4SchedClasses [ClusterCount];
		unsigned Fpu6SchedClasses [ClusterCount];
		unsigned Fpu7SchedClasses [ClusterCount];

	public:
		TargetMachine &TM;
		const TargetInstrInfo *TII;

		static char ID;
		Pinner(TargetMachine &tm) 
			: MachineFunctionPass(ID), TM(tm), TII(tm.getInstrInfo()) { 

			for (unsigned i = 0; i < ClusterCount; ++i) {
				AluSchedClasses[i] = getClassForPattern(AluItins[i], 2);
				MemSchedClasses[i] = getClassForPattern(MemItins[i], 2);
				JmpSchedClasses[i] = getClassForPattern(JmpItins[i], 2);
				Fpu0SchedClasses[i] = getClassForPattern(Fpu0Itins[i], 3);
				Fpu1SchedClasses[i] = getClassForPattern(Fpu1Itins[i], 4);
				Fpu2SchedClasses[i] = getClassForPattern(Fpu2Itins[i], 5);
				Fpu3SchedClasses[i] = getClassForPattern(Fpu3Itins[i], 6);
				Fpu4SchedClasses[i] = getClassForPattern(Fpu4Itins[i], 7);
				Fpu6SchedClasses[i] = getClassForPattern(Fpu6Itins[i], 9);
				Fpu7SchedClasses[i] = getClassForPattern(Fpu7Itins[i], 10);
			}
		}

		virtual const char *getPassName() const {
			return "Lemberg Pinner";
		}

		int getCluster(MachineRegisterInfo *MRI, MachineInstr &MI);
		void pinToCluster(MachineInstr &MI, int cluster);

		bool runOnMachineFunction(MachineFunction &F);

	private:
		unsigned getClassForPattern(LembergFU::FuncUnit P [], unsigned L);
		bool compatibleSchedClass(unsigned A, unsigned B);
	};
		
	char Pinner::ID = 0;

	struct PostPinner : Pinner {

		PostPinner(TargetMachine &tm) 
			: Pinner(tm) {
		}

		virtual const char *getPassName() const {
			return "Lemberg PostPinner";
		}

		bool runOnMachineFunction(MachineFunction &F);

	};

} // end of anonymous namespace

// createLembergPinnerPass - Returns a pass that pins
FunctionPass *llvm::createLembergPinnerPass(LembergTargetMachine &TM,
											CodeGenOpt::Level OptLevel) {
	return new Pinner(TM);
}

// createLembergPostPinnerPass - Returns a pass that pins after scheduling
FunctionPass *llvm::createLembergPostPinnerPass(LembergTargetMachine &TM,
												CodeGenOpt::Level OptLevel) {
	return new PostPinner(TM);
}

unsigned Pinner::getClassForPattern(LembergFU::FuncUnit P [], unsigned L) {

	const LembergSubtarget *LST = ((LembergTargetMachine &)TM).getSubtargetImpl();
	const InstrItineraryData &IID = LST->getInstrItins();
	const InstrItinerary *IITab = IID.Itineraries;
	const InstrStage *IIStages = IID.Stages;

	for (unsigned c = 0; !IID.isEndMarker(c); ++c) {
		// Do not accept itineraties with different length
		if (IITab[c].LastStage - IITab[c].FirstStage != L) {
			continue;
		}
		unsigned i, s;
		for (i = 0, s = IITab[c].FirstStage; i < L; ++i, ++s) {
			// Units are not equal
			if (IIStages[s].getUnits() != LST->getFuncUnit(P[i]))
				break;
		}
		// All units are the same
		if (i == L)
			return c;
	}

	llvm_unreachable("Cannot find class for itinerary pattern");
}

bool Pinner::compatibleSchedClass(unsigned A, unsigned B) {
	const LembergSubtarget *LST = ((LembergTargetMachine &)TM).getSubtargetImpl();
	const InstrItineraryData &IID = LST->getInstrItins();
	const InstrItinerary *IITab = IID.Itineraries;
	const InstrStage *IIStages = IID.Stages;

	// Need to have same length
	if (IITab[A].LastStage - IITab[A].FirstStage !=
		IITab[B].LastStage - IITab[B].FirstStage) {
		return false;
	}

	for (unsigned a = IITab[A].FirstStage, b = IITab[B].FirstStage;
		 a != IITab[A].LastStage && b != IITab[B].LastStage;
		 ++a, ++b) {
		// Stages must at least overlap
		if ((IIStages[a].getUnits() & IIStages[b].getUnits()) == 0) {
			return false;
		}
	}

	return true;
}

int Pinner::getCluster(MachineRegisterInfo *MRI, MachineInstr &MI) {

	// Only dummy operands, call target is immediate
	if (MI.getOpcode() == Lemberg::CALLga) {
		return -1;
	}
	// Only call target operand is relevant
	if (MI.getOpcode() == Lemberg::CALL) {
		unsigned regNo = MI.getOperand(2).getReg();
		for (unsigned k = 0; k < ClusterCount; k++) {
			if (Clusters[k]->contains(regNo)) {
				return k;
			}
		}
		return -1;
	}

	// All operands are relevant
	for (unsigned i = 0, e = MI.getNumOperands(); i < e; ++i) {
		MachineOperand &MO = MI.getOperand(i);
		if (!MO.isReg())
			continue;
		unsigned regNo = MO.getReg();
		for (unsigned k = 0; k < ClusterCount; k++) {
			if (Clusters[k]->contains(regNo)
				|| MulClusters[k]->contains(regNo)) {
				return k;
			}
		}
	}

	return -1;
}

void Pinner::pinToCluster(MachineInstr &MI, int cluster) {

	// Ignore implicitly created no-ops and inline asm
	if (MI.isKill() || MI.isImplicitDef() || MI.isInlineAsm()) {
		return;
	}

	unsigned SchedClass = MI.getDesc().getSchedClass();

	const TargetInstrDesc &TID = MI.getDesc();
	TargetInstrDesc *NTID = new TargetInstrDesc();
	*NTID = TID;
	
	if (compatibleSchedClass(SchedClass, AluSchedClasses[cluster])) {
		NTID->SchedClass = AluSchedClasses[cluster];
	} else if (compatibleSchedClass(SchedClass, MemSchedClasses[cluster])) {
		NTID->SchedClass = MemSchedClasses[cluster];
	} else if (compatibleSchedClass(SchedClass, JmpSchedClasses[cluster])) {
		NTID->SchedClass = JmpSchedClasses[cluster];
	} else if (compatibleSchedClass(SchedClass, Fpu0SchedClasses[cluster])) {
		NTID->SchedClass = Fpu0SchedClasses[cluster];
	} else if (compatibleSchedClass(SchedClass, Fpu1SchedClasses[cluster])) {
		NTID->SchedClass = Fpu1SchedClasses[cluster];
	} else if (compatibleSchedClass(SchedClass, Fpu2SchedClasses[cluster])) {
		NTID->SchedClass = Fpu2SchedClasses[cluster];
	} else if (compatibleSchedClass(SchedClass, Fpu3SchedClasses[cluster])) {
		NTID->SchedClass = Fpu3SchedClasses[cluster];
	} else if (compatibleSchedClass(SchedClass, Fpu4SchedClasses[cluster])) {
		NTID->SchedClass = Fpu4SchedClasses[cluster];
	} else if (compatibleSchedClass(SchedClass, Fpu6SchedClasses[cluster])) {
		NTID->SchedClass = Fpu6SchedClasses[cluster];
	} else if (compatibleSchedClass(SchedClass, Fpu7SchedClasses[cluster])) {
		NTID->SchedClass = Fpu7SchedClasses[cluster];
	} else {
		MI.dump();
		llvm_unreachable("Cannot find compatible schedule class for pinning");
	}
	
	MI.setDesc(*NTID);
}

bool Pinner::runOnMachineFunction(MachineFunction &F) 
{
	bool Changed = false;

	MachineRegisterInfo *MRI = &F.getRegInfo();

	for (MachineFunction::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
		for (MachineBasicBlock::iterator BI = FI->begin(), BE = FI->end(); BI != BE; ++BI) {

			MachineInstr &MI = *BI;
			int cluster = getCluster(MRI, MI);

			if (cluster < 0) {
				NumUnpinned++;
			} else {
				pinToCluster(MI, cluster);
				Changed = true;
				NumPinned++;
			}
		}
	}

	return Changed;
}

bool PostPinner::runOnMachineFunction(MachineFunction &F) 
{
	bool Changed = false;

	MachineRegisterInfo *MRI = &F.getRegInfo();

	SmallVector<MachineInstr *, ClusterCount> UnPinned;
	unsigned ScoreBoard = 0;

	for (MachineFunction::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
		for (MachineBasicBlock::iterator BI = FI->begin(), BE = FI->end(); BI != BE; ++BI) {

			MachineInstr &MI = *BI;

			if (MI.getOpcode() == Lemberg::NOP) {
				// No cluster needed for a NOP
				continue;
			}
			
			if (MI.getOpcode() == Lemberg::SEP) {
				// Handle unpinned insns
				unsigned cluster = 0;
				for (SmallVector<MachineInstr *, ClusterCount>::iterator
						 I = UnPinned.begin(), E = UnPinned.end(); I != E; ++I) {
					MachineInstr *P = *I;
					for ( ; cluster < ClusterCount; cluster++) {
						if ((ScoreBoard & (1 << cluster)) == 0) {
							pinToCluster(*P, cluster);
							ScoreBoard |= 1 << cluster;
							Changed = true;
							break;
						}
					}
				}
				assert(cluster != ClusterCount && "Could not find suitable cluster");
				// Reset state
				UnPinned.clear();
				ScoreBoard = 0;
				continue;
			}
			
			int cluster = getCluster(MRI, MI);		
			if (cluster < 0) {
				UnPinned.push_back(&MI);
			} else {
				ScoreBoard |= 1 << cluster;
			}
		}
	}

	return Changed;
}

